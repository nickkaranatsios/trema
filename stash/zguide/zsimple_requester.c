#include "czmq.h"
#include "zcommon.c"


static void *
reconnect( zctx_t *ctx, void *socket ) {
  int rc;

  zsocket_destroy( ctx, socket );
  socket = zsocket_new( ctx, ZMQ_REQ );
  rc = zsocket_connect( socket, "tcp://localhost:6000" );
  if ( rc ) {
    printf( "failed to reconnect %s\n", zmq_strerror( errno ) );
    socket = NULL;
  }
  return ( char * ) socket;
}


char *
resend_recv( void *socket, char *string ) {
  int rc;
  int i;
  char *reply = NULL;

  zstr_send( socket, string );
  for ( i = 0; i < 3 && reply == NULL; i++ ) {
    reply = poll_recv( socket );
  }

  return reply;
}
  
    

static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  //zsocket_set_sndhwm( requester, 10000 );
  //zsocket_set_rcvhwm( requester, 10000 );
  //zsocket_set_sndtimeo( requester, 0 );
  int rc = zsocket_connect( requester, "tcp://localhost:6000" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }

#ifdef TEST
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
  printf( "reply timeout count( %" PRIu64 " )\n" ,reply_timeout_count );
#endif

  zclock_sleep( 100 * 1000 );
  uint64_t timeout = 0;
  uint64_t ok = 0;
  uint64_t reply_count = 0;
  int i;
  uint8_t max_retries = 3;
  uint8_t retry_count = 0;
  printf( "start receiving\n" );
  char *reply = NULL;

  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string ) {
      rc = zstr_send( requester, string );
      if ( !rc ) {
        reply = poll_recv( requester );
        if ( reply ) {
printf( "%s\n", reply );
          reply_count++;
          free( reply );
        }
      }
      if ( rc == -1 || reply == NULL ) {
        reply = resend_recv( requester, string );
        while ( reply == NULL ) {
          requester = reconnect( ctx, requester );
          reply = resend_recv( requester, string );
        }
        if ( reply ) { 
          reply_count++;
          free( reply );
        }
        else {
          printf( "discarding string %s\n", string );
        }
      }
    }
  }
  printf( "reply count( %" PRIu64 " )\n" ,reply_count );
  printf( "ok count( %" PRIu64 " ) \n" ,ok );
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

#ifdef TEST
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
#endif
  printf( "rcvhwm %d\n", zsocket_rcvhwm( requester ) );
  printf( "sndhwm %d\n", zsocket_sndhwm( requester ) );
  zsocket_set_sndtimeo( requester, 0 );
  int i;
  for ( i = 0; i < 2050; i++ ) {
    char string[ 32 ];
    sprintf( string, "%c-%09d", randof( 10 ) + 'A', i );
    rc = zstr_send( string );
    if ( rc == -1 ) {
      printf( "send failed %s at %d\n", zmq_strerror( errno ), i );
    }
  }
  printf( "complete send to pipe\n" );
  uint64_t now, after = 2 * 60 * 1000 + zclock_time();
  while ( !zctx_interrupted ) {
    now = zclock_time();
    if ( now >= after ) {
      char string[ 32 ];
      sprintf( string, "%c-%09d", randof( 10 ) + 'A', i++ );
      zstr_send( requester, string );
      after = 2 * 60 * 1000 + now;
    }
    zclock_sleep( 500 );
  }

  return 0;
}
