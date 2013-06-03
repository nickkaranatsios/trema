#include "czmq.h"
#include "zhelpers.h"
#include "zcommon.c"


#define ONE_SEC 1000


static void
set_sock_identity( void *socket ) {
  char identity[ 10 ];
  sprintf( identity, "%c-%05d", randof( 10 ) + 'A', getpid() );
  printf( "identity of client: %s\n", identity );
  zmq_setsockopt( socket, ZMQ_IDENTITY, identity, strlen( identity ) );
}
  

static int
multi_msg_send( void *socket, char *identity, char *string ) {
  s_sendmore( socket, identity );
  s_sendmore( socket, "" );
  return s_send( socket, string );
}


char *
resend_recv( void *socket, char *identity, char *string ) {
  int rc;
  int i;
  char *reply = NULL;

  multi_msg_send( socket, identity, string );
  for ( i = 0; i < 3 && reply == NULL; i++ ) {
    reply = poll_recv( socket );
  }

  return reply;
}


void *
reconnect( zctx_t *ctx, void *socket ) {
  int rc;

  zsocket_destroy( ctx, socket );
  socket = zsocket_new( ctx, ZMQ_REQ );
  set_sock_identity( socket );
  rc = zsocket_connect( socket, "tcp://localhost:7777" );
  if ( rc ) {
    socket = NULL;
    printf( "failed to connect requester\n" );
  }
  return ( char * ) socket;
}


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  char *peer_identity = args;
  void *requester = zsocket_new( ctx, ZMQ_REQ );
  set_sock_identity( requester );
  
  // zsocket_set_sndhwm( requester, 100 );
  //zsocket_set_rcvhwm( requester, 100 );
  //zsocket_set_sndtimeo( requester, 0 );
  int rc = zsocket_connect( requester, "tcp://localhost:7777" );
  if ( rc ) {
    printf( "failed to connect requester\n" );
    return;
  }
  zclock_sleep( 10 * 1000 );

  uint64_t reply_count = 0;
  uint64_t reply_timeout_count = 0;
  uint64_t nr_messages = 0;
  char *reply;
  while ( true ) {
    char *string = zstr_recv( pipe );
    if ( string != NULL ) {
      if ( string[ 0 ] == '\0' ) {
        multi_msg_send( requester, peer_identity, string );
        free( string );
        break;
      }
      // printf( "request message %s\n", string );
      // serialize the request
      // send the request 
      rc = multi_msg_send( requester, peer_identity, string );
      reply = poll_recv( requester );
      if ( reply ) {
        printf( "reply %s\n", reply );
        reply_count++;
        free( reply );
      }
      if ( rc == -1 || reply == NULL ) {
        reply_timeout_count++;
        reply = resend_recv( requester, peer_identity, string );
        while ( reply == NULL ) {
          requester = reconnect( ctx, requester );
          reply = resend_recv( requester, peer_identity, string );
        }
        if ( reply ) {
          reply_count++;
          free( reply );
        }
      }
      free( string );
    }
  }

  printf( "reply count( %" PRIu64 ")\n" ,reply_count );
  printf( "reply timeout count( %" PRIu64 ")\n" ,reply_timeout_count );
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
  // send 2000 messages to pipe wait for 10 seconds before reading from pipe
  // then sleep indefinitely
  int i;
  int rc;
  for ( i = 0; i < 2000; i++ ) {
    char string[ 32 ];
    sprintf( string, "%c-%09d", randof( 10 ) + 'A', i );
    zstr_send( requester, string );
  }
  printf( "send complete\n" );

  while ( !zctx_interrupted ) {
    zclock_sleep( 100 * ZMQ_POLL_MSEC );
  }
#ifdef TEST
  uint64_t pipe_timeout = 0;
  uint64_t send_count = 1;
  if ( requester != NULL ) {
    uint64_t end = 2 * 60 * 1000 * ZMQ_POLL_MSEC + zclock_time();
    while ( !zctx_interrupted ) {
      if ( zclock_time() >= end ) {
        zstr_send( requester, "" );
        zclock_sleep( 2000 * ZMQ_POLL_MSEC );
        break;
      }
      char string[ 32 ];
      //sprintf( string, "%c-%05d", randof( 10 ) + 'A', getpid() );
      sprintf( string, "%c-%09llu", randof( 10 ) + 'A', send_count++ );
      int rc = zstr_send( requester, string );
      if ( rc == -1 ) {
        pipe_timeout++;
        printf( "pipe out full %s\n", strerror( errno ) );
      }
    }
    printf( "pipe timeout ( %" PRIu64 ")\n" ,pipe_timeout );
  }
#endif

  return 0;
}
