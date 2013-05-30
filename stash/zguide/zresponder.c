#include "czmq.h"


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  int rc = zsocket_connect( responder, "tcp://localhost:5560" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  while ( true ) {
    char *string = zstr_recv( responder );
    if ( string ) {
      printf( "Received request [ %s ]\n", string );
      zstr_send( responder, string );
      free( string );
    }
    // to think how to pass the callback this thread from main thread.
  }
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
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      zclock_sleep( 1000 );
    }
  }

  return 0;
}
