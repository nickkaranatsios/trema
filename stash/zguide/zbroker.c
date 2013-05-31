#include "czmq.h"
#include "zhelpers.h"


static int
frontend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *backend = arg;

  char *client_id = s_recv( poller->socket );
  char *empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );

  char *peer_id = s_recv( poller->socket );
  printf( "peer id %s\n", peer_id );
  empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );
  
  char *request = s_recv( poller->socket );
  printf( "Frontend request %s\n", request );

  s_sendmore( backend, peer_id );
  s_sendmore( backend, "" );
  s_sendmore( backend, client_id );
  s_sendmore( backend, "" );
  s_send( backend, request ); 
  free( client_id );
  free( request );

  return 0;
}


static int
backend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *frontend = arg;
  char *empty;

  char *worker_id = s_recv( poller->socket );
  printf( "worker_id %s\n", worker_id );

  empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );

  char *client_id = s_recv( poller->socket );
  if ( strcmp( client_id, "READY" ) != 0 ) {
    empty = s_recv( poller->socket );
    assert( empty[ 0 ] == 0 );
    free( empty );
    char *reply = s_recv( poller->socket );
     s_sendmore( frontend, client_id );
     s_sendmore( frontend, "" );
     s_send( frontend, reply );
     free( reply );
  }

  return 0;
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();

  void *frontend = zsocket_new( ctx, ZMQ_ROUTER );
  void *backend = zsocket_new( ctx, ZMQ_ROUTER );
  int rc;

  rc = zsocket_bind( frontend, "ipc://frontend.ipc" );
  if ( rc < 0 ) {
    printf( "Failed to bind frontent socket %d\n", rc );
    exit( 1 );
  }
  rc = zsocket_bind( backend, "ipc://backend.ipc" );
  if ( rc < 0 ) {
    printf( "Failed to bind backend socket %d\n", rc );
    exit( 1 );
  }
  zloop_t *loop;
  loop = zloop_new();
  zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN, 0 };
  poller.socket = backend;
  zloop_poller( loop, &poller, backend_recv, frontend );
  poller.socket = frontend;
  zloop_poller( loop, &poller, frontend_recv, backend );

  zloop_start( loop );
  zloop_destroy( &loop );

  return 0;
}

