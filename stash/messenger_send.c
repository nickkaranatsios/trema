/*
 * Trema messenger send library.
 *
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
 *
 * Copyright (C) 2008-2012 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include "event_handler.h"
#include "hash_table.h"
#include "log.h"
#include "messenger.h"
#include "timer.h"
#include "wrapper.h"
#include "messenger_common.h"
#include "message_buffer.h"
#include "messenger_context.h"
#include "messenger_send.h"


// file scope global declarations
static const uint32_t messenger_send_queue_length = MESSENGER_SEND_BUFFER * 4;
static const uint32_t messenger_send_length_for_flush = MESSENGER_SEND_BUFFER;
static const uint32_t messenger_recv_queue_length = MESSENGER_SEND_BUFFER * 2;
static hash_table *send_queues = NULL;
static char socket_directory[ PATH_MAX ];
static const uint32_t messenger_bucket_size = MESSENGER_SEND_BUFFER;
static memory_desc desc[ MAX_MEM_DESC ];


void on_send_write( int fd, void *data );
void on_send_read( int fd, void *data );
static int send_queue_connect_timeout( send_queue *sq );
static uint32_t get_send_data( send_queue *sq, size_t offset );
static FILE *fp = NULL;


static job_ctrl ctrl;
static pthread_t threads[ THREADS ];
static pthread_t main_thread;
#define ITEM_ARRAY_SIZE ARRAY_SIZE( ctrl->item )


static inline void
job_lock() {
  pthread_mutex_lock( &ctrl.mutex );
}


static inline void
job_unlock() {
  pthread_mutex_unlock( &ctrl.mutex );
}


static inline pthread_t
self() {
  return pthread_self();
}


void
init_desc() {
  memory_region *ptr;
  int i;
  uint32_t mult = 0;
  uint32_t next_mult;

  for ( i = 0; i < ARRAY_SIZE( desc ); i++ ) {
    ptr = ( memory_region * )xmalloc( sizeof( memory_region ) );
    if ( ptr != NULL ) {
      ptr->start = mult * MEM_BLOCK;
      next_mult = !mult ? mult + 2 : mult + mult;
      ptr->end =  next_mult * MEM_BLOCK;
      ptr->left = ptr->end - ptr->start;
      ptr->size = ptr->left;
      ptr->head = ptr->tail = xmalloc( ptr->size );
      desc[ i ].mregion = ptr;
      if ( !mult ) {
        mult += 2;
      }
      else {
        mult += mult;
      }
    }
  }
}


void *
get_memory( uint32_t requested_size ) {
  void *address = NULL;
  bool found = false;
  int i;

  for ( i = 0; i < ARRAY_SIZE( desc ) && found == false; i++ ) {
    if ( requested_size < desc[ i ].mregion->size && requested_size < desc[ i ].mregion->left ) {
      address = desc[ i ].mregion->tail;
      desc[ i ].mregion->left -= requested_size;
      desc[ i ].mregion->tail = ( char * ) desc[ i ].mregion->tail + requested_size;
      desc[ i ].mregion->last_accessed = desc[ i ].mregion->tail;
      found = true;
    }
  }
  // reset tail pointer from mregion until found requested size
  if ( address == NULL ) {
    for ( i = 0; i < ARRAY_SIZE( desc ) && found == false; i++ ) {
      desc[ i ].mregion->tail = desc[ i ].mregion->head;
      desc[ i ].mregion->left = desc[ i ].mregion->size;
      if ( requested_size < desc[ i ].mregion->size && requested_size < desc[ i ].mregion->left ) {
        address = desc[ i ].mregion->tail;
        desc[ i ].mregion->left -= requested_size;
        desc[ i ].mregion->tail = ( char * ) desc[ i ].mregion->tail + requested_size;
        desc[ i ].mregion->last_accessed = desc[ i ].mregion->tail;
        found = true;
      }
    }
  }
  return address;
}


void release_memory( void *address, uint32_t release_size ) {
  int i;
  bool found = false;

  for ( i = 0; i < ARRAY_SIZE( desc ) && found == false; i++ ) {
    if ( desc[ i ].mregion->last_accessed == address ) {
      desc[ i ].mregion->left += release_size;
      desc[ i ].mregion->tail = ( char * ) desc[ i ].mregion->tail - release_size;
      found = true;
    }
  }
}

      
int
CAS( void *shared, const void *oldvalue, const void *newvalue, size_t len ) {
  int ret = 0;

  job_lock();
  if ( !memcmp( shared, oldvalue, len ) ) {
    memcpy( shared, newvalue, len );
    ret = 1;
  }
  job_unlock();
  return ret;
}


void
write_job_done( job_ctrl *ctrl, int value, int idx ) {
  int other_tag;
  int other_idx = 0;

  if ( !idx ) {
    other_idx = 1;
  }
  other_tag = ctrl->thread_ctrl[ other_idx ].job_done_tag;
  ctrl->thread_ctrl[ idx ].job_done_value = value;
  if ( !idx ) {
    ctrl->thread_ctrl[ idx ].job_done_tag = 1 - other_tag;
  }
  else {
    ctrl->thread_ctrl[ idx ].job_done_tag = other_tag;
  }
}


int
read_job_done( job_ctrl *ctrl ) {
  int tag0, tag1;
  int value = -1;
  int match_equal_cnt = 0;
  int match_unequal_cnt = 0;

  while( match_equal_cnt < 2 || match_unequal_cnt < 2 ) {
    tag0 = ctrl->thread_ctrl[ 0 ].job_done_tag;
    tag1 = ctrl->thread_ctrl[ 1 ].job_done_tag;
    if ( tag0 == tag1 ) {
      value = ctrl->thread_ctrl[ 1 ].job_done_value;
      match_equal_cnt++;
    }
    else {
      value = ctrl->thread_ctrl[ 0 ].job_done_value;
      match_unequal_cnt++;
    }
  }
  return value;
}


static void
update_server_socket( job_ctrl *ctrl, const char *service_name, const int server_socket ) {
  job_item *item;
  int i;
  int end;

  end = ctrl->job_end;
  for ( i = 0; i < end; i++ ) {
    item = &ctrl->item[ i ];
    if ( !strncmp( service_name, item->opt.service_name, MESSENGER_SERVICE_NAME_LENGTH ) ) {
      if ( CAS( &ctrl->item[ i ], item, item, sizeof( *item ) ) ) {
        item->opt.server_socket = server_socket;
      }
    }
  }
}


static int
job_queue_full( job_ctrl *ctrl ) {
  int end;
  int done;
  
  end = ctrl->job_end;
die( "in job_queue_full done");
  done = read_job_done( ctrl );
  if ( ( end + 1 ) % ITEM_ARRAY_SIZE == done ) {
    return 1;
  }
  return 0;
}


static int
any_data_in_job_queue( job_ctrl *ctrl ) {
  int done;
  int end;

  done = read_job_done( ctrl );
  end = ctrl->job_end;
  if ( done != end ) {
    return 1;
  }
  return 0;
}


/*
 * Adds a job item to the job pool. A job item stores the following fields:
 * server_socket - the socket to send the message to.
 * service_name - a string that identifies a unique service name
 * buffer - an address in memory where the message is copied to
 * buffer_len - the length of buffer bytes
 *
 * @return [void]
 */
