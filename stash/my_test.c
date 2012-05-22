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
#define ITEM_SIZE 32
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
  pthread_t ref;
  uint16_t done;
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


int
CAS( void *address, const void *oldvalue, const void *newvalue, size_t len ) {
  int ret = 0;

  if ( !memcmp(address, oldvalue, len ) ) {
    memcpy( address, newvalue, len );
    ret = 1;
  }
  return ret;
}


static void
update_server_socket( struct job_ctrl *ctrl, const char *service_name, const int server_socket ) {
  struct job_item *item;
  int i;
  int end = ctrl->job_end;

  for ( i = 0; i < end; i++ ) {
    item = &ctrl->item[ i ];
    if ( item->opt.server_socket == -1 ) {
      if ( !strncmp( service_name, item->opt.service_name, MESSENGER_SERVICE_NAME_LEN ) ) {
        if ( CAS( &ctrl->item[ i ], item, item, sizeof( *item ) ) ) {
          item->opt.server_socket = server_socket;
        }
      }
    }
  }
}




static int
add_job( struct job_ctrl *ctrl, struct job_item *input_item ) {
  struct job_item *item;
  uint32_t end;
  uint32_t start;
  uint32_t tmp;
  
  while ( 1 ) {
    end = ctrl->job_end;
    item = &ctrl->item[ end ];
    if ( end != ctrl->job_end ) {
      continue;
    }
    start = ctrl->job_start;
    if ( ( end + 1 ) % ARRAY_SIZE( ctrl->item ) == start ) {
      return 0; 
    }
    if ( !item->done ) {
      input_item->ref = self();
      if ( CAS( &ctrl->item[ end ], item, input_item, sizeof( *item ) ) ) {
        tmp = ( end + 1 ) % ARRAY_SIZE( ctrl->item );
        if ( CAS( &ctrl->job_end, &end, &tmp, sizeof( tmp ) ) ) {
          return;
        }
      } 
    }
  }
}


