#include <jansson.h>
#include "czmq.h"
#include "morph.c"
#include "raw_array.c"
#include "raw_string.c"


#define PUB_BASE 6000
#define MAX_PUBS 3


typedef struct _proxy_t {
  zctx_t *ctx;
  void *sub;
  void *pub;
  const char *pub_endpoint;

  void *xsub;
  void *xpub;
  void *listener;

  void *requester;
  void *responder;
  void *frontend;
  void *backend;
  zhash_t *requester_cache;
  zhash_t *responder_cache;
} proxy_t;



typedef void ( subscriber_callback )( void *args );
typedef void ( reply_callback )( void *args );
typedef void ( request_callback )( void *args );

typedef struct _subscription_info {
  subscriber_callback *callback;
  const char *pub_endpoint;
} subscription_info;
  

static void
my_subscriber_handler( void *args ) {
  puts( "inside my_subscriber_handler" );
}


static void
my_reply_handler( void *args ) {
  puts( "inside my_reply_handler" );
}


static void
my_request_handler( void *args ) {
  puts( "inside my_request_handler" );
}

#ifdef X
static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  subscription_info *subscription = args;
  int rc;

  void *sub = zsocket_new( ctx, ZMQ_SUB );
  // connect to all available subscribers.
  rc = zsocket_connect( sub, "%s", subscription->pub_endpoint );
  if ( rc ) {
    printf( "subscriber failed to connect %d\n", rc );
  }
  rc = zsocket_connect( sub, "tcp://localhost:6001" );
  if ( rc ) {
    printf( "subscriber failed to connect %d\n", rc );
  }
  zsockopt_set_subscribe( sub, "B" );
  zsockopt_set_subscribe( sub, "C" );

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
#endif


static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  subscription_info *subscription = args;
  void *sub = zsocket_new( ctx, ZMQ_SUB );
  int rc;

  rc = zsocket_connect( sub, "tcp://localhost:6001" );
  zsockopt_set_subscribe( sub, "B" );
  zsockopt_set_subscribe( sub, "C" );

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


#ifdef X
static void
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  void *pub = NULL;
  void *tmp_sock;
  int port = PUB_BASE;
  int i = 0;

  while ( i < MAX_PUBS || pub == NULL ) {
    tmp_sock = zsocket_new( ctx, ZMQ_PUB );
    rc = zsocket_bind( tmp_sock, "tcp://lo:%d", port );
    i++;
    port++;
    if ( rc < 0 ) {
      zsocket_destroy( ctx, tmp_sock );
      printf( "failed to bind publisher %d\n", rc );
      continue;
    }
    pub = tmp_sock;
  }

  if ( pub != NULL ) {
    zstr_send( pipe, zsocket_last_endpoint( pub ) );
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
      zclock_sleep( 100 );     //  Wait for 1/10th second
    }
  }
}
#endif


static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  void *pub = zsocket_new( ctx, ZMQ_PUB );
#ifdef TEST
  rc = zsocket_bind( pub, "tcp://*:6000" );
  if ( rc < 0 ) {
    printf( "Failed to bind to XSUB %d\n", rc );
    return;
  }
#endif
  rc = zsocket_connect( pub, "tcp://localhost:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect to XSUB %d\n", rc );
    return;
  }

  if ( pub != NULL ) {
    zstr_send( pipe, zsocket_last_endpoint( pub ) );
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
      

static void
proxy_fe_pub_sub( proxy_t *proxy ) {
  proxy->pub = zthread_fork( proxy->ctx, publisher_thread, NULL );
  proxy->pub_endpoint = zstr_recv( proxy->pub );

  subscription_info *subscription = ( subscription_info * ) zmalloc( sizeof( *subscription ) );
  subscription->callback = my_subscriber_handler;
  subscription->pub_endpoint = proxy->pub_endpoint;
  proxy->sub = zthread_fork( proxy->ctx, subscriber_thread, subscription );
}


static int
frontend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  printf( "fe recv\n" );
  return 0;
}


static int
backend_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  printf( "be recv\n" );
  return 0;
}


static void
proxy_fe_requester_responder( proxy_t *proxy ) {
  proxy->frontend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->frontend == NULL ) {
    printf( "Failed to create frontend ROUTER socket errno: %d:%s\n", errno, zmq_strerror( errno ) );
  }
  proxy->backend = zsocket_new( proxy->ctx, ZMQ_DEALER );
  int rc;
  rc = zsocket_bind( proxy->frontend, "tcp://*:5559" );
  if ( rc < 0 ) {
    printf( "Failed to bind ROUTER frontend socket %p errno: %d:%s\n" ,proxy->frontend, errno, zmq_strerror( errno ) );
  }
  rc = zsocket_bind( proxy->backend,  "tcp://*:5560" );
  if ( rc < 0 ) {
    printf( "Failed to bind DEALER backend socket %p errno: %d:%s\n", proxy->backend, errno, zmq_strerror( errno ) );
  }
  zloop_t *loop = zloop_new();
  
  zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN, 0 };
  poller.socket = proxy->frontend;
  zloop_poller( loop, &poller, frontend_recv, proxy );
  poller.socket = proxy->backend;
  zloop_poller( loop, &poller, backend_recv, proxy );
}


