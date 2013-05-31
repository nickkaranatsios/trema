#include "zhelpers.h"
#include "czmq.h"


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  char *identity = args;
  void *responder = zsocket_new( ctx, ZMQ_REQ );
  printf( "identity of client: %s\n", identity );
  zmq_setsockopt( responder, ZMQ_IDENTITY, identity, strlen( identity ) );

  int rc = zsocket_connect( responder, "ipc://backend.ipc" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  s_send( responder, "READY" ); 

  while ( true ) {
    char *identity = zstr_recv( responder );
    char *empty = zstr_recv( responder );
    assert( *empty == 0 );
    free( empty );
    char *request = zstr_recv( responder );
    if ( request ) {
      printf( "Received request [ %s ]\n", request );
      s_sendmore( responder, identity );
      s_sendmore( responder, "" );
      s_send( responder, request );
      free( identity );
      free( request );
    }
    // to think how to pass the callback this thread from main thread.
  }
}


void *
responder_init( zctx_t *ctx, char *id ) {
  return zthread_fork( ctx, responder_thread, id );
}


int
main( int argc, char **argv ) {
  if ( argc < 2 ) {
    printf( "%s self identity name\n", argv[ 0 ] );
    exit( 1 );
  }
  char *identity = argv[ 1 ];

  zctx_t *ctx;
  ctx = zctx_new();
  void *responder = responder_init( ctx, identity );
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      zclock_sleep( 1000 );
    }
  }

  return 0;
}