static struct job_item *
get_exec_job( struct job_ctrl *ctrl ) {
  struct job_item *item;
  struct job_item ref;
  int tmp;
  uint32_t start;

  while ( 1 ) {
    start = ctrl->job_start;
    item = &ctrl->item[ start ];   
    if ( start != ctrl->job_start ) {
      continue;
    }
    if ( start == ctrl->job_end ) {
      continue;
    }
    if ( item->opt.server_socket != -1 && !item->done ) {
      memcpy( &ref, item, sizeof( ref ) ); 
      ref.done = 1;
      ref.ref = self();
      if ( CAS( &ctrl->item[ start ], item, &ref, sizeof( ref ) ) ) {
        tmp = ( start + 1 ) % ARRAY_SIZE( ctrl->item ); 
        if ( CAS( &ctrl->job_start, &start, &tmp, sizeof( tmp ) ) ) {
          return item;
        }
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
  struct job_item *item;
  int ret = 1;

  while ( 1 ) {
    item = get_exec_job( ctrl );
    if ( item != NULL ) {
      printf( "data %s address %p %lu\n", ( char * )item->opt.buffer, ( char * )item->opt.buffer, pthread_self() );
      free( item->opt.buffer );
      item->done = 0;
      item->opt.buffer = NULL;
      item->opt.buffer_len = 0;
    }
    sleep( 1 );
  }
  return ( void * ) ( intptr_t ) ret;
}

#define EQUAL( table, x, y ) ( ( x ) == ( y ) || ( *table->type->compare )( ( x ), ( y ) ) == 0 )

#define PTR_NOT_EQUAL( table, ptr, hash_val, key ) \
  ( ( ptr ) != 0 && ( ptr->hash != ( hash_val ) || !EQUAL( ( table ), ( key ), ( ptr )->key ) ) 


#define ADD_DIRECT( table, key, value, hash_val, bin_pos ) do { \
  st_table_entry *entry; \
  if ( table->nr_entries > ST_DEFAULT_MAX_DENSITY * table->nr_bins ) { \
    rehash( table ); \
    bin_pos = hash_val % table->nr_bins; \
  } \
  entry = malloc( st_table_entry ); \
  entry->hash = hash_val; \
  entry->key = key; \
  entry->record = value; \
  entry->next = table->bins[ bin_pos ]; \
  if ( table->head != 0 ) { \
    entry->before = 0; \
    ( entry->back = entry->tail )->before = entry; \
    table->tail = entry; \
  } \
  else { \
    table->head = table->tail = entry; \
    entry->before = entry->back = 0; \
  } \
  table->bins[ bin_pos ] = entry; \
  table->nr_entries++; \
} while ( 0 )
    
 

#define FIND_ENTRY( table, ptr, hash_val, bin_pos ) do { \
  bin_pos = hash_val % ( table )->nr_bins; \
  ptr = ( table )->bins[ bin_pos ]; \
  if ( PTR_NOT_EQUAL( table, ptr, hash_val, key ) ) { \
    while ( PTR_NOT_EQUAL( table, ptr->next, hash_val, key ) ) { \
      ptr = ptr->next; \
    } \
    ptr = ptr->next; \
  } \
} while ( 0 )

typedef struct st_table st_table;

struct st_table {
  int32_t nr_bins;
  int32_t nr_entries : ST_INDEX_BITS - 1;
  struct st_table_entry **bins;
  struct st_table_entry *head;
  struct st_table_entry *tail;
};


typedef struct st_table_entry st_table_entry;

struct st_table_entry {
  int32_t hash;
  int32_t key;
  st_table_entry *next;
  st_table_entry *before;
  st_table_entry *back;
};

st_table *
init_table() {
  st_table *tbl;
  tbl->malloc( sizeof( st_table ) );
  tbl->nr_entries = 0;
  tbl->nr_bins = size;
  tbl->bins = ( st_table_entry ** )xcalloc( size, sizeof( st_table_entry * ) );
  tbl->head = 0;
  tbl->tail = 0;
  return tbl;
}


int
st_insert( st_table *table, st_data_t key, st_data_t value ) {
  st_table_entry *ptr;
  st_index_t hash_val, bin_pos;

  hash_val = key << 1 | 1;
  FIND_ENTRY( table, ptr, hash_val, bin_pos );
  if ( ptr == 0 ) {
    ADD_DIRECT( table, key, value, hash_val, bin_pos );
    return 0;
  }
  else {
    ptr->record = value;
    return 1;
  }
}


int
st_lookup( st_table *table, st_data_t key, st_data_t *value ) {
  st_table_entry *ptr;
  st_index_t hash_val, bin_pos;

  hash_val = key << 1 | 1;
  FIND_ENTRY( table, ptr, hash_val, bin_pos );
  if ( ptr == 0 ) {
    return 0;
  }
  else {
    if ( value != 0 ) {
      *value = ptr->record;
    }
    return 1;
 }
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
  char service_name[ MESSENGER_SERVICE_NAME_LEN ];
  uint32_t buffer_len;
  uint32_t i = 0;
  int server_socket = -1;
  pthread_t thread_id[ THREADS ];
  struct job_item item;
  struct job_ctrl *ctrl;


  printf("sizeof st_data_t %u\n", sizeof(st_data_t));
  init_mutexes();
  strncpy( service_name, "repeater_hub", MESSENGER_SERVICE_NAME_LEN );
  ctrl = ( struct job_ctrl * ) malloc( sizeof( *ctrl ) );
  memset( ctrl, 0, sizeof( *ctrl ) );
  for ( i = 0; i < THREADS; i++ ) {
    if ( ( pthread_create( &thread_id[ i ], NULL, run_thread, ctrl ) != 0 ) ) {
      printf( "Failed to create thread\n" );
      exit;
    }
  }
  
  i = 0;
  while ( 1 ) {
    item.opt.buffer = malloc( 64 );
    buffer_len = sprintf( item.opt.buffer, "header - body %d", i++  );
    item.opt.buffer_len = buffer_len;
    item.ref = 0;
    if ( i == 10 ) {
      server_socket = 8;
      update_server_socket( ctrl, service_name, server_socket );
    }
    item.opt.server_socket = server_socket;
    item.done = 0;
    strncpy(item.opt.service_name, service_name, MESSENGER_SERVICE_NAME_LEN );
    add_job( ctrl, &item );
    sleep( 2 );
  }
  
  return 0;
}
