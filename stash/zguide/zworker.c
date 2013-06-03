#include "zhelpers.h"
#include "czmq.h"


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  char *identity = args;
  void *responder = zsocket_new( ctx, ZMQ_REQ );
  printf( "identity of client: %s\n", identity );
  zmq_setsockopt( responder, ZMQ_IDENTITY, identity, strlen( identity ) );
  zsocket_set_sndhwm( responder, 10000 );
  zsocket_set_rcvhwm( responder, 10000 );

  int rc = zsocket_connect( responder, "tcp://localhost:8888" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  s_send( responder, "READY" ); 

  uint64_t read_timeout = 0;
  uint64_t reply_count = 0;
  while ( true ) {
    zmq_pollitem_t poller = { responder, 0, ZMQ_POLLIN, 0 }; 
    rc = zmq_poll( &poller, 1, 1000 );
    if ( rc == -1 ) {
      break;
    }
    if ( !rc ) {
      read_timeout++;
    }
    if ( poller.revents & ZMQ_POLLIN ) {
      char *identity = zstr_recv( responder );
      char *empty = zstr_recv( responder );
      assert( *empty == 0 );
      free( empty );
      char *request = zstr_recv( responder );
      if ( request ) {
        if ( request[ 0 ] == '\0' ) {
          break;
        }
        printf( "Received request [ %s ]\n", request );
        s_sendmore( responder, identity );
        s_sendmore( responder, "" );
        s_send( responder, request );
        free( identity );
        free( request );
        reply_count++;
      }
    }
  }
  printf( "reply count ( %" PRIu64 " )\n" ,reply_count );
  printf( "read timeout count ( %" PRIu64 " )\n" ,read_timeout );
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
    uint64_t end = 10 * 60 * 1000 + zclock_time();
    while ( !zctx_interrupted ) {
      zclock_sleep( 2 * 60 * 1000 );
      if ( zclock_time() >= end ) {
        break;
      }
    }
  }

  return 0;
}
