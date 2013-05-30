#include "czmq.h"

static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( pub, "tcp://localhost:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect to XSUB %d\n", rc );
    return;
  }

  if ( pub != NULL ) {
    while ( true ) {
      char *string = zstr_recv( pipe );
      if ( string == NULL ) {
        break;
      }
      printf( "publish string %s\n", string );
      if ( zstr_send( pub, string ) == -1 ) {
        break;              //  Interrupted
      }
      free( string );
    }
  }
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new(); 

  void *pub = zthread_fork( ctx, publisher_thread, NULL );
  while ( !zctx_interrupted ) {
    char string[ 10 ];

    sprintf( string, "%c-%05d", randof( 10 ) + 'A', randof( 100000 ) );
    zstr_send( pub, string );
    zclock_sleep( 1000 );
  }
  
  zctx_destroy( &ctx );

  return 0; 
}