static bool
add_lock_free_job( job_ctrl *ctrl, job_item *item ) {
  job_item *end_item;
  int end;
  int tmp;
  bool added = false;

  if ( job_queue_full( ctrl ) ) {
    return added;
  }
  while ( added == false ) {
    end = ctrl->job_end;
    tmp = ( end + 1 ) % ITEM_ARRAY_SIZE;
    end_item = &ctrl->item[ end ];
    if ( item->tag_id == self() ) {
      ctrl->job_end = tmp;
//    if ( CAS( &ctrl->job_end, &end, &tmp, sizeof( tmp ) ) ) {
      memcpy( end_item, item, sizeof( *item ) );
      added = true;
    }
  }
  return added;
}


#ifdef TEST
static job_item *
get_lock_free_job( job_ctrl *ctrl ) {
  job_item *item;
  struct timespec req;
  int tmp;
  int start;

  req.tv_sec = 0;
  req.tv_nsec = 1;
  while ( true ) {
    start = ctrl->job_start;
    item = &ctrl->item[ start ];
    if ( start != ctrl->job_start ) {
      continue;
    }
    if ( start == ctrl->job_end ) {
      nanosleep( &req, NULL );
      continue;
    }
    tmp = ( start + 1 ) % ITEM_ARRAY_SIZE;
    if ( CAS( &ctrl->job_start, &start, &tmp, sizeof( tmp ) ) ) {
      if ( item->opt.server_socket != -1 && item->opt.buffer != NULL ) {
        return item;
      }
    }
  }
}
#endif


