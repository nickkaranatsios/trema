#include "czmq.h"

static uint64_t reply_count = 0;

static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  int rc = zsocket_bind( responder, "tcp://*:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
    return;
  }

  char *string;
  do {
    string = zstr_recv( responder );
    if ( string ) {
      zstr_send( responder, string );
      reply_count++;
      free( string );
    }
    // to think how to pass the callback this thread from main thread.
  } while ( string != NULL );
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

  uint64_t end = 2 * 60 * 1000 + zclock_time();
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      if ( zclock_time() >= end ) {
        break;
      }
    }
  }
  printf( "reply count ( %" PRIu64 ")\n" ,reply_count );

  return 0;
}
