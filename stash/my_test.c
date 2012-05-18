#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strncpy */
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>

#define MESSENGER_SERVICE_NAME_LEN 32
#define ITEM_SIZE 256
#define THREADS 2
#define ARRAY_SIZE( x ) ( int32_t ) ( sizeof( x ) / sizeof( x[ 0 ] ) )
#define MAX_TAKE 10

#if SIZEOF_LONG == SIZEOF_VOIDP
typedef unsigned long st_data_t;
#elif SIZEOF__LONG_LONG == SIZEOF_VOIDP
typedef unsigned LONG_LONG st_data_t;
#endif

struct job_opt {
  char service_name[MESSENGER_SERVICE_NAME_LEN];
  int server_socket;
  void *buffer;
  uint32_t buffer_len;
};


struct job_item {
  struct job_opt opt;
  uint16_t done;
  pthread_t tag_id;
};


/*
 * one job control structure for each service for all job items
 */
struct job_ctrl {
  /*
   * incremented by one after a new job item is added to item
  */
  uint32_t job_end;
  uint32_t job_start;
  uint32_t job_done;
  /*
   * incremented by one after a job item has been retrieved from item for
   * processing
  */
  pthread_t tag_id;
  /*
  * an array of job items to handle.
  * The client that is connected to the service uses adds items to this array.
  */
  struct job_item item[ ITEM_SIZE ];
};

/* this lock protects all the variables above */
static pthread_mutex_t work_mutex;


static inline void
job_lock( void ) {
  pthread_mutex_lock( &work_mutex );
}


static inline void
job_unlock( void ) {
  pthread_mutex_unlock( &work_mutex );
}


static inline pthread_t
self( void ) {
  return pthread_self();
}


/* signalled when new work is added to todo */
static pthread_cond_t cond_add;


/* signalled when the result of one item is written to socket */
static pthread_cond_t cond_write;


static int
store_conditional_item( struct job_item *item, const struct job_opt *opt ) {

  if ( item->tag_id == self() ) {
    item->tag_id = 0;
    item->done = 0;
    memcpy( &item->opt, opt, sizeof( *opt ) );
    return 1;
  }
  return 0;
}


void
load_lock_ctrl( struct job_ctrl *ctrl ) {
  ctrl->tag_id = self();
}


struct job_item *
load_lock_item( struct job_item *item ) {
  item->tag_id = self();
  return item;
}


static void
update_server_socket( struct job_ctrl *ctrl, const char *service_name, const int server_socket ) {
  struct job_item *item;
  int i;
  int end = ctrl->job_end;

  for ( i = 0; i < end; i++ ) {
    item = load_lock_item( &ctrl->item[ i ] );
    if ( item->opt.server_socket == -1 ) {
      if ( !strncmp( service_name, item->opt.service_name, MESSENGER_SERVICE_NAME_LEN ) ) {
        if ( item->tag_id == self() ) {
          item->tag_id = 0;
          item->opt.server_socket = server_socket;
        }
      }
    }
  }
}


static int
add_job( struct job_ctrl *ctrl, const struct job_opt *opt ) {
  struct job_item *item;
  uint32_t end;
  uint32_t done;
  
  while ( 1 ) {
    end = ctrl->job_end;
    done = ctrl->job_done;
    if ( ( end + 1 ) % ARRAY_SIZE( ctrl->item ) == done ) {
      return 0; 
    }
    item = load_lock_item( &ctrl->item[ end ] );
    if ( store_conditional_item( item, opt ) ) {
      load_lock_ctrl( ctrl );
      if ( ctrl->tag_id == self() ) {
        ctrl->tag_id = 0;
        ctrl->job_end = ( ctrl->job_end + 1 ) % ARRAY_SIZE( ctrl->item );
        return 1;
      }
    }
  }
}


static int
get_exec_job( struct job_ctrl *ctrl ) {
  struct job_item *items[ MAX_TAKE ];
  struct job_item *item;
  int start;
  int end;
  uint32_t total_len = 0;
  uint16_t count = 0;
  uint16_t i;


  start = ctrl->job_start;
  while ( 1 ) {
    load_lock_ctrl( ctrl );
    if ( ctrl->tag_id == self() ) {
      ctrl->tag_id = 0;
      end = ctrl->job_end;
      break;
    }
  }

  if ( start == end ) {
    return 0;
  }
  while ( start != end ) {
    item = load_lock_item( &ctrl->item[ start ] );
    if ( item->opt.server_socket != -1 ) {
      items[ count ] = item;
      total_len += items[ count++ ]->opt.buffer_len;
      if ( item->opt.server_socket != items[ count - 1 ]->opt.server_socket || count == MAX_TAKE ) {
        break;
      }
    }
    while ( 1 ) {
      load_lock_ctrl( ctrl );
      if ( ctrl->tag_id == self() ) {
        ctrl->job_start = ( ctrl->job_start + 1 ) % ARRAY_SIZE( ctrl->item );
        start = ctrl->job_start;
        ctrl->tag_id = 0;
        break;
      }
    }
  }
  for ( i = 0; i < count; i++ ) {
    printf( "processing %s count %d thread_id %ld\n", ( char * )items[ i ]->opt.buffer, count, self() );
  }
  if ( count ) {
    while ( 1 ) {
      load_lock_ctrl( ctrl );
      if ( ctrl->tag_id == self() ) {
        ctrl->tag_id = 0;
        ctrl->job_done = ( ctrl->job_done + count ) % ARRAY_SIZE( ctrl->item );
        break;
      }
    } 
  }
}