static void
get_lock_free_job( job_ctrl *ctrl ) {
  job_item *item;
  struct timespec req;
  int done;
  int tmp;
  int thread_idx = 0;

#ifdef TEST
    done = ctrl->job_done;
    me = self();
    if ( done != ctrl->job_done ) {
      continue;
    }
      if ( me == self() ) {
        ctrl->job_done = tmp;
    // after item
#endif
  req.tv_sec = 0;
  req.tv_nsec = 1;
  if ( self() == threads[ 1 ] ) {
    thread_idx = 1;
  }
  while ( true ) {
    done = read_job_done( ctrl );
    item = &ctrl->item[ done ];
    if ( done == ctrl->job_end ) {
      nanosleep( &req, NULL );
      continue;
    }
    tmp = ( done + 1 ) % ITEM_ARRAY_SIZE;
    if ( item->opt.server_socket != -1 && item->opt.buffer != NULL ) {
      send( item->opt.server_socket, item->opt.buffer, item->opt.buffer_len, MSG_DONTWAIT );
      write_job_done( ctrl, tmp, thread_idx );
    }
  }
}

ssize_t xwrite( int fd, const void *buf, size_t len ) {
  ssize_t nw;

  while ( true ) {
    nw = write( fd, buf, len );
    if ( ( nw < 0 ) && ( errno == EAGAIN || errno == EINTR ) ) {
      continue;
    }
    return nw;
  }
}

 
ssize_t packet_write( int fd, void *buf, size_t len ) {
  struct pollfd pollfd;
  ssize_t nw = -1;
  nfds_t nfd = 1;
  int ret;

  nw = send( fd, buf, len, MSG_DONTWAIT );
  if ( nw < 0 && errno == EAGAIN ) {
    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    ret = poll( &pollfd, nfd, -1 );
    if ( ret > 0 ) {
      if ( pollfd.revents & POLLOUT ) {
        nw = send( fd, buf, len, MSG_DONTWAIT );
      }
    }
  }
  return nw;
}


#ifdef TEST
static void
get_multiple_jobs( job_ctrl *ctrl ) {
  job_item *items[ MAX_TAKE ];
  job_item *first = NULL;
  job_item *last_item = NULL;
  uint32_t total_len = 0;
  int i;
  int done, done_tmp;

  for ( i = 0; i < MAX_TAKE; i++ ) {
    items[ i ] = get_lock_free_job( ctrl );
    first = items[ 0 ];
    if ( items[ i ] == NULL ) {
      break;
    }
    if ( i > 0 && items[ i - 1 ]->opt.server_socket != items[ i ]->opt.server_socket ) {
      last_item = items[ i - 1 ];
      break;
    }
    if ( i > 0 && ( char * )( items[ i - 1 ]->opt.buffer ) + items[ i - 1 ]->opt.buffer_len != items[ i ]->opt.buffer ) {
      last_item = items[ i - 1 ];
      break;
    }
    total_len += items[ i ]->opt.buffer_len;
  }
  done_tmp = i;
  if ( first != NULL ) {
    packet_write( first->opt.server_socket, first->opt.buffer, total_len );
  }
  if ( last_item != NULL ) {
    packet_write( last_item->opt.server_socket, last_item->opt.buffer, last_item->opt.buffer_len );
  }
  while ( true ) {
    done = ctrl->job_done;
    done_tmp = ( done + done_tmp ) % ITEM_ARRAY_SIZE;
    if ( CAS( &ctrl->job_done, &done, &done_tmp, sizeof( done_tmp ) ) ) {
      break;
    }
  }
}
#endif


#ifdef TEST
    fprintf(fp, "start dumping\n");
    for ( i = 0; i < count; i++ ) {
      fprintf(fp, "server_socket %d buffer %p length %d\n", items[i]->opt.server_socket, items[i]->opt.buffer, items[i]->opt.buffer_len);
    }
    fprintf(fp, "end dumping\n");
    fflush(fp);
  }
#endif


#ifdef TEST
void
get_single_job( job_ctrl *ctrl ) {
  job_item *item;
  int done;
  int tmp;

  item = get_lock_free_job( ctrl );
  send( item->opt.server_socket, item->opt.buffer, item->opt.buffer_len, MSG_DONTWAIT );
  do {
    done = ctrl->job_done;
    tmp = ( done + 1 ) % ITEM_ARRAY_SIZE;
  } while ( !CAS( &ctrl->job_done, &done, &tmp, sizeof( tmp ) ) ); 
}
#endif


void 
*run_thread( void *data ) {
  job_ctrl *ctrl = data;
  int ret = 1;
  
  while ( true ) {
    get_lock_free_job( ctrl );
  }
  return ( void * ) ( intptr_t ) ret;
}


/**
 * connects send_queue to the service
 * return value: -1:error, 0:refused (retry), 1:connected
 */
