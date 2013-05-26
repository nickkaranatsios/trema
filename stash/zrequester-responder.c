#include "czmq.h"


typedef struct _proxy_t {
  zctx_t *ctx;
  void *frontend;
  void *backend;
  zloop_t *loop;
} proxy_t;


static int
frontend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  printf( "fe recv\n" );
  proxy_t *self = arg;

  zframe_t *fe_frame = zframe_recv( self->frontend );
  if ( fe_frame ) {
    zframe_print( fe_frame, NULL );
printf( "sending the frame to backend %p\n", self->backend );
    zframe_send( &fe_frame, self->backend, 0 );
    zframe_destroy( &fe_frame );
  }

  return 0;
}


static int
backend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  proxy_t *self = arg;
  printf( "be recv\n" );
  zframe_t *be_frame = zframe_recv( self->backend );
  if ( be_frame ) {
    zframe_print( be_frame, NULL );
    zframe_destroy( &be_frame );
  }

  return 0;
}


static void
fe_requester_responder( proxy_t *proxy ) {
  int rc;
  proxy->frontend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->frontend == NULL ) {
    printf( "Failed to create frontend ROUTER socket errno: %d:%s\n", errno, zmq_strerror( errno ) );
  }

  rc = zsocket_bind( proxy->frontend, "tcp://*:5559" );
  if ( rc < 0 ) {
    printf( "Failed to bind ROUTER frontend socket %p errno: %d:%s\n" ,proxy->frontend, errno, zmq_strerror( errno ) );
  }

  proxy->backend = zsocket_new( proxy->ctx, ZMQ_DEALER );
  if ( proxy->backend == NULL ) {
    printf( "Failed to create backend socket errno: %d:%s\n", errno, zmq_strerror( errno ) );
  }

  rc = zsocket_bind( proxy->backend,  "tcp://*:5560" );
  if ( rc < 0 ) {
    printf( "Failed to bind DEALER backend socket %p errno: %d:%s\n", proxy->backend, errno, zmq_strerror( errno ) );
  }
  proxy->loop = zloop_new();
  
  zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN };
  poller.socket = proxy->frontend;
  zloop_poller( proxy->loop, &poller, frontend_recv, proxy );
  poller.socket = proxy->backend;
  zloop_poller( proxy->loop, &poller, backend_recv, proxy );
}


static void
proxy_fe( proxy_t *proxy ) {
  fe_requester_responder( proxy );
}


int
main (int argc, char *argv []) {
  proxy_t *self = ( proxy_t * ) zmalloc( sizeof( *self ) );
  self->ctx = zctx_new();

  proxy_fe( self );

  zloop_start( self->loop );

  zloop_destroy( &self->loop );
  puts( "interrupted" );
  zctx_destroy( &self->ctx );
  free( self );
  
  return 0;
}
