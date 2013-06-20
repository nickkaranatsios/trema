#include "czmq.h"


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  int rc = zsocket_connect( requester, "tcp://localhost:5559" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }

  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string != NULL ) {
      printf( "request message %s\n", string );
      // serialize the request
      // send the request 
      zstr_send( requester, string );
      free( string );
    }
  }
}


void *
requester_init( zctx_t *ctx ) {
  return zthread_fork( ctx, requester_thread, NULL );
}

int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();
  void *requester = requester_init( ctx );
  if ( requester != NULL ) {
    char string[ 10 ];
    sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
    zstr_send( requester, string );
  }
  zclock_sleep( 500 );

  return 0;
}
