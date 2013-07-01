#include "emirates.h"


static int
requester_output( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *requester = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  zframe_t *service_frame = zmsg_first( msg );
  zframe_t *service_data = zmsg_next( msg );
  if ( service_data != NULL ) {
    zmsg_send( &msg, requester );
  }
  else {
    return EINVAL;
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
start_requester( void *pipe, void *pub ) {
  zloop_t *loop = zloop_new();
  zmq_pollitem_t poller = { pipe, 0, ZMQ_POLLIN, 0 };
  zloop_poller( loop, &poller, requester_output, pub );
  zloop_start( loop );
}


static void
service_reply( const char *service, emirates_iface *iface, reply_callback *callback ) {
  // save the reply_callback in the map.
  zmsg_t *add_service_reply = zmsg_new();
  zmsg_addstr( add_service_reply, ADD_SERVICE_REPLY_MSG );
  zmsg_addstr( add_service_reply, service );
  zmsg_addstr( add_service_reply, get_requester_id( iface, service ) );
  zmsg_send( iface->requester );
  zmsg_destroy( &add_service_reply );
}


void
set_menu_reply( emirates_iface *iface, reply_callback *callback ) {
  service_reply( "menu", iface, value );
}


static void
service_request( const char *service, emirates_iface *iface, request_callback *callback ) {
  zmsg_t *add_service_request = zmsg_new();
  zmsg_addstr( add_service_request, ADD_SERVICE_REQUEST_MSG );
  zmsg_addstr( add_service_request, service );
  zmsg_addstr( add_service_request, get_replier_id( iface, service ) );
  zmsg_send( iface->replier );
  zmsg_destroy( &add_service_request );
}


void
set_menu_request( emirates_iface *iface, request_callback *callback ) {
  service_request( "menu", iface );
}


void
send_menu_request( emirates_iface *iface, jedex_value *service_data ) {
}


void
send_menu_reply( emirates_iface *iface, jedex_value *service_data ) {
}


void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  uint32_t *port = args;
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  int rc = zsocket_connect( requester, "tcp://localhost:%u", *port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", *port );
    send_ng_status( pipe );
    return;
  }

  send_ok_status( pipe );
  start_requester( pipe, requester );
}


int
requester_init( emirates_priv *priv ) {
  priv->requester = zthread_fork( priv->ctx, requester_thread, &priv->requester_port );
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
