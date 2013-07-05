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
process_request( void *pipe, zmsg_t *msg ) {
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
    zmsg_send( &msg, pipe );
  }
}


static int
responder_input( void *responder, void *pipe ) {
  zmsg_t *msg = one_or_more_msg( responder );

  size_t nr_frames = zmsg_size( msg );
  size_t frame_size;
  printf( "responder_input %zu\n", nr_frames );
  if ( nr_frames == 1 ) {
    zframe_t *msg_type_frame = zmsg_first( msg );
    self_msg_recv( pipe, msg, msg_type_frame );
  }
  else {
    process_request( pipe, msg );
  }

  return 0;
}


static int
responder_output( void *pipe, void *responder ) {
  zmsg_t *msg = one_or_more_msg( pipe );
  size_t nr_frames = zmsg_size( msg );
  printf( "responder_output %zu\n", nr_frames );

  zmsg_send( &msg, responder );

  return 0;
}


static void
start_responder( void *pipe, void *responder ) {
  while ( 1 ) {
    zmq_pollitem_t items[] = {
      { pipe, 0, ZMQ_POLLIN, 0 },
      { responder, 0, ZMQ_POLLIN, 0 }
    };
    int poll_size = 2;
    int rc = zmq_poll( items, poll_size, -1 );
    if ( rc == -1 ) {
      break;
    }
    if ( items[ 0 ].revents & ZMQ_POLLIN ) {
      rc = responder_output( pipe, responder );
    }
    if ( items[ 1 ].revents & ZMQ_POLLIN ) {
      rc = responder_input( responder, pipe );
    }
    if ( rc == -1 ) {
      break;
    }
  }
  printf( "out of responder\n" );
}


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  emirates_priv *priv = args;
  uint32_t port = priv->responder_port;
  
  void *responder = zsocket_new( ctx, ZMQ_REQ );

  priv->responder_id = ( char * ) zmalloc( sizeof( char ) * RESPONDER_ID_SIZE );
  snprintf( priv->responder_id, RESPONDER_ID_SIZE, "%lld", zclock_time() );
#ifdef TEST
  snprintf( priv->responder_id, RESPONDER_ID_SIZE, "%s", "worker1" );
#endif
  zsocket_set_identity( responder, priv->responder_id );

  int rc = zsocket_connect( responder, "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( pipe );
    return;
  }
  
  send_ok_status( pipe );
  start_responder( pipe, responder );
}


int
responder_init( emirates_priv *priv ) {
  priv->responder_port = RESPONDER_BASE_PORT;
  priv->responder = zthread_fork( priv->ctx, responder_thread, priv );
  if ( check_status( priv->responder ) ) {
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
