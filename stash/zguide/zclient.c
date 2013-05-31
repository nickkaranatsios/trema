#include "czmq.h"
#include "zhelpers.h"


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  char *peer_identity = args;
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  char identity[ 10 ];
  sprintf( identity, "%c-%05d", randof( 10 ) + 'A', getpid() );
  printf( "identity of client: %s\n", identity );
  zmq_setsockopt( requester, ZMQ_IDENTITY, identity, strlen( identity ) );
  
  int rc = zsocket_connect( requester, "ipc://frontend.ipc" );
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
      s_sendmore( requester, peer_identity );
      s_sendmore( requester, ""  );
      s_send( requester, string );
      free( string );
    }
    char *reply = zstr_recv( requester );
    if ( reply != NULL ) {
      printf( "reply message %s\n", reply );
    }
  }
}


void *
requester_init( zctx_t *ctx, char *peer_identity ) {
  return zthread_fork( ctx, requester_thread, peer_identity );
}


int
main( int argc, char **argv ) {
  if ( argc < 2 ) {
    printf( "%s peer identity name\n", argv[ 0 ] );
    exit( 1 );
  }
  char *peer_identity = argv[ 1 ];

  zctx_t *ctx;
  ctx = zctx_new();
  void *requester = requester_init( ctx, peer_identity );
  if ( requester != NULL ) {
    while ( !zctx_interrupted ) {
      char string[ 10 ];
      sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
      zstr_send( requester, string );
      zclock_sleep( 1000 );
    }
  }

  return 0;
}