static int
send_queue_connect( send_queue *sq ) {
  assert( sq != NULL );

  sq->running_timer = false;
  if ( ( sq->server_socket = socket( AF_UNIX, SOCK_SEQPACKET, 0 ) ) == -1 ) {
    error( "Failed to call socket ( errno = %s [%d] ).", strerror( errno ), errno );
    return -1;
  }

  if ( geteuid() == 0 ) {
    int wmem_size = 1048576;
    int ret = setsockopt( sq->server_socket, SOL_SOCKET, SO_SNDBUFFORCE, ( const void * ) &wmem_size, ( socklen_t ) sizeof( wmem_size ) );
    if ( ret < 0 ) {
      error( "Failed to set SO_SNDBUFFORCE to %d ( %s [%d] ).", wmem_size, strerror( errno ), errno );
      close( sq->server_socket );
      sq->server_socket = -1;
      return -1;
    }
  }
  int ret = fcntl( sq->server_socket, F_SETFL, O_NONBLOCK );
  if ( ret < 0 ) {
    error( "Failed to set O_NONBLOCK ( %s [%d] ).", strerror( errno ), errno );
    close( sq->server_socket );
    sq->server_socket = -1;
    return -1;
  }

  if ( connect( sq->server_socket, ( struct sockaddr * ) &sq->server_addr, sizeof( struct sockaddr_un ) ) == -1 ) {
    error( "Connection refused ( service_name = %s, sun_path = %s, fd = %d, errno = %s [%d] ).",
           sq->service_name, sq->server_addr.sun_path, sq->server_socket, strerror( errno ), errno );

    send_dump_message( MESSENGER_DUMP_SEND_REFUSED, sq->service_name, NULL, 0 );
    close( sq->server_socket );
    sq->server_socket = -1;
    return 0;
  }
  update_server_socket( &ctrl, sq->service_name, sq->server_socket );

  set_fd_handler( sq->server_socket, on_send_read, sq, &on_send_write, sq->service_name );
  set_readable( sq->server_socket, true );

  error( "Connection established ( service_name = %s, sun_path = %s, fd = %d ).",
         sq->service_name, sq->server_addr.sun_path, sq->server_socket );

  socklen_t optlen = sizeof ( sq->socket_buffer_size );
  if ( getsockopt( sq->server_socket, SOL_SOCKET, SO_SNDBUF, &sq->socket_buffer_size, &optlen ) == -1 ) {
    sq->socket_buffer_size = 0;
  }

  send_dump_message( MESSENGER_DUMP_SEND_CONNECTED, sq->service_name, NULL, 0 );

  return 1;
}


static int
send_queue_connect_timer( send_queue *sq ) {
  if ( sq->server_socket != -1 ) {
    return 1;
  }
  if ( sq->running_timer ) {
    sq->running_timer = false;
    delete_timer_event( ( timer_callback )send_queue_connect_timeout, sq );
  }

  int ret = send_queue_connect( sq );
  struct itimerspec interval;

  switch ( ret ) {
  case -1:
    // Print an error, and find a better way of indicating the send
    // queue has an error.
    sq->reconnect_interval.tv_sec = -1;
    sq->reconnect_interval.tv_nsec = 0;
    update_server_socket( &ctrl, sq->service_name, sq->server_socket );
    return -1;

  case 0:
    // Try again later.
    sq->refused_count++;
    sq->reconnect_interval.tv_sec = ( 1 << ( sq->refused_count > 4 ? 4 : sq->refused_count - 1 ) );

    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_nsec = 0;
    interval.it_value = sq->reconnect_interval;
    add_timer_event_callback( &interval, ( void (*)(void *) )send_queue_connect_timeout, ( void * ) sq );
    sq->running_timer = true;

    debug( "refused_count = %d, reconnect_interval = %u.", sq->refused_count, sq->reconnect_interval.tv_sec );
    return 0;

  case 1:
    // Success.
    sq->refused_count = 0;
    sq->reconnect_interval.tv_sec = 0;
    sq->reconnect_interval.tv_nsec = 0;
    update_server_socket( &ctrl, sq->service_name, sq->server_socket );
    return 1;

  default:
    die( "Got invalid value from send_queue_connect_timer( send_queue* )." );
  };

  return -1;
}


static int
send_queue_connect_timeout( send_queue *sq ) {
  sq->running_timer = false;
  return send_queue_connect_timer( sq );
}


static int
send_queue_try_connect( send_queue *sq ) {
  // TODO: Add a proper check for this.
  if ( sq->reconnect_interval.tv_sec != 0 ) {
    return 0;
  }

  return send_queue_connect_timer( sq );
}


/**
 * creates send_queue and connects to specified service name.
 */
