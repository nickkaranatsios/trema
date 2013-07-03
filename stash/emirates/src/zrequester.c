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
requester_output( void *pipe, void *requester ) {
  zmsg_t *msg = zmsg_recv( pipe );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  printf( "requester_output %u\n", nr_frames );

  if ( nr_frames ) {
    zmsg_send( &msg, requester );
  }
  zmsg_destroy( &msg );

  return 0;
}


static int
requester_input( void *requester, void *pipe ) {
  zmsg_t *msg = zmsg_recv( requester );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  printf( "requester_input %u\n", nr_frames );
  if ( nr_frames == 1 ) {
    zframe_t *msg_type_frame = zmsg_first( msg );
    size_t frame_size = zframe_size( msg_type_frame );
    if ( !msg_is( ADD_SERVICE_REPLY, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
      zmsg_send( &msg, pipe );
    }
  }
  else if ( nr_frames > 1 ) {
    zmsg_send( &msg, pipe );
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
start_requester( void *pipe, void *requester ) {
  while ( 1 ) {
    zmq_pollitem_t items[] = {
      { pipe, 0, ZMQ_POLLIN, 0 },
      { requester, 0, ZMQ_POLLIN, 0 }
    };
    int poll_size = 2;
    int rc = zmq_poll( items, poll_size, -1 );
    if ( rc == -1 ) {
      break;
    }
    if ( items[ 0 ].revents & ZMQ_POLLIN ) {
      rc = requester_output( pipe, requester );
    }
    if ( items[ 1 ].revents & ZMQ_POLLIN ) {
      rc = requester_input( requester, pipe );
    }
    if ( rc == -1 ) {
      break;
    }
  }
  printf( "out of requester\n" );
}


static char *
get_requester_id( emirates_priv *priv ) {
  return priv->requester_id;
}
 



static char *
get_responder_id( emirates_priv *priv ) {
  return priv->responder_id;
}


static int
wait_for_reply( void *socket ) {
  zmsg_t *msg = zmsg_recv( socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 1 );

  zframe_t *msg_type_frame = zmsg_first( msg );
  size_t frame_size = zframe_size( msg_type_frame );
  if ( !msg_is( ADD_SERVICE_REPLY, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
    printf( "ADD_SERVICE_REPLY\n" );
    return 0;
  }
  if ( !msg_is( ADD_SERVICE_REQUEST, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
    return 0;
  }


  return EINVAL;
}


static void
service_request( const char *service, emirates_priv *priv, request_callback *callback ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->responder );
  zmsg_destroy( &msg );
  wait_for_reply( priv->responder );
}


static void
service_reply( const char *service, emirates_priv *priv, reply_callback *callback ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REPLY );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->requester );
  zmsg_destroy( &msg );
  wait_for_reply( priv->requester );
}


static void
send_request( const char *service, emirates_priv *priv ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->requester );
  zmsg_destroy( &msg );
}


void
set_menu_request( emirates_iface *iface, request_callback *callback ) {
  service_request( "menu", iface->priv, callback );
}


void
set_menu_reply( emirates_iface *iface, reply_callback *callback ) {
  service_reply( "menu", iface->priv, callback );
}


void
send_menu_request( emirates_iface *iface ) {
  send_request( "menu", iface->priv );
}


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  emirates_priv *priv = args;
  uint32_t port = priv->requester_port;

  void *requester = zsocket_new( ctx, ZMQ_REQ );

  priv->requester_id = ( char * ) zmalloc( sizeof( char ) * REQUESTER_ID_SIZE );
#ifdef TEST
  snprintf( priv->requester_id, REQUESTER_ID_SIZE, "%p", requester );
#endif
  snprintf( priv->requester_id, REQUESTER_ID_SIZE, "%s", "client1" );
  zsocket_set_identity( requester, priv->requester_id );

  int rc = zsocket_connect( requester, "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( pipe );
    return;
  }

  send_ok_status( pipe );
  start_requester( pipe, requester );
}


int
requester_init( emirates_priv *priv ) {
  priv->requester_port = REQUESTER_BASE_PORT;
  priv->requester = zthread_fork( priv->ctx, requester_thread, priv );
  if ( check_status( priv->requester ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


#ifdef OLD
static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  int rc = zsocket_connect( requester, "tcp://localhost:5559" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }

  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string != NULL ) {
      printf( "request message %s\n", string );
      // serialize the request
      // send the request 
      zstr_send( requester, string );
      free( string );
    }
  }
}


void *
requester_init( zctx_t *ctx ) {
  return zthread_fork( ctx, requester_thread, NULL );
}

int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();
  void *requester = requester_init( ctx );
  if ( requester != NULL ) {
    char string[ 10 ];
    sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
    zstr_send( requester, string );
  }
  zclock_sleep( 500 );

  return 0;
}
#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
