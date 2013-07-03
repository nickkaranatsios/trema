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


static int
responder_input( void *responder, void *pipe ) {
  zmsg_t *msg = zmsg_recv( responder );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  size_t frame_size;
  printf( "responder_input %u\n", nr_frames );
  if ( nr_frames == 1 ) {
    zframe_t *msg_type_frame = zmsg_first( msg );
    frame_size = zframe_size( msg_type_frame );
    if ( !msg_is( ADD_SERVICE_REQUEST, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
      printf( "ADD_SERVICE_REQUEST_REPLY\n" );
      zmsg_send( &msg, pipe );
      return 0;
    }
  }
  else if ( nr_frames > 1 ) {
    zframe_t *identity_frame = zmsg_first( msg );
    size_t identity_size = zframe_size( identity_frame );
    assert( identity_size < IDENTITY_MAX - 1 );
    char *identity;
    if ( identity_size ) {
      identity = ( char * ) zframe_data( identity_frame );
      printf( "client id %s\n", identity );
    }

    zframe_t *empty_frame = zmsg_next( msg );
    frame_size = zframe_size( empty_frame );
    assert( frame_size == 0 );

    zframe_t *msg_type_frame = zmsg_next( msg );
    size_t frame_size = zframe_size( msg_type_frame );
    if ( !msg_is( REQUEST, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
      // at the moment send a test reply
      zmsg_send( &msg, responder );
    }
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
send_ready( void *socket ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, READY );
  zmsg_send( &msg, socket );
  zmsg_destroy( &msg );
}


static int
responder_output( void *pipe, void *responder ) {
  zmsg_t *msg = zmsg_recv( pipe );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  printf( "responder_output %u\n", nr_frames );

  if ( nr_frames ) {
    zmsg_send( &msg, responder );
  }
  zmsg_destroy( &msg );

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
#ifdef TEST
  snprintf( priv->responder_id, RESPONDER_ID_SIZE, "%p", responder );
#endif
  snprintf( priv->responder_id, RESPONDER_ID_SIZE, "%s", "worker1" );
  zsocket_set_identity( responder, priv->responder_id );

  int rc = zsocket_connect( responder, "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( pipe );
    return;
  }
  
  send_ok_status( pipe );
  //send_ready( responder );
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


#ifdef OLD

static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  int rc = zsocket_connect( responder, "tcp://localhost:5560" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  while ( true ) {
    char *string = zstr_recv( responder );
    if ( string ) {
      printf( "Received request [ %s ]\n", string );
      zstr_send( responder, string );
      free( string );
    }
    // to think how to pass the callback this thread from main thread.
  }
}


void *
responder_init( zctx_t *ctx ) {
  return zthread_fork( ctx, responder_thread, NULL );
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();
  void *responder = responder_init( ctx );
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      zclock_sleep( 1000 );
    }
  }

  return 0;
}
#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
