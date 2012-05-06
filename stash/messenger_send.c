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
#include "doubly_linked_list.h"
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


void on_send_write( int fd, void *data );
void my_on_send_write( void *args );
void on_send_read( int fd, void *data );
static int send_queue_connect_timeout( send_queue *sq );
static uint32_t get_send_data( send_queue *sq, size_t offset );


static pthread_t threads[ THREADS ];
static struct work_item todo[ TODO_SIZE ];
static int todo_start;
static int todo_end;
static int todo_done;
static const char *debug_cnt = "";


/* this lock protects all the variables above */
static pthread_mutex_t work_mutex;


static inline void
work_lock( void ) {
  pthread_mutex_lock( &work_mutex );
}


static inline void
work_unlock( void ) {
  pthread_mutex_unlock( &work_mutex );
}


/* signalled when new work is added to todo */
static pthread_cond_t cond_add;


/* signalled when the result of one item is written to socket */
static pthread_cond_t cond_write;


/* signalled when we finished with everything */
static pthread_cond_t cond_result;


static void 
add_work( const struct work_opt *opt ) {

  work_lock();

  while ( ( todo_end + 1 ) % ARRAY_SIZE( todo ) == todo_done ) {
    pthread_cond_wait( &cond_write, &work_mutex );  
  }

  todo[ todo_end ].done = 0;
  memcpy( &todo[ todo_end ].opt, opt, sizeof( *opt ) );
  memcpy( &todo[ todo_end ].buffer, opt->hdr_buf, opt->hdr_len );
  memcpy( &todo[ todo_end ].buffer + opt->hdr_len, opt->data_buf, opt->data_len );
  todo_end = ( todo_end + 1 ) % ARRAY_SIZE( todo );

  pthread_cond_signal( &cond_add );
  work_unlock();
}


static struct work_item *
get_work( void ) {
  struct work_item *ret;

  work_lock();
  debug_cnt = "before timedwait";
  while ( todo_start == todo_end ) {
    pthread_cond_wait( &cond_add, &work_mutex );
#ifdef TEST
  struct timespec now;
  int err = 0;
    clock_gettime( CLOCK_REALTIME, &now );
    now.tv_sec += 1;
    now.tv_nsec = 0;
    err = pthread_cond_timedwait( &cond_add, &work_mutex, &now );
#endif
  }
  debug_cnt = "after timedwait";

  if ( todo_start == todo_end ) {
    ret = NULL;
  }
  else {
    ret = &todo[ todo_start ];
    todo_start = ( todo_start + 1 ) % ARRAY_SIZE( todo );
  }
  work_unlock();
  return ret;
}
  

static void 
work_done( struct work_item *w ) {
  int old_done;
  ssize_t data_out_size;

  work_lock();
  w->done = 1;
  old_done = todo_done;

  for( ; todo[ todo_done ].done && todo_done != todo_start;
    todo_done = ( todo_done + 1 ) % ARRAY_SIZE( todo ) ) {
    w = &todo[ todo_done ];
    data_out_size = send( w->opt.server_socket, w->buffer, w->opt.hdr_len + w->opt.data_len, MSG_DONTWAIT );
    if ( data_out_size < 0 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR ) {
      ;
    }
  }
  if ( old_done != todo_done ) {
    debug_cnt = "cond_write is set";
    pthread_cond_signal( &cond_write );
  }
  if ( todo_done == todo_end ) {
    pthread_cond_signal( &cond_result );
  }

  work_unlock();
}


void 
*run_thread( ) {
  int ret = 1;

  while ( 1 ) {
    struct work_item *w = get_work();
    if ( !w ) {
      continue;
    }
    work_done( w );
  }
  return ( void * ) ( intptr_t ) ret;
}