static send_queue *
create_send_queue( const char *service_name ) {
  assert( service_name != NULL );

  debug( "Creating a send queue ( service_name = %s ).", service_name );

  send_queue *sq;

  assert( send_queues != NULL );

  sq = lookup_hash_entry( send_queues, service_name );
  if ( NULL != sq ) {
    warn( "Send queue for %s is already created.", service_name );
    return sq;
  }

  sq = xmalloc( sizeof( send_queue ) );
  memset( sq->service_name, 0, MESSENGER_SERVICE_NAME_LENGTH );
  strncpy( sq->service_name, service_name, MESSENGER_SERVICE_NAME_LENGTH );

  memset( &sq->server_addr, 0, sizeof( struct sockaddr_un ) );
  sq->server_addr.sun_family = AF_UNIX;
  sprintf( sq->server_addr.sun_path, "%s/trema.%s.sock", socket_directory, service_name );
  debug( "Set sun_path to %s.", sq->server_addr.sun_path );

  sq->server_socket = -1;
  sq->buffer = NULL;
  sq->refused_count = 0;
  sq->reconnect_interval.tv_sec = 0;
  sq->reconnect_interval.tv_nsec = 0;
  sq->running_timer = false;
  sq->overflow = 0;
  sq->overflow_total_length = 0;
  sq->socket_buffer_size = 0;
  
  if ( send_queue_try_connect( sq ) == -1 ) {
    xfree( sq );
    error( "Failed to create a send queue for %s.", service_name );
    return NULL;
  }

  insert_hash_entry( send_queues, sq->service_name, sq );
  return sq;
}


static int
my_push_message_to_send_queue( const char *service_name, const uint8_t message_type, const uint16_t tag, const void *data, size_t len ) {

  debug( "Pushing a message to send queue ( service_name = %s, message_type = %#x, tag = %#x, data = %p, len = %u ).",
         service_name, message_type, tag, data, len );

  if ( send_queues == NULL ) {
    error( "All send queues are already deleted or not created yet." );
    return false;
  }

  send_queue *sq = lookup_hash_entry( send_queues, service_name );
  if ( NULL == sq ) {
    sq = create_send_queue( service_name );
    assert( sq != NULL );
  }

  uint32_t length = ( uint32_t ) ( sizeof( message_header ) + len );
  if ( job_queue_full( &ctrl ) ) {
    error( "queue is full1" );
    ++sq->overflow;
    sq->overflow_total_length += length;
    send_dump_message( MESSENGER_DUMP_SEND_OVERFLOW, sq->service_name, NULL, 0 );
    return false;
  }
die("we are here");
  sq->overflow = 0;
  sq->overflow_total_length = 0;

  message_header header;
  header.version = 0;
  header.message_type = message_type;
  header.tag = htons( tag );
  header.message_length = htonl( length );

  job_item item;

  memset( &item, 0, sizeof( item ) );
  strncpy( item.opt.service_name, service_name, MESSENGER_SERVICE_NAME_LENGTH );
  item.opt.server_socket = sq->server_socket;
  item.opt.buffer_len = length;
#ifdef TEST
  item.opt.buffer = get_message_buffer_tail( sq->buffer, item.opt.buffer_len );
  write_message_buffer_at_tail( sq->buffer, &header, sizeof( message_header ), data, len );
  item.opt.buffer = xmalloc( item.opt.buffer_len );
  memcpy( item.opt.buffer, &header, sizeof( message_header ) );
  memcpy( ( char * )item.opt.buffer + sizeof( message_header ), data, len );
#endif
  item.opt.buffer = get_memory( item.opt.buffer_len );
  memcpy( item.opt.buffer, &header, sizeof( message_header ) );
  memcpy( ( char * )item.opt.buffer + sizeof( message_header ), data, len );

debug( "about to add_lock_free_job buffer %x length %d", item.opt.buffer, item.opt.buffer_len );
  if ( !main_thread ) {
    main_thread = self();
  }
  item.tag_id = main_thread;
  if ( add_lock_free_job( &ctrl, &item ) == false ) {
    release_memory( item.opt.buffer, item.opt.buffer_len );
    error( "queue is full2" );
    ++sq->overflow;
    sq->overflow_total_length += length;
    send_dump_message( MESSENGER_DUMP_SEND_OVERFLOW, sq->service_name, NULL, 0 );
    return false;
  }
  if ( sq->server_socket == -1 ) {
    debug( "Tried to send message on closed send queue, connecting..." );

    send_queue_try_connect( sq );
    return true;
  }
  return true;
}


