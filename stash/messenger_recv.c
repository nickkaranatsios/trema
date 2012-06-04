/*
 * Trema messenger receive library.
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
#include "doubly_linked_list.h"
#include "event_handler.h"
#include "hash_table.h"
#include "log.h"
#include "messenger.h"
#include "timer.h"
#include "wrapper.h"
#include "messenger_common.h"
#include "message_buffer.h"
#include "messenger_recv.h"
#include "messenger_dump.h"
#include "messenger_recv.h"
#include "messenger_context.h"


static const uint32_t messenger_recv_queue_length = MESSENGER_RECV_BUFFER * 2;
static const uint32_t messenger_recv_queue_reserved = MESSENGER_RECV_BUFFER;
static hash_table *receive_queues = NULL;
static char socket_directory[ PATH_MAX ];


static void on_accept( int fd, void *data );
static void on_recv( int fd, void *data );


/*
 * closes accepted sockets and listening socket, and releases memories.
 */
static void
delete_receive_queue( void *service_name, void *_rq, void *user_data ) {
  debug( "Deleting a receive queue ( service_name = %s, _rq = %p, user_data = %p ).", service_name, _rq, user_data );

  receive_queue *rq = _rq;
  messenger_socket *client_socket;
  dlist_element *element;
  receive_queue_callback *cb;

  assert( rq != NULL );
  for ( element = rq->message_callbacks->next; element; element = element->next ) {
    cb = element->data;
    debug( "Deleting a callback ( function = %p, message_type = %#x ).", cb->function, cb->message_type );
    xfree( cb );
  }
  delete_dlist( rq->message_callbacks );

  for ( element = rq->client_sockets->next; element; element = element->next ) {
    client_socket = element->data;

    debug( "Closing a client socket ( fd = %d ).", client_socket->fd );

    set_readable( client_socket->fd, false );
    delete_fd_handler( client_socket->fd );

    close( client_socket->fd );
    xfree( client_socket );
    send_dump_message( MESSENGER_DUMP_RECV_CLOSED, rq->service_name, NULL, 0 );
  }
  delete_dlist( rq->client_sockets );

  set_readable( rq->listen_socket, false );
  delete_fd_handler( rq->listen_socket );

  close( rq->listen_socket );
  free_message_buffer( rq->buffer );
  unlink( rq->listen_addr.sun_path );

  if ( receive_queues != NULL ) {
    delete_hash_entry( receive_queues, rq->service_name );
  }
  else {
    error( "All receive queues are already deleted or not created yet." );
  }
  xfree( rq );
}


void
delete_all_receive_queues() {
  debug( "Deleting all receive queues ( receive_queues = %p ).", receive_queues );

  if ( receive_queues != NULL ) {
    foreach_hash( receive_queues, delete_receive_queue, NULL );
    delete_hash( receive_queues );
    receive_queues = NULL;
  }
  else {
    error( "All receive queues are already deleted or not created yet." );
  }
}


