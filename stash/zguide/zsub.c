#include "czmq.h"

typedef void ( subscriber_callback )( void *args );

typedef struct _subscription_info {
  subscriber_callback *callback;
  const char *pub_endpoint;
  const char *filter;
} subscription_info;


static void
my_subscriber_handler( void *args ) {
  puts( "inside my_subscriber_handler" );
}

static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  subscription_info *subscription = args;
  void *sub = zsocket_new( ctx, ZMQ_SUB );
  int rc;

  rc = zsocket_connect( sub, "tcp://localhost:6001" );
  zsockopt_set_subscribe( sub, subscription->filter );

  while ( true ) {
    char *string = zstr_recv( sub );
    if ( string == NULL ) {
      break;              //  Interrupted
    }
    printf( "subscribe string: %s\n", string );
    subscription->callback( sub );
    free( string );
  }
}


int
main( int argc, char **argv ) {
  if ( argc != 2 ) {
    printf( "%s subscription\n", argv[ 0 ] );
    exit( 1 );
  }
  zctx_t *ctx;
  ctx = zctx_new(); 

  subscription_info *subscription = ( subscription_info * ) zmalloc( sizeof( *subscription ) );
  subscription->filter = argv[ 1 ];
  subscription->callback = my_subscriber_handler;
  void *sub = zthread_fork( ctx, subscriber_thread, subscription );

  while ( !zctx_interrupted ) {
    zclock_sleep( 1000 );
  }
  zctx_destroy( &ctx );

  return 0; 
}