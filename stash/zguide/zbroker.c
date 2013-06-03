#include "czmq.h"
#include "zhelpers.h"


struct broker {
  void *frontend;
  void *backend;
  char *workers[ 10 ];
  int nr_workers;
  int round_robin;
};


static int
frontend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  struct broker *broker = arg;
  if ( !broker->nr_workers ) {
    return 0;
  }

  char *client_id = s_recv( poller->socket );
  char *empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );

  char *peer_id = s_recv( poller->socket );
  //printf( "peer id %s\n", peer_id );
  empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );
  
  char *request = s_recv( poller->socket );
  //printf( "Frontend request %s\n", request );

  int i;
  for ( i = 0; i < broker->nr_workers; i++ ) {
    if ( !strcmp( peer_id, broker->workers[ i ] ) ) {
      s_sendmore( broker->backend, broker->workers[ i ] );
    } 
  } 
  // s_sendmore( broker->backend, peer_id );
#ifdef ROUND_ROBIN
  // sends message in round robin fashion
  s_sendmore( broker->backend, broker->workers[ broker->round_robin ] );
  broker->round_robin = ( broker->round_robin + 1 ) % broker->nr_workers;
#endif
  s_sendmore( broker->backend, "" );
  s_sendmore( broker->backend, client_id );
  s_sendmore( broker->backend, "" );
  s_send( broker->backend, request ); 
  free( client_id );
  free( request );

  return 0;
}


static int
backend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  struct broker *broker = arg;
  char *empty;

  char *worker_id = s_recv( poller->socket );
  int i;
  bool found = false;
  for ( i = 0; i < broker->nr_workers; i++ ) {
    if ( !strcmp( broker->workers[ i ], worker_id ) ) {
      found = true;
    }
  }
  if ( found == false ) {
    broker->workers[ broker->nr_workers++ ] = worker_id;
  } 
  //printf( "worker_id %s\n", worker_id );

  empty = s_recv( poller->socket );
  assert( empty[ 0 ] == 0 );
  free( empty );

  char *client_id = s_recv( poller->socket );
  if ( strcmp( client_id, "READY" ) != 0 ) {
    empty = s_recv( poller->socket );
    assert( empty[ 0 ] == 0 );
    free( empty );
    char *reply = s_recv( poller->socket );
    s_sendmore( broker->frontend, client_id );
    s_sendmore( broker->frontend, "" );
    s_send( broker->frontend, reply );
    free( reply );
  }

  return 0;
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();

  struct broker *broker = ( struct broker * ) zmalloc( sizeof( *broker ) );
  memset( broker, 0, sizeof( *broker ) );
  broker->frontend = zsocket_new( ctx, ZMQ_ROUTER );
  broker->backend = zsocket_new( ctx, ZMQ_ROUTER );
  int rc;

  rc = zsocket_bind( broker->frontend, "tcp://*:7777" );
  if ( rc < 0 ) {
    printf( "Failed to bind frontent socket %d\n", rc );
    exit( 1 );
  }
  rc = zsocket_bind( broker->backend, "tcp://*:8888" );
  if ( rc < 0 ) {
    printf( "Failed to bind backend socket %d\n", rc );
    exit( 1 );
  }
  zloop_t *loop;
  loop = zloop_new();
  zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN, 0 };
  poller.socket = broker->backend;
  zloop_poller( loop, &poller, backend_recv, broker );
  poller.socket = broker->frontend;
  zloop_poller( loop, &poller, frontend_recv, broker );

  zloop_start( loop );
  zloop_destroy( &loop );

  return 0;
}