static receive_queue *
create_receive_queue( const char *service_name ) {
  assert( service_name != NULL );
  assert( strlen( service_name ) < MESSENGER_SERVICE_NAME_LENGTH );

  debug( "Creating a receive queue (service_name = %s).", service_name );

  assert( receive_queues != NULL );
  receive_queue *rq = lookup_hash_entry( receive_queues, service_name );
  if ( rq != NULL ) {
    warn( "Receive queue for %s is already created.", service_name );
    return rq;
  }

  rq = xmalloc( sizeof( receive_queue ) );
  memset( rq->service_name, 0, MESSENGER_SERVICE_NAME_LENGTH );
  strncpy( rq->service_name, service_name, MESSENGER_SERVICE_NAME_LENGTH );

  memset( &rq->listen_addr, 0, sizeof( struct sockaddr_un ) );
  rq->listen_addr.sun_family = AF_UNIX;
  sprintf( rq->listen_addr.sun_path, "%s/trema.%s.sock", socket_directory, service_name );
  debug( "Set sun_path to %s.", rq->listen_addr.sun_path );

  rq->listen_socket = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
  if ( rq->listen_socket == -1 ) {
    error( "Failed to call socket (errno = %s [%d]).", strerror( errno ), errno );
    xfree( rq );
    return NULL;
  }

  unlink( rq->listen_addr.sun_path ); // FIXME: handle error correctly

  int ret;
  ret = bind( rq->listen_socket, ( struct sockaddr * ) &rq->listen_addr, sizeof( struct sockaddr_un ) );
  if ( ret == -1 ) {
    error( "Failed to bind (fd = %d, sun_path = %s, errno = %s [%d]).",
           rq->listen_socket, rq->listen_addr.sun_path, strerror( errno ), errno );
    close( rq->listen_socket );
    xfree( rq );
    return NULL;
  }

  ret = listen( rq->listen_socket, SOMAXCONN );
  if ( ret == -1 ) {
    error( "Failed to listen (fd = %d, sun_path = %s, errno = %s [%d]).",
           rq->listen_socket, rq->listen_addr.sun_path, strerror( errno ), errno );
    close( rq->listen_socket );
    xfree( rq );
    return NULL;
  }

  ret = fcntl( rq->listen_socket, F_SETFL, O_NONBLOCK );
  if ( ret < 0 ) {
    error( "Failed to set O_NONBLOCK ( %s [%d] ).", strerror( errno ), errno );
    close( rq->listen_socket );
    xfree( rq );
    return NULL;
  }

  set_fd_handler( rq->listen_socket, on_accept, rq, NULL, NULL );
  set_readable( rq->listen_socket, true );

  rq->message_callbacks = create_dlist();
  rq->client_sockets = create_dlist();
  rq->buffer = create_message_buffer( messenger_recv_queue_length );

  insert_hash_entry( receive_queues, rq->service_name, rq );

  return rq;
}


static bool
add_message_callback( const char *service_name, uint8_t message_type, void *callback ) {
  assert( receive_queues != NULL );
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Adding a message callback (service_name = %s, message_type = %#x, callback = %p).",
         service_name, message_type, callback );

  receive_queue *rq = lookup_hash_entry( receive_queues, service_name );
  if ( rq == NULL ) {
    debug( "No receive queue found. Creating." );
    rq = create_receive_queue( service_name );
    if ( rq == NULL ) {
      error( "Failed to create a receive queue." );
      return false;
    }
  }

  receive_queue_callback *cb = xmalloc( sizeof( receive_queue_callback ) );
  cb->message_type = message_type;
  cb->function = callback;
  insert_after_dlist( rq->message_callbacks, cb );

  return true;
}


static bool
_add_message_received_callback( const char *service_name, const callback_message_received callback ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Adding a message received callback (service_name = %s, callback = %p).",
         service_name, callback );

  return add_message_callback( service_name, MESSAGE_TYPE_NOTIFY, callback );
}
bool ( *add_message_received_callback )( const char *service_name, const callback_message_received function ) = _add_message_received_callback;


static bool
_add_message_requested_callback( const char *service_name,
                                 void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Adding a message requested callback ( service_name = %s, callback = %p ).",
         service_name, callback );

  return add_message_callback( service_name, MESSAGE_TYPE_REQUEST, callback );
}
bool ( *add_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) ) = _add_message_requested_callback;


static bool
_add_message_replied_callback( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Adding a message replied callback ( service_name = %s, callback = %p ).",
         service_name, callback );

  return add_message_callback( service_name, MESSAGE_TYPE_REPLY, callback );
}
bool ( *add_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) ) = _add_message_replied_callback;


static bool
delete_message_callback( const char *service_name, uint8_t message_type, void ( *callback ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Deleting a message callback ( service_name = %s, message_type = %#x, callback = %p ).",
         service_name, message_type, callback );

  if ( receive_queues == NULL ) {
    debug( "All receive queues are already deleted or not created yet." );
    return false;
  }

  receive_queue *rq = lookup_hash_entry( receive_queues, service_name );
  receive_queue_callback *cb;

  if ( NULL != rq ) {
    dlist_element *e;
    for ( e = rq->message_callbacks->next; e; e = e->next ) {
      cb = e->data;
      if ( ( cb->function == callback ) && ( cb->message_type == message_type ) ) {
        debug( "Deleting a callback ( message_type = %#x, callback = %p ).", message_type, callback );
        xfree( cb );
        delete_dlist_element( e );
        if ( rq->message_callbacks->next == NULL ) {
          debug( "No more callback for message_type = %#x.", message_type );
          delete_receive_queue( rq->service_name, rq, NULL );
        }
        return true;
      }
    }
  }

  error( "No registered message callback found." );

  return false;
}


static bool
_delete_message_received_callback( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Deleting a message received callback ( service_name = %s, callback = %p ).",
         service_name, callback );

  return delete_message_callback( service_name, MESSAGE_TYPE_NOTIFY, callback );
}
bool ( *delete_message_received_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len ) ) = _delete_message_received_callback;


