#include "czmq.h"


static uint64_t request_count = 0;
static uint64_t reply_count = 0;

static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  int rc = zsocket_connect( requester, "tcp://localhost:6000" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }

  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string != NULL ) {
      // serialize the request
      // send the request 
      zstr_send( requester, string );
      request_count++;
      free( string );
    }
    char *reply = zstr_recv( requester );
    if ( reply != NULL ) {
      reply_count++;
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

  uint64_t end = 2 * 60 * 1000 + zclock_time();
  if ( requester != NULL ) {
    while ( !zctx_interrupted ) {
      if ( zclock_time() >= end ) {
        break;
      }
      char string[ 10 ];
      sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
      zstr_send( requester, string );
    }
  }
  printf( "request count( %" PRIu64 ")\n" ,request_count );
  printf( "reply count( %" PRIu64 ")\n" ,reply_count );

  return 0;
}
