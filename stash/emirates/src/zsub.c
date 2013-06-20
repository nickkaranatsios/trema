#include "czmq.h"

typedef void ( subscriber_callback )( void *args );

typedef struct _subscription_info {
  subscriber_callback *callback;
  const char *pub_endpoint;
  const char *filter;
} subscription_info;


static void
my_subscriber_handler( void *args ) {
  //puts( "inside my_subscriber_handler" );
}



void
subscribe_service_profile( subscriber_callback *callback ) {
  const char *fn_name = __func__;
  printf( "fn_name is %s\n", fn_name );
}


static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  subscription_info *subscription = args;
  void *sub = zsocket_new( ctx, ZMQ_SUB );
  int rc;

  rc = zsocket_connect( sub, "tcp://localhost:6001" );
  // zsockopt_set_subscribe( sub, subscription->filter );
  zsockopt_set_subscribe( sub, "" );


  uint64_t sub_count = 0;
  while ( true ) {
    char *string = zstr_recv( sub );
    if ( string == NULL ) {
      break;              //  Interrupted
    }
    //printf( "subscribe string: %s\n", string );
    subscription->callback( sub );
    sub_count++;
    free( string );
  }
  printf( "subscribe( %" PRIu64 ")\n", sub_count );
}


int
main( int argc, char **argv ) {
#ifdef BENCHMARK
  if ( argc != 2 ) {
    printf( "%s subscription\n", argv[ 0 ] );
    exit( 1 );
  }
#endif
  zctx_t *ctx;
  ctx = zctx_new(); 

  subscription_info *subscription = ( subscription_info * ) zmalloc( sizeof( *subscription ) );
  //subscription->filter = argv[ 1 ];
  subscription->filter = "";
  subscription->callback = my_subscriber_handler;
  subscribe_service_profile( my_subscriber_handler );
  void *sub = zthread_fork( ctx, subscriber_thread, subscription );

  uint64_t end = 2 * 60 * 1000 + zclock_time();
  while ( !zctx_interrupted ) {
    if ( zclock_time() >= end ) {
      break;
    }
#ifdef BENCHMARK
    zclock_sleep( 1000 );
#endif
  }
  zctx_destroy( &ctx );

  return 0; 
}