static bool
_delete_message_requested_callback( const char *service_name,
  void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Deleting a message requested callback ( service_name = %s, callback = %p ).",
         service_name, callback );

  return delete_message_callback( service_name, MESSAGE_TYPE_REQUEST, callback );
}
bool ( *delete_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) ) = _delete_message_requested_callback;


static bool
_delete_message_replied_callback( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) ) {
  assert( service_name != NULL );
  assert( callback != NULL );

  debug( "Deleting a message replied callback ( service_name = %s, callback = %p ).",
         service_name, callback );

  return delete_message_callback( service_name, MESSAGE_TYPE_REPLY, callback );
}
bool ( *delete_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) ) = _delete_message_replied_callback;


static bool
_rename_message_received_callback( const char *old_service_name, const char *new_service_name ) {
  assert( old_service_name != NULL );
  assert( new_service_name != NULL );
  assert( receive_queues != NULL );

  debug( "Renaming a message received callback ( old_service_name = %s, new_service_name = %s ).",
         old_service_name, new_service_name );

  receive_queue *old_rq = lookup_hash_entry( receive_queues, old_service_name );
  receive_queue *new_rq = lookup_hash_entry( receive_queues, new_service_name );
  dlist_element *element;
  receive_queue_callback *cb;

  if ( old_rq == NULL ) {
    error( "No receive queue for old service name ( %s ) found.", old_service_name );
    return false;
  }
  else if ( new_rq != NULL ) {
    error( "Receive queue for new service name ( %s ) is already created.", new_service_name );
    return false;
  }

  for ( element = old_rq->message_callbacks->next; element; element = element->next ) {
    cb = element->data;
    add_message_callback( new_service_name, cb->message_type, cb->function );
  }

  delete_receive_queue( old_rq->service_name, old_rq, NULL );

  return true;
}
bool ( *rename_message_received_callback )( const char *old_service_name, const char *new_service_name ) = _rename_message_received_callback;


static void
add_recv_queue_client_fd( receive_queue *rq, int fd ) {
  assert( rq != NULL );
  assert( fd >= 0 );

  debug( "Adding a client fd to receive queue ( fd = %d, service_name = %s ).", fd, rq->service_name );

  messenger_socket *socket;

  socket = xmalloc( sizeof( messenger_socket ) );
  socket->fd = fd;
  insert_after_dlist( rq->client_sockets, socket );

  set_fd_handler( fd, on_recv, rq, NULL, NULL );
  set_readable( fd, true );
}


static void
on_accept( int fd, void *data ) {
  receive_queue *rq = ( receive_queue* )data;

  assert( rq != NULL );

  int client_fd;
  struct sockaddr_un addr;

  socklen_t addr_len = sizeof( struct sockaddr_un );

  if ( ( client_fd = accept( fd, ( struct sockaddr * ) &addr, &addr_len ) ) == -1 ) {
    error( "Failed to accept ( fd = %d, errno = %s [%d] ).", fd, strerror( errno ), errno );
    return;
  }

  if ( geteuid() == 0 ) {
    int rmem_size = 1048576;
    int ret = setsockopt( client_fd, SOL_SOCKET, SO_RCVBUFFORCE, ( const void * ) &rmem_size, ( socklen_t ) sizeof( rmem_size ) );
    if ( ret < 0 ) {
      error( "Failed to set SO_RCVBUFFORCE to %d ( %s [%d] ).", rmem_size, strerror( errno ), errno );
      close( client_fd );
      return;
    }
  }
  int ret = fcntl( client_fd, F_SETFL, O_NONBLOCK );
  if ( ret < 0 ) {
    error( "Failed to set O_NONBLOCK ( %s [%d] ).", strerror( errno ), errno );
    close( client_fd );
    return;
  }

  add_recv_queue_client_fd( rq, client_fd );
  send_dump_message( MESSENGER_DUMP_RECV_CONNECTED, rq->service_name, NULL, 0 );
}