#ifdef TEST
static bool
push_message_to_send_queue( const char *service_name, const uint8_t message_type, const uint16_t tag, const void *data, size_t len ) {
  assert( service_name != NULL );

  debug( "Pushing a message to send queue ( service_name = %s, message_type = %#x, tag = %#x, data = %p, len = %u ).",
         service_name, message_type, tag, data, len );

  message_header header;

  if ( send_queues == NULL ) {
    error( "All send queues are already deleted or not created yet." );
    return false;
  }

  send_queue *sq = lookup_hash_entry( send_queues, service_name );

  if ( NULL == sq ) {
    sq = create_send_queue( service_name );
    assert( sq != NULL );
  }

  header.version = 0;
  header.message_type = message_type;
  header.tag = htons( tag );
  uint32_t length = ( uint32_t ) ( sizeof( message_header ) + len );
  header.message_length = htonl( length );

  if ( message_buffer_remain_bytes( sq->buffer ) < length ) {
    if ( sq->overflow == 0 ) {
      warn( "Could not write a message to send queue due to overflow ( service_name = %s, fd = %u, length = %u ).", sq->service_name, sq->server_socket, length );
    }
    ++sq->overflow;
    sq->overflow_total_length += length;
    send_dump_message( MESSENGER_DUMP_SEND_OVERFLOW, sq->service_name, NULL, 0 );
    return false;
  }
  if ( sq->overflow > 1 ) {
    warn( "Could not write a message to send queue due to overflow ( service_name = %s, fd = %u, count = %u, total length = %" PRIu64 " ).", sq->service_name, sq->server_socket, sq->overflow, sq->overflow_total_length );
  }
  sq->overflow = 0;
  sq->overflow_total_length = 0;

debug("before writting to %x data_len %d", sq->buffer, sq->buffer->data_length);
  write_message_buffer( sq->buffer, &header, sizeof( message_header ) );
  write_message_buffer( sq->buffer, data, len );
debug("writting to %x len %d", sq->buffer, len);

  if ( sq->server_socket == -1 ) {
    debug( "Tried to send message on closed send queue, connecting..." );

    send_queue_try_connect( sq );
    return true;
  }

  set_writable( sq->server_socket, true );
  if ( sq->buffer->data_length > messenger_send_length_for_flush ) {
    on_send_write( sq->server_socket, sq );
  }
  return true;
}
#endif


static bool
_send_message( const char *service_name, const uint16_t tag, const void *data, size_t len ) {
  assert( service_name != NULL );

  debug( "Sending a message ( service_name = %s, tag = %#x, data = %p, len = %u ).",
         service_name, tag, data, len );

  return my_push_message_to_send_queue( service_name, MESSAGE_TYPE_NOTIFY, tag, data, len );
}
bool ( *send_message )( const char *service_name, const uint16_t tag, const void *data, size_t len ) = _send_message;


static bool
_send_request_message( const char *to_service_name, const char *from_service_name, const uint16_t tag, const void *data, size_t len, void *user_data ) {
  assert( to_service_name != NULL );
  assert( from_service_name != NULL );

  debug( "Sending a request message ( to_service_name = %s, from_service_name = %s, tag = %#x, data = %p, len = %u, user_data = %p ).",
         to_service_name, from_service_name, tag, data, len, user_data );

  char *request_data, *p;
  size_t from_service_name_len = strlen( from_service_name ) + 1;
  size_t handle_len = sizeof( messenger_context_handle ) + from_service_name_len;
  messenger_context *context;
  messenger_context_handle *handle;
  bool return_value;

  context = insert_context( user_data );

  request_data = xmalloc( handle_len + len );
  handle = ( messenger_context_handle * ) request_data;
  handle->transaction_id = htonl( context->transaction_id );
  handle->service_name_len = htons( ( uint16_t ) from_service_name_len );
  strcpy( handle->service_name, from_service_name );
  p = request_data + handle_len;
  memcpy( p, data, len );

  return_value = my_push_message_to_send_queue( to_service_name, MESSAGE_TYPE_REQUEST, tag, request_data, handle_len + len );

  xfree( request_data );

  return return_value;
}
bool ( *send_request_message )( const char *to_service_name, const char *from_service_name, const uint16_t tag, const void *data, size_t len, void *user_data ) = _send_request_message;


static bool
_send_reply_message( const messenger_context_handle *handle, const uint16_t tag, const void *data, size_t len ) {
  assert( handle != NULL );

  debug( "Sending a reply message ( handle = [ transaction_id = %#x, service_name_len = %u, service_name = %s ], "
         "tag = %#x, data = %p, len = %u ).",
         handle->transaction_id, handle->service_name_len, handle->service_name, tag, data, len );

  char *reply_data;
  messenger_context_handle *reply_handle;
  bool return_value;

  reply_data = xmalloc( sizeof( messenger_context_handle ) + len );
  reply_handle = ( messenger_context_handle * ) reply_data;
  reply_handle->transaction_id = htonl( handle->transaction_id );
  reply_handle->service_name_len = htons( 0 );
  memcpy( reply_handle->service_name, data, len );

  return_value = my_push_message_to_send_queue( handle->service_name, MESSAGE_TYPE_REPLY, tag, reply_data, sizeof( messenger_context_handle ) + len );

  xfree( reply_data );

  return return_value;
}
bool ( *send_reply_message )( const messenger_context_handle *handle, const uint16_t tag, const void *data, size_t len ) = _send_reply_message;