#ifdef TEST
static void
job_exec( struct job_ctrl *ctrl, struct job_item *w ) {
  int old_done;

  w->done = 1;
  old_done = ctrl->job_done;

  while ( ctrl->item[ ctrl->job_done ].done && ctrl->job_done != ctrl->job_start ) {
    w = &ctrl->item[ ctrl->job_done ];
    if ( ctrl->item[ ctrl->job_done ].opt.server_socket != 1 ) {
      printf( "job_exec: %s\n", ( char * ) w->opt.buffer );
    }
    work_lock();
    ctrl->job_done = ( ctrl->job_done + 1 ) % ARRAY_SIZE( ctrl->item );
    work_unlock();
  }
  work_lock();
  if ( old_done != ctrl->job_done ) {
    pthread_cond_signal( &cond_write );
  }
  work_unlock();
}
#endif


static void
init_mutexes( void ) {
  pthread_mutex_init( &work_mutex, NULL );
  pthread_cond_init( &cond_add, NULL );
  pthread_cond_init( &cond_write, NULL );
}


void
*run_thread( void *data ) {
  struct job_ctrl *ctrl = data;
  int ret = 1;
  int i = 1;

  while ( 1 ) {
    get_exec_job( ctrl );
    sleep( i );
    i = ( i + 1 ) % 10;
  }
  return ( void * ) ( intptr_t ) ret;
}


int
main()
{
#ifdef TEST
  int server_socket;
  struct sockaddr_un server_addr;


  if ( ( server_socket = socket( AF_UNIX, SOCK_SEQPACKET, 0 ) ) == -1 ) {
    printf( "failed to create a unix socket\n" );
    return -1;
  }
  int ret = fcntl( server_socket, F_SETFL, O_NONBLOCK );
  if ( ret < 0 ) {
    printf( "failed to set socket to non_blocking\n" );
    close( server_socket );
    return -1;
  }
  memset( &server_addr, 0, sizeof( struct sockaddr_un ) );
  server_addr.sun_family = AF_UNIX;
  strcpy( server_addr.sun_path, "/home/nick/test.sock" );

  if ( connect( server_socket, ( struct sockaddr * ) &server_addr, sizeof( struct sockaddr_un ) ) == -1 ) {
    printf( "Connection refused ( sun_path = %s, fd = %d, errno = %s [%d] ).\n",
           server_addr.sun_path, server_socket, strerror( errno ), errno );

    close( server_socket );
    return -1;
  }
  int socket_buffer_size;
  socklen_t optlen = sizeof ( socket_buffer_size );
  if ( getsockopt( server_socket, SOL_SOCKET, SO_SNDBUF, &socket_buffer_size, &optlen ) == -1 ) {
    socket_buffer_size = 0;
  }
#endif
  char buffer[ 128 ];
  char service_name[ MESSENGER_SERVICE_NAME_LEN ];
  uint32_t buffer_len;
  uint32_t i = 0;
  int server_socket = -1;
  pthread_t thread_id[ THREADS ];
  struct job_opt opt;
  struct job_ctrl *ctrl;


  printf("sizeof st_data_t %u\n", sizeof(st_data_t));
  init_mutexes();
  strncpy( service_name, "repeater_hub", MESSENGER_SERVICE_NAME_LEN );
  ctrl = ( struct job_ctrl * ) malloc( sizeof( *ctrl ) );
  memset( ctrl, 0, sizeof( *ctrl ) );
  for ( i = 0; i < THREADS; i++ ) {
    if ( ( pthread_create( &thread_id[ THREADS ], NULL, run_thread, ctrl ) != 0 ) ) {
      printf( "Failed to create thread\n" );
      exit;
    }
  }
  
  while ( 1 ) {
    buffer_len = sprintf( buffer, "header - body %d", i++  );
    opt.buffer = buffer;
    if ( i == 10 ) {
      server_socket = 8;
      update_server_socket( ctrl, service_name, server_socket );
    }
    opt.server_socket = server_socket;
    opt.buffer_len = buffer_len;
    strncpy(opt.service_name, service_name, MESSENGER_SERVICE_NAME_LEN );
printf("adding job %d\n", i);
    add_job( ctrl, &opt );
    sleep( 2 );
  }
  
  return 0;
}