static void
proxy_fe( proxy_t *proxy ) {
  proxy_fe_pub_sub( proxy );
  // proxy_fe_requester_responder( proxy );
}


static void
proxy_be( proxy_t *proxy ) {
  int rc;

  proxy->xsub = zsocket_new( proxy->ctx, ZMQ_XSUB );
  rc = zsocket_connect( proxy->xsub, "tcp://localhost:6000" );
  if ( rc ) {
    printf( "xsub failed to connect %d\n", rc );
  }

  proxy->xpub = zsocket_new( proxy->ctx, ZMQ_XPUB );
  rc = zsocket_bind( proxy->xpub, "tcp://*:6001" );
  if ( rc < 0 ) {
    printf( "xpub failed to bind %d\n", rc );
  }
}


static void
listener_thread( void *args, zctx_t *ctx, void *pipe ) {
  //  Print everything that arrives on pipe
  while ( true ) {
    zframe_t *frame = zframe_recv( pipe );
puts( "frame recv" );
    if ( !frame )
      break;              //  Interrupted
    zframe_print( frame, NULL );
    zframe_destroy( &frame );
  }
}


static void
publish_service_module( void *publisher, const char *mname, const char *uri ) {
//  char string[ 256 ];
//  sprintf( string, "%s: %s", "test0", "test1" );
//  zstr_send( publisher, string );
    char string[ 10 ];

    sprintf( string, "%c-%05d", randof( 10 ) + 'B', randof( 100000 ) );
    zstr_send( publisher, string );
}

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


static void
proxy_requester( proxy_t *proxy ) {
  proxy->requester = zthread_fork( proxy->ctx, requester_thread, NULL );
}


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  int rc = zsocket_connect( responder, "tcp://localhost:5560" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  while ( true ) {
    char *string = zstr_recv( responder );
    if ( string ) {
      printf( "Received request [ %s ]\n", string );
      free( string );
    }
    // to think how to pass the callback this thread from main thread.
  }
}

static void
proxy_responder( proxy_t *proxy ) {
  proxy->responder = zthread_fork( proxy->ctx, responder_thread, NULL );
}


static void
set_host_location_reply_callback( zhash_t *cache, reply_callback *callback ) {
  // store the association of service name and callback
  zhash_insert( cache, "host_location", callback );
}


static void
send_host_location_request( proxy_t *proxy ) {
  void *callback = zhash_lookup( proxy->requester_cache, "host_location" );
  if ( callback == NULL ) {
    puts( "No reply callback registered for add_host_location" );
  }
  else {
    printf( "callback = %p\n", callback );
  }
}


static void
set_host_location_request_callback( zhash_t *cache, request_callback *callback ) {
  zhash_insert( cache, "host_location", callback );
}


static json_t *
make_request_message() {
  json_t *root;

  root = json_object();
  json_int_t x = 10;
  json_object_set( root, "usage", json_integer( x ) );

  return root;
}


int
main( int argc, char **argv ) {
  morph_init();
  test_raw_string();
  
  proxy_t *self = ( proxy_t * ) zmalloc( sizeof( *self ) );
  self->ctx = zctx_new();

  proxy_fe( self );
  proxy_requester( self );
  proxy_responder( self );
  int add_once = 0;
//  proxy_be( self );
  
//  self->listener = zthread_fork( self->ctx, listener_thread, NULL );
//  zmq_proxy( self->xsub, self->xpub, self->listener );
  while ( !zctx_interrupted ) {
    if ( !add_once ) {
      self->requester_cache = zhash_new();
      reply_callback *callback = my_reply_handler;
      // client A (requester ) adds a reply callback
      set_host_location_reply_callback( self->requester_cache, callback );

      self->responder_cache = zhash_new();
      request_callback *request_callback = my_request_handler;
      // client B ( responder ) adds a request callback
      set_host_location_request_callback( self->responder_cache, request_callback );
      json_t *msg = make_request_message();
      printf( "msg = %s\n", json_dumps( msg, JSON_ENCODE_ANY | JSON_INDENT( 2 ) ) );
      printf( "morph schema %p\n", morph_schema_string() );
      zstr_send( self->requester, "this is a request test message" );
      add_once = 1;
    }
    publish_service_module( self->pub, "test", "test" );
    zclock_sleep( 1000 );
  }

  puts( "interrupted" );
  zctx_destroy( &self->ctx );
  
  return 0;
}