static bool
_clear_send_queue( const char *service_name ) {
  assert( service_name != NULL );

  debug( "Deleting all messages from send queue ( service_name = %s ).", service_name );

  if ( send_queues == NULL ) {
    error( "All send queues are already deleted or not created yet." );
    return false;
  }

  send_queue *sq = lookup_hash_entry( send_queues, service_name );

  if ( NULL == sq ) {
    error( "Send queue is already deleted or not created yet ( service_name = %s ).", service_name );
    return false;
  }
  if ( NULL == sq->buffer ) {
    error( "Message buffer is already deleted or not created yet ( send_queue = %p, service_name = %s ).",
           sq, service_name );
    return false;
  }

  if ( sq->buffer->data_length > 0 ) {
    set_writable( sq->server_socket, false );
  }

  sq->buffer->head_offset = 0;
  sq->buffer->data_length = 0;

  return true;
}
bool ( *clear_send_queue )( const char *service_name ) = _clear_send_queue;


void
number_of_send_queue( int *connected_count, int *sending_count, int *reconnecting_count, int *closed_count ) {
  assert( connected_count != NULL );
  assert( sending_count != NULL );
  assert( reconnecting_count != NULL );
  assert( closed_count != NULL );

  debug( "Checking queue statuses." );

  hash_iterator iter;
  hash_entry *e;

  *connected_count = 0;
  *sending_count = 0;
  *reconnecting_count = 0;
  *closed_count = 0;

  if ( send_queues == NULL ) {
    error( "All send queues are already deleted or not created yet." );
    return;
  }

  init_hash_iterator( send_queues, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    send_queue *sq = e->value;
    if ( sq->server_socket != -1 ) {
      if ( sq->buffer != NULL ) {
        if ( sq->buffer->data_length == 0 ) {
          ( *connected_count )++;
        }
      }
      else {
        if ( sq->refused_count > 0 ) {
            ( *reconnecting_count )++;
        }
        else {
            ( *sending_count )++;
        }
      }
    }
    else {
        ( *closed_count )++;
    }
  }

  debug( "connected_count = %d, reconnecting_count = %d, sending_count = %d, closed_count = %d.",
         *connected_count, *reconnecting_count, *sending_count, *closed_count );
}


void
delete_send_queue( send_queue *sq ) {
  assert( NULL != sq );

  debug( "Deleting a send queue ( service_name = %s, fd = %d ).", sq->service_name, sq->server_socket );

  if ( sq->server_socket != -1 ) {
    set_readable( sq->server_socket, false );
    delete_fd_handler( sq->server_socket );

    close( sq->server_socket );
  }
  if ( send_queues != NULL ) {
    delete_hash_entry( send_queues, sq->service_name );
  }
  else {
    error( "All send queues are already deleted or not created yet." );
  }
  xfree( sq );
}


void
cancel_threads() {
  void *ret = ( void * ) ( intptr_t ) ( - 1 );
  int i;

  for ( i = 0; i < THREADS; i++ ) {
    pthread_join( threads[ i ], &ret );
  }
  pthread_mutex_destroy( &ctrl.mutex );
  for ( i = 0; i < ARRAY_SIZE( desc ); i++ ) {
    xfree( desc[ i ].mregion->head );
  }
}


void
delete_all_send_queues() {
  hash_iterator iter;
  hash_entry *e;

  debug( "Deleting all send queues ( send_queues = %p ).", send_queues );

  if ( send_queues != NULL ) {
    init_hash_iterator( send_queues, &iter );
    while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
      delete_send_queue( e->value );
    }
    delete_hash( send_queues );
    send_queues = NULL;
  }
  else {
    error( "All send queues are already deleted or not created yet." );
  }
  cancel_threads();
}


void
on_send_read( int fd, void *data ) {
  UNUSED( fd );

  char buf[ 256 ];
  send_queue *sq = ( send_queue* )data;

  if ( recv( sq->server_socket, buf, sizeof( buf ), 0 ) <= 0 ) {
    send_dump_message( MESSENGER_DUMP_SEND_CLOSED, sq->service_name, NULL, 0 );

    set_readable( sq->server_socket, false );
    delete_fd_handler( sq->server_socket );

    close( sq->server_socket );
    sq->server_socket = -1;

    if ( any_data_in_job_queue( &ctrl ) ) {
    // Tries to reconnecting immediately, else adds a reconnect timer.
      send_queue_try_connect( sq );
    }
    else {
      delete_send_queue( sq );
    }
  }
}