static int
del_recv_queue_client_fd( receive_queue *rq, int fd ) {
  assert( rq != NULL );
  assert( fd >= 0 );

  messenger_socket *socket;
  dlist_element *element;

  debug( "Deleting a client fd from receive queue ( fd = %d, service_name = %s ).", fd, rq->service_name );

  for ( element = rq->client_sockets->next; element; element = element->next ) {
    socket = element->data;
    if ( socket->fd == fd ) {
      set_readable( fd, false );
      delete_fd_handler( fd );

      debug( "Deleting fd ( %d ).", fd );
      delete_dlist_element( element );
      xfree( socket );
      return 1;
    }
  }

  return 0;
}


/**
 * pulls message data from recv_queue.
 * returns 1 if succeeded, otherwise 0.
 */
static int
pull_from_recv_queue( receive_queue *rq, uint8_t *message_type, uint16_t *tag, void *data, size_t *len, size_t maxlen ) {
  assert( rq != NULL );
  assert( message_type != NULL );
  assert( tag != NULL );
  assert( data != NULL );
  assert( len != NULL );

  debug( "Pulling a message from receive queue ( service_name = %s ).", rq->service_name );

  message_header *header;

  if ( rq->buffer->data_length < sizeof( message_header ) ) {
    debug( "Queue length is smaller than a message header ( queue length = %u ).", rq->buffer->data_length );
    return 0;
  }

  header = ( message_header * ) get_message_buffer_head( rq->buffer );

  uint32_t length = ntohl( header->message_length );
  assert( length != 0 );
  assert( length < messenger_recv_queue_length );
  if ( rq->buffer->data_length < length ) {
    debug( "Queue length is smaller than message length ( queue length = %u, message length = %u ).",
           rq->buffer->data_length, length );
    return 0;
  }

  *message_type = header->message_type;
  *tag = ntohs( header->tag );
  *len = length - sizeof( message_header );
  memcpy( data, header->value, *len > maxlen ? maxlen : *len );
  truncate_message_buffer( rq->buffer, length );

  debug( "A message is retrieved from receive queue ( message_type = %#x, tag = %#x, len = %u, data = %p ).",
         *message_type, *tag, *len + sizeof( message_header), header );

  return 1;
}


static void
call_message_callbacks( receive_queue *rq, const uint8_t message_type, const uint16_t tag, void *data, size_t len ) {
  assert( rq != NULL );

  dlist_element *element;
  receive_queue_callback *cb;

  debug( "Calling message callbacks ( service_name = %s, message_type = %#x, tag = %#x, data = %p, len = %u ).",
         rq->service_name, message_type, tag, data, len );

  for ( element = rq->message_callbacks->next; element; element = element->next ) {
    cb = element->data;
    if ( cb->message_type != message_type ) {
      continue;
    }
    switch ( message_type ) {
    case MESSAGE_TYPE_NOTIFY:
      {
        void ( *received_callback )( uint16_t tag, void *data, size_t len );
        received_callback = cb->function;

        debug( "Calling a callback ( %p ) for MESSAGE_TYPE_NOTIFY (%#x) ( tag = %#x, data = %p, len = %u ).",
               cb->function, message_type, tag, data, len );

        received_callback( tag, data, len );
      }
      break;
    case MESSAGE_TYPE_REQUEST:
      {
        void ( *requested_callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len );
        messenger_context_handle *handle;
        char *requested_data;
        size_t header_len;

        requested_callback = cb->function;
        handle = ( messenger_context_handle * ) data;
        handle->transaction_id = ntohl( handle->transaction_id );
        handle->service_name_len = ntohs( handle->service_name_len );
        header_len = sizeof( messenger_context_handle ) + handle->service_name_len;
        requested_data = ( ( char * ) data ) + header_len;

        debug( "Calling a callback ( %p ) for MESSAGE_TYPE_REQUEST (%#x) ( handle = %p, tag = %#x, requested_data = %p, len = %u ).",
               cb->function, message_type, handle, tag, requested_data, len - header_len );

        requested_callback( handle, tag, ( void * ) requested_data, len - header_len );
      }
      break;
    case MESSAGE_TYPE_REPLY:
      {
        debug( "Calling a callback ( %p ) for MESSAGE_TYPE_REPLY (%#x).", cb->function, message_type );

        void ( *replied_callback )( uint16_t tag, void *data, size_t len, void *user_data );
        messenger_context_handle *reply_handle;
        messenger_context *context;

        replied_callback = cb->function;
        reply_handle = data;
        reply_handle->transaction_id = ntohl( reply_handle->transaction_id );
        reply_handle->service_name_len = ntohs( reply_handle->service_name_len );

        context = get_context( reply_handle->transaction_id );

        if ( NULL != context ) {
          debug( "tag = %#x, data = %p, len = %u, user_data = %p.",
                 tag, reply_handle->service_name, len - sizeof( messenger_context_handle ), context->user_data );
          replied_callback( tag, reply_handle->service_name, len - sizeof( messenger_context_handle ), context->user_data );
          delete_messenger_context( context );
        }
        else {
          warn( "No context found." );
        }
      }
      break;
    default:
      error( "Unknown message type ( %#x ).", message_type );
      assert( 0 );
    }
  }
}


