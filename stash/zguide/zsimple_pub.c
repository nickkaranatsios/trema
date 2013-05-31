#include "czmq.h"

static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_bind( pub, "tcp://*:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect to XSUB %d\n", rc );
    return;
  }

  uint64_t pub_count = 0;
  if ( pub != NULL ) {
    while ( true ) {
      char *string = zstr_recv( pipe );
      if ( string == NULL ) {
        break;
      }
      if ( zstr_send( pub, string ) == -1 ) {
        break;              //  Interrupted
      }
      pub_count++;
      free( string );
    }
  }
  printf( "publish( %" PRIu64 ")\n" ,pub_count );
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new(); 

  void *pub = zthread_fork( ctx, publisher_thread, NULL );
  
  uint64_t end = 2 * 60 * 1000 + zclock_time();
  while ( !zctx_interrupted ) {
    if ( zclock_time() >= end ) {
      break;
    }
    char string[ 10 ];

    sprintf( string, "%c-%05d", randof( 10 ) + 'A', randof( 100000 ) );
    zstr_send( pub, string );
  }
  
  zctx_destroy( &ctx );

  return 0; 
}
