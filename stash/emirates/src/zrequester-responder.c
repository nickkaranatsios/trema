#include "czmq.h"


typedef struct _proxy_t {
  zctx_t *ctx;
  void *frontend;
  void *backend;
  zloop_t *loop;
} proxy_t;


static int
frontend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  proxy_t *self = arg;

  zmq_msg_t message;
  while ( 1 ) {
    zmq_msg_init( &message );
    zmq_msg_recv( &message, poller->socket, 0 );
    int more = zmq_msg_more( &message );
    zmq_msg_send( &message, self->backend, more? ZMQ_SNDMORE: 0 );
    zmq_msg_close( &message );
    if ( !more )
      break;      //  Last message part
  }

  return 0;
}


static int
backend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  proxy_t *self = arg;
  zmq_msg_t message;

  while ( 1 ) {
    zmq_msg_init( &message );
    zmq_msg_recv( &message, poller->socket, 0 );
    int more = zmq_msg_more( &message );
    zmq_msg_send( &message, self->frontend, more? ZMQ_SNDMORE: 0 );
    zmq_msg_close( &message );
    if ( !more )
      break;      //  Last message part
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
  
  zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN, 0 };
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

#ifdef TEST
  zmq_proxy( self->frontend, self->backend, NULL );
#endif
  puts( "interrupted" );
  zctx_destroy( &self->ctx );
  free( self );
  
  return 0;
}