static void
on_recv( int fd, void *data ) {
  receive_queue *rq = ( receive_queue* )data;

  assert( rq != NULL );
  assert( fd >= 0 );

  debug( "Receiving data from remote ( fd = %d, service_name = %s ).", fd, rq->service_name );

  uint8_t buf[ MESSENGER_RECV_BUFFER ];
  ssize_t recv_len;
  size_t buf_len;
  uint8_t message_type;
  uint16_t tag;

  while ( ( buf_len = message_buffer_remain_bytes( rq->buffer ) ) > messenger_recv_queue_reserved ) {
    if ( buf_len > sizeof( buf ) ) {
      buf_len = sizeof( buf );
    }
    recv_len = recv( fd, buf, buf_len, 0 );
    if ( recv_len == -1 ) {
      if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
        error( "Failed to recv ( fd = %d, errno = %s [%d] ).", fd, strerror( errno ), errno );
        send_dump_message( MESSENGER_DUMP_RECV_CLOSED, rq->service_name, NULL, 0 );
        del_recv_queue_client_fd( rq, fd );
        close( fd );
      }
      else {
        debug( "Failed to recv ( fd = %d, errno = %s [%d] ).", fd, strerror( errno ), errno );
      }
      break;
    }
    else if ( recv_len == 0 ) {
      debug( "Connection closed ( fd = %d, service_name = %s ).", fd, rq->service_name );
      send_dump_message( MESSENGER_DUMP_RECV_CLOSED, rq->service_name, NULL, 0 );
      del_recv_queue_client_fd( rq, fd );
      close( fd );
      break;
    }
    
    if ( !write_message_buffer( rq->buffer, buf, ( size_t ) recv_len ) ) {
      warn( "Could not write a message to receive queue due to overflow ( service_name = %s, len = %u ).", rq->service_name, recv_len );
      send_dump_message( MESSENGER_DUMP_RECV_OVERFLOW, rq->service_name, buf, ( uint32_t ) recv_len );
    }
    else {
      debug( "Pushing a message to receive queue ( service_name = %s, len = %u ).", rq->service_name, recv_len );
      send_dump_message( MESSENGER_DUMP_RECEIVED, rq->service_name, buf, ( uint32_t ) recv_len );
    }
  }

  while ( pull_from_recv_queue( rq, &message_type, &tag, buf, &buf_len, sizeof( buf ) ) == 1 ) {
    call_message_callbacks( rq, message_type, tag, buf, buf_len );
  }
}


void
init_messenger_recv( const char *working_directory ) {
  // TODO replace strcpy with strncpy
  strcpy( socket_directory, working_directory );
  receive_queues = create_hash( compare_string, hash_string );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
