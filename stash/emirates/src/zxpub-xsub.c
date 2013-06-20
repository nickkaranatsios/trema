#include "czmq.h"

int
main( void ) {
  zctx_t *context = zctx_new();

  // this is the public endpoint for publishers
  void *frontend = zsocket_new( context, ZMQ_XSUB );
  zsocket_bind( frontend, "tcp://*:6000" );

  // this is the public endpoint for subscribers
  void *backend = zsocket_new( context, ZMQ_XPUB );
  zsocket_bind( backend, "tcp://*:6001" );

  // Run the proxy until the user interrupts us
  zmq_proxy( frontend, backend, NULL );

  zctx_destroy( &context );

  return 0;
}
