//  Weather proxy device

#include "czmq.h"

int main (void) {
  zctx_t *context = zctx_new();

  //  This is where the weather server sits
  void *frontend = zsocket_new( context, ZMQ_XSUB );
  zsocket_connect( frontend, "tcp://localhost:6000");

  //  This is our public endpoint for subscribers
  void *backend = zsocket_new( context, ZMQ_XPUB );
  zsocket_bind( backend, "tcp://*:6001");

  //  Run the proxy until the user interrupts us
  zmq_proxy( frontend, backend, NULL );

  zctx_destroy( &context );

  return 0;
}