static void
start_threads( void ) {
  int i;

  pthread_mutex_init( &work_mutex, NULL );
  pthread_cond_init( &cond_add, NULL );
  pthread_cond_init( &cond_write, NULL );
  pthread_cond_init( &cond_result, NULL );
  
  for ( i = 0; i < ARRAY_SIZE( todo ); i++ ) {
    int err;

    err = pthread_create( &threads[ i ], NULL, run_thread, NULL );
    if ( err ) {
      die( "Failed to create thread: %s", strerror( err ) );
    }
  }
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

  set_fd_handler( sq->server_socket, on_send_read, sq, &on_send_write, sq->service_name );
  set_readable( sq->server_socket, true );

  if ( sq->buffer != NULL && sq->buffer->data_length >= sizeof( message_header ) ) {
    struct work_opt opt;
    void *send_data;
    size_t send_len;
    size_t sent_total = 0;

    opt.hdr_len = sizeof( message_header );
    opt.server_socket = sq->server_socket;
    message_header *hdr;
    if ( ( send_len = get_send_data( sq, sent_total ) ) > 0 ) {
      send_data = ( ( char * ) get_message_buffer_head( sq->buffer ) + sent_total );
      hdr = send_data;
      opt.hdr_buf = send_data;
      opt.data_buf = (char * ) send_data + opt.hdr_len;
      opt.data_len = ntohl( hdr->message_length ) - opt.hdr_len;
error( "about to add work ");
      add_work( &opt );
    }
  }

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

  sq->buffer = create_message_buffer( messenger_send_queue_length );

  insert_hash_entry( send_queues, sq->service_name, sq );
  return sq;
}


static int
my_push_message_to_send_queue( const char *service_name, const uint8_t message_type, const uint16_t tag, const void *data, size_t len ) {

  error( "Pushing a message to send queue ( service_name = %s, message_type = %#x, tag = %#x, data = %p, len = %u ).",
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

  message_header header;
  header.version = 0;
  header.message_type = message_type;
  header.tag = htons( tag );
  uint32_t length = ( uint32_t ) ( sizeof( message_header ) + len );
  header.message_length = htonl( length );

#ifdef TEST
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
#endif

  if ( sq->server_socket != -1 ) {
    struct work_opt opt;

    opt.server_socket = sq->server_socket;
    opt.hdr_buf = &header;
    opt.hdr_len = sizeof( message_header );
    opt.data_buf = data;
    opt.data_len = len;
    add_work( &opt );
error( "about to add_work hdr_buf %x data_buf %x", opt.hdr_buf, opt.data_buf );
  } 
  else {
    write_message_buffer( sq->buffer, &header, sizeof( message_header ) );
    write_message_buffer( sq->buffer, data, len );
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
      if ( sq->buffer->data_length == 0 ) {
          ( *connected_count )++;
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

  free_message_buffer( sq->buffer );
  if ( sq->server_socket != -1 ) {
    set_readable( sq->server_socket, false );
    set_writable( sq->server_socket, false );
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
}


void
on_send_read( int fd, void *data ) {
  UNUSED( fd );

  char buf[ 256 ];
  send_queue *sq = ( send_queue* )data;

  if ( recv( sq->server_socket, buf, sizeof( buf ), 0 ) <= 0 ) {
    send_dump_message( MESSENGER_DUMP_SEND_CLOSED, sq->service_name, NULL, 0 );

    set_readable( sq->server_socket, false );
    set_writable( sq->server_socket, false );
    delete_fd_handler( sq->server_socket );

    close( sq->server_socket );
    sq->server_socket = -1;

    // Tries to reconnecting immediately, else adds a reconnect timer.
    if ( sq->buffer->data_length > 0 ) {
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
my_on_send_write( void *args ) {
  struct push_args *pargs = ( struct push_args * )args;
  int fd = pargs->fd;

  send_queue *sq = lookup_hash_entry( send_queues, ( char * ) pargs->data );

  assert( sq != NULL );
  assert( fd >= 0 );

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
debug("truncating message buffer %d length %d offset %d", sent_total, sq->buffer->data_length, sq->buffer->head_offset);
  if ( sq->buffer->data_length == 0 ) {
    set_writable( sq->server_socket, false );
  }
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
  start_threads();
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
