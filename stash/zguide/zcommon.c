#include "czmq.h"

#define POLL_DFLT 1

#define poll_recv( socket ) \
  recv_reply( ( socket ), POLL_DFLT )

#define poll_recv_timeout( socket, timeout ) \
  recv_reply( ( socket ), ( timeout ) ) 


char *
recv_reply( void *socket, int timeout ) {
  int rc;
  char *reply = NULL;

  zmq_pollitem_t poller = { socket, 0, ZMQ_POLLIN, 0 };
  rc = zmq_poll( &poller, 1, timeout * 1000 );
  if ( rc ) {
    if ( poller.revents & ZMQ_POLLIN ) {
      reply = zstr_recv( socket );
    }
  }

  return reply;
}
