/*
 * Copyright (C) 2013 NEC Corporation
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


#include "emirates.h"


static void
self_msg_recv( void *pipe, zmsg_t *msg, zframe_t *msg_type_frame ) {
  size_t msg_type_frame_size = zframe_size( msg_type_frame );

  if ( !msg_is( READY, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    printf( "READY REPLY\n" );
  }
  if ( !msg_is( ADD_SERVICE_REQUEST, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    printf( "ADD_SERVICE_REQUEST_REPLY\n" );
    zmsg_send( &msg, pipe );
  }
}


static void
process_request( responder_info *self, zmsg_t *msg ) {
  zframe_t *client_id_frame = zmsg_first( msg );
  size_t client_id_frame_size = zframe_size( client_id_frame );
  assert( client_id_frame_size < IDENTITY_MAX );

  char client_id[ IDENTITY_MAX ];
  memcpy( client_id,  ( char * ) zframe_data( client_id_frame ), client_id_frame_size );
  client_id[ client_id_frame_size ] = '\0';

  zframe_t *empty_frame = zmsg_next( msg );
  size_t empty_frame_size = zframe_size( empty_frame );
  assert( empty_frame_size == 0 );

  zframe_t *msg_type_frame = zmsg_next( msg );
  size_t msg_type_frame_size = zframe_size( msg_type_frame );

  if ( !msg_is( REQUEST, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    responder_expiry( self ) = zclock_time() + REPLY_TIMEOUT;
    zmsg_send( &msg, responder_pipe_socket( self ) );
  }
}


static int
responder_input( responder_info *self ) {
  zmsg_t *msg = one_or_more_msg( responder_zmq_socket( self ) );

  size_t nr_frames = zmsg_size( msg );
  size_t frame_size;
  printf( "responder_input %zu\n", nr_frames );
  if ( nr_frames == 1 ) {
    zframe_t *msg_type_frame = zmsg_first( msg );
    self_msg_recv( responder_pipe_socket( self ), msg, msg_type_frame );
  }
  else {
    process_request( self, msg );
  }

  return 0;
}


static int
responder_output( responder_info *self ) {
  zmsg_t *msg = one_or_more_msg( responder_pipe_socket( self ) );
  size_t nr_frames = zmsg_size( msg );
  printf( "responder_output %zu\n", nr_frames );

  responder_expiry( self ) = 0;
  zmsg_send( &msg, responder_zmq_socket( self ) );

  return 0;
}


static void
start_responder( responder_info *self ) {
  while ( 1 ) {
    zmq_pollitem_t items[] = {
      { responder_pipe_socket( self ), 0, ZMQ_POLLIN, 0 },
      { responder_zmq_socket( self ), 0, ZMQ_POLLIN, 0 }
    };
    int poll_size = 2;
    int rc = zmq_poll( items, poll_size, get_time_left( responder_expiry( self ) ) );
    if ( rc == -1 ) {
      break;
    }
    if ( items[ 0 ].revents & ZMQ_POLLIN ) {
      rc = responder_output( self );
    }
    if ( items[ 1 ].revents & ZMQ_POLLIN ) {
      rc = responder_input( self );
    }
    if ( rc == -1 ) {
      break;
    }
    if ( responder_expiry( self ) > zclock_time() ) {
      if ( zclock_time() >= responder_expiry( self ) ) {
        printf( "waiting for reply from application expired\n" );
      }
      responder_expiry( self ) = 0;
    }
  }
  printf( "out of responder\n" );
}


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  responder_info *self = args;
  responder_pipe_socket( self ) = pipe;
  uint32_t port = responder_port( self );
  
  responder_zmq_socket( self ) = zsocket_new( ctx, ZMQ_REQ );

  responder_id( self ) = ( char * ) zmalloc( sizeof( char ) * RESPONDER_ID_SIZE );
  snprintf( responder_id( self ), RESPONDER_ID_SIZE, "%lld", zclock_time() );
#ifdef TEST
  snprintf( responder_id( self ), RESPONDER_ID_SIZE, "%s", "worker1" );
#endif
  zsocket_set_identity( responder_zmq_socket( self ), responder_id( self ) );

  int rc = zsocket_connect( responder_zmq_socket( self ), "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( responder_pipe_socket( self ) );
    return;
  }
  
  send_ok_status( responder_pipe_socket( self ) );
  start_responder( self );
}


int
responder_init( emirates_priv *priv ) {
  priv->responder = ( responder_info * ) zmalloc( sizeof( responder_info ) );
  if ( priv->responder == NULL ) {
    return ENOMEM;
  }
  responder_port( priv->responder ) = RESPONDER_BASE_PORT;
  responder_socket( priv->responder ) = zthread_fork( priv->ctx, responder_thread, priv->responder );
  if ( check_status( responder_socket( priv->responder ) ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
