#include "czmq.h"



static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  zsocket_set_sndhwm( requester, 10000 );
  zsocket_set_rcvhwm( requester, 10000 );
  int rc = zsocket_connect( requester, "tcp://localhost:6000" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }

  uint64_t request_count = 0;
  uint64_t reply_count = 0;
  uint64_t reply_timeout_count = 0;

  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string != NULL ) {
      if ( string[ 0 ] == '\0' ) {
        zstr_send( requester, "" );
        break;
      }
      // serialize the request
      // send the request 
      zstr_send( requester, string );
      request_count++;
      free( string );
    }
    zmq_pollitem_t poller = { requester, 0, ZMQ_POLLIN, 0 };
    int rc = zmq_poll( &poller, 1, 5 * 1000 );
    if ( rc == -1 ) {
      break;
    }
    if ( rc == 0 ) {
      reply_timeout_count++;
    }
    if ( poller.revents & ZMQ_POLLIN ) {
      char *reply = zstr_recv( requester );
      if ( reply != NULL ) {
        reply_count++;
      }
    }
  }
  printf( "request count( %" PRIu64 ")\n" ,request_count );
  printf( "reply count( %" PRIu64 ")\n" ,reply_count );
  printf( "reply timeout count( %" PRIu64 ")\n" ,reply_timeout_count );
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
  uint64_t pipe_timeout = 0;
  if ( requester != NULL ) {
    while ( !zctx_interrupted ) {
      if ( zclock_time() >= end ) {
        zstr_send( requester, "" );
        zclock_sleep( 2000 );
        break;
      }
      zmq_pollitem_t poller = { requester, 0, ZMQ_POLLOUT, 0 };
      int rc = zmq_poll( &poller, 1, 1000 );
      if ( rc == -1 ) {
        break;
      }
      if ( !rc ) {
        pipe_timeout++;
      }
      if ( poller.revents & ZMQ_POLLOUT ) {
        char string[ 10 ];
        sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
        zstr_send( requester, string );
      }
    }
    printf( "pipe timeout ( %" PRIu64 ")\n" ,pipe_timeout );
  }

  return 0;
}