static uint32_t
get_send_data( send_queue *sq, size_t offset ) {
  assert( sq != NULL );

  uint32_t bucket_size = messenger_bucket_size;
  if ( sq->socket_buffer_size != 0 ) {
    int used;
    if ( ioctl( sq->server_socket, SIOCOUTQ, &used ) == 0 ) {
      if ( used < sq->socket_buffer_size ) {
        bucket_size = ( uint32_t ) ( sq->socket_buffer_size - used ) << 1;
	      if ( bucket_size > messenger_bucket_size ) {
	        bucket_size = messenger_bucket_size;
	      }
      }
      else {
        bucket_size = 1;
      }
    }
  }

  uint32_t length = 0;
  message_header *header;
  while ( ( sq->buffer->data_length - offset ) >= sizeof( message_header ) ) {
    header = ( message_header * ) ( ( char * ) get_message_buffer_head( sq->buffer ) + offset );
    uint32_t message_length = ntohl( header->message_length );
    assert( message_length != 0 );
    assert( message_length < messenger_recv_queue_length );
    if ( length + message_length > bucket_size ) {
      if ( length == 0 ) {
        length = message_length;
      }
      break;
    }
    length += message_length;
    offset += message_length;
  }
  return length;
}


void
on_send_write( int fd, void *data ) {
  send_queue *sq = ( send_queue* )data;

  assert( sq != NULL );
  assert( fd >= 0 );

error( "on_send_write" );
  debug( "Sending data to remote ( fd = %d, service_name = %s, buffer = %p, data_length = %u ).",
         fd, sq->service_name, get_message_buffer_head( sq->buffer ), sq->buffer->data_length );

  if ( sq->buffer->data_length < sizeof( message_header ) ) {
    set_writable( sq->server_socket, false );
    return;
  }

  void *send_data;
  size_t send_len;
  ssize_t sent_len;
  size_t sent_total = 0;

  while ( ( send_len = get_send_data( sq, sent_total ) ) > 0 ) {
    send_data = ( ( char * ) get_message_buffer_head( sq->buffer ) + sent_total );
    sent_len = send( fd, send_data, send_len, MSG_DONTWAIT );
    if ( sent_len == -1 ) {
      int err = errno;
      if ( err != EAGAIN && err != EWOULDBLOCK ) {
        error( "Failed to send ( service_name = %s, fd = %d, errno = %s [%d] ).",
               sq->service_name, fd, strerror( err ), err );
        send_dump_message( MESSENGER_DUMP_SEND_CLOSED, sq->service_name, NULL, 0 );

        set_readable( sq->server_socket, false );
        set_writable( sq->server_socket, false );
        delete_fd_handler( sq->server_socket );

        close( sq->server_socket );
        sq->server_socket = -1;
        sq->refused_count = 0;

        // Tries to reconnecting immediately, else adds a reconnect timer.
        send_queue_try_connect( sq );
      }
error( "truncate inside error %d", sent_total );
      truncate_message_buffer( sq->buffer, sent_total );
      if ( err == EMSGSIZE || err == ENOBUFS || err == ENOMEM ) {
        warn( "Dropping %u bytes data in send queue ( service_name = %s ).", sq->buffer->data_length, sq->service_name );
        truncate_message_buffer( sq->buffer, sq->buffer->data_length );
      }
      return;
    }
    assert( sent_len != 0 );
    assert( send_len == ( size_t ) sent_len );
    send_dump_message( MESSENGER_DUMP_SENT, sq->service_name, send_data, ( uint32_t ) sent_len );
    sent_total += ( size_t ) sent_len;
  }

  truncate_message_buffer( sq->buffer, sent_total );
  if ( sq->buffer->data_length == 0 ) {
    set_writable( sq->server_socket, false );
  }
}


void 
delete_send_service( const char *service_name ) {
  assert( send_queues != NULL );
  send_queue *sq = lookup_hash_entry( send_queues, service_name );
  if ( sq != NULL ) {
    delete_send_queue( sq );
  }
}


void
init_messenger_send( const char *working_directory ) {
  strcpy( socket_directory, working_directory );
  send_queues = create_hash( compare_string, hash_string );
  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_NORMAL );
  pthread_mutex_init( &ctrl.mutex, &attr );
  init_desc();
}


void
start_messenger_send( void ) {
  pthread_attr_t attr;
  int i;

  pthread_attr_init( &attr ) ;
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
  fp = fopen("test.log", "w");
  for ( i = 0; i < THREADS; i++ ) {
    if ( ( pthread_create( &threads[ i ], &attr, run_thread, &ctrl ) != 0 ) ) {
      die( "Failed to create thread" );
    }
  }
  pthread_attr_destroy( &attr );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
