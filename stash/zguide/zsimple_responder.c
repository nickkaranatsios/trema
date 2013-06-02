#include "czmq.h"


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  zsocket_set_sndhwm( responder, 10000 );
  zsocket_set_rcvhwm( responder, 10000 );
  int rc = zsocket_bind( responder, "tcp://*:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
    return;
  }

  uint64_t reply_count = 0;
  uint64_t wait_for_request_timeout = 0;
  char *test = NULL;
  char *string;
  do {
    zmq_pollitem_t poller = { responder, 0, ZMQ_POLLIN, 0 };
    int rc = zmq_poll( &poller, 1, 5 * 1000 );
    if ( rc == -1 ) {
      break;
    }
    if ( !rc ) {
      wait_for_request_timeout++;
    }
    if ( poller.revents & ZMQ_POLLIN ) {
      string = zstr_recv( responder );
      test = string;
      if ( string ) {
        if ( string[ 0 ] == '\0' ) {
          test = NULL;
        }
        zstr_send( responder, string );
        reply_count++;
        free( string );
      }
    }
    // to think how to pass the callback this thread from main thread.
  } while ( test != NULL );
  printf( "reply count ( %" PRIu64 ")\n" ,reply_count );
  printf( "wait for request count ( %" PRIu64 ")\n" ,wait_for_request_timeout );
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

  uint64_t end = 3 * 60 * 1000 + zclock_time();
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      zclock_sleep( 2 * 60 * 1000 );
      uint64_t now = zclock_time();
      if ( now >= end ) {
        break;
      }
    }
  }

  return 0;
}
