/*
 * Copyright (C) 2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "emirates.h"


static sub_callback *
lookup_subscription_service( zlist_t *sub_callbacks, const char *service ) {
  size_t list_size = zlist_size( sub_callbacks );

  for ( size_t i = 0; i < list_size; i++ ) {
    sub_callback *item = zlist_next( sub_callbacks );
    if ( !strcmp( item->service, service ) ) {
      return item;
    }
  }

  return NULL;
}


static int
subscription_callback_add( emirates_iface *iface, sub_callback *cb ) {
  if ( iface->priv->sub_callbacks == NULL ) {
    iface->priv->sub_callbacks = zlist_new();
  }

  size_t list_size = zlist_size( iface->priv->sub_callbacks );
  int rc = 0;
  sub_callback *item = lookup_subscription_service( iface->priv->sub_callbacks, cb->service );
  if ( !item ) { 
    rc = zlist_append( iface->priv->sub_callbacks, cb );
  }
  else {
    item->callback = cb->callback;
  }

  return rc;
}


static void
subscribe_to_service( const char *service, emirates_iface *iface, const char **sub_schema_names, subscriber_callback *user_callback ) {
  sub_callback *cb = ( sub_callback * ) zmalloc( sizeof( sub_callback ) );
  if ( cb == NULL ) {
    return;
  }
  cb->callback = user_callback;
  cb->service = service;
  if ( *sub_schema_names ) {
    int nr_sub_schema_names = 0;
    while ( *( sub_schema_names + nr_sub_schema_names ) ) {
      nr_sub_schema_names++;
    }
    size_t schema_size = sizeof( const char * ) * nr_sub_schema_names;
    cb->sub_schema_names = ( const char ** ) zmalloc( schema_size );
    for ( int i = 0; i < nr_sub_schema_names; i++ ) {
      if ( !i ) {
        cb->schema = jedex_initialize( sub_schema_names[ i ] );
        continue;
      }
      *( cb->sub_schema_names + i ) = sub_schema_names[ i ];
    }
  }
    
  if ( !subscription_callback_add( iface, cb ) ) {
    zmsg_t *set_subscription = zmsg_new();
    zmsg_addstr( set_subscription, SUBSCRIPTION_MSG );
    zmsg_addstr( set_subscription, service );
    zmsg_send( &set_subscription, iface->priv->sub );
    zmsg_destroy( &set_subscription );
  }
  else {
    free( cb->sub_schema_names );
    free( cb );
  }
}


static int
subscriber_data_handler( zmq_pollitem_t *poller, void *arg ) {
  emirates_iface *iface = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }
  
  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  int rc = 0;

  zframe_t *sub_frame = zmsg_first( msg );
  if ( sub_frame != NULL ) {
    // match subscription and send to application
    sub_callback *item = lookup_subscription_service( iface->priv->sub_callbacks, ( const char * ) zframe_data( sub_frame ) );  
    if ( item != NULL ) {
      zframe_t *service_frame = zmsg_next( msg );
      const char *json_data = ( const char * ) zframe_data( service_frame );
      json_to_jedex_value( item->schema, item->sub_schema_names, json_data );
      
      item->callback( service_frame );
    }
  }
  zmsg_destroy( &msg );

  return rc;
}


static int
subscriber_data_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *pipe = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  int rc = zmsg_send( &msg, pipe );
  if ( rc != 0 ) {
    log_err( "Failed to send subscribed topic to parent thread" );
  }
  zmsg_destroy( &msg );

  return 0;
}


static int
subscriber_ctrl_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *sub = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }
  
  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  int rc = 0;

  zframe_t *sub_frame = zmsg_first( msg );
  if ( zframe_streq( sub_frame, SUBSCRIPTION_MSG ) && zframe_more( sub_frame ) ) {
    zframe_t *subscription_frame = zmsg_next( msg );
    if ( subscription_frame ) {
      zsockopt_set_subscribe( sub, ( char * ) zframe_data( subscription_frame ) );
    }
  }
  else {
   if ( zframe_streq( sub_frame, EXIT_MSG ) ) {
     rc = EINVAL;
   }
  }
  zmsg_destroy( &msg );

  return rc;
}


static void
start_subscribing( void *sub, void *pipe ) {
  zloop_t *sub_loop = zloop_new();

  zmq_pollitem_t poller = { sub, 0, ZMQ_POLLIN, 0 };
  // wait for any matched publishing data
  zloop_poller( sub_loop, &poller, subscriber_data_recv, pipe );
  poller.socket = pipe;
  zloop_poller( sub_loop, &poller, subscriber_ctrl_recv, sub );

  zloop_set_verbose( sub_loop, true );
  int rc = zloop_start( sub_loop );
  if ( rc == -1 ) {
    log_err( "Subscriber child thread stopped" );
  }
  send_ng_status( pipe );
}


static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  uint32_t *port = args;
  int rc;

  void *sub = zsocket_new( ctx, ZMQ_SUB );
  rc = zsocket_connect( sub, "tcp://localhost:%u", *port );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XPUB %d %u", rc, *port );
    send_ng_status( pipe );
    return;
  }
  send_ok_status( pipe );
  start_subscribing( sub, pipe );
}


void
subscribe_service_profile( emirates_iface *iface, const char **sub_schema_names, subscriber_callback *user_callback ) {
  subscribe_to_service( "service_profile", iface, sub_schema_names, user_callback );
}


void
subscribe_user_profile( emirates_iface *iface, const char **sub_schema_names, subscriber_callback *user_callback ) {
  subscribe_to_service( "user_profile", iface, sub_schema_names, user_callback );
}


int
subscriber_init( emirates_priv *priv ) {
  priv->sub = zthread_fork( priv->ctx, subscriber_thread, &priv->sub_port );
  if( check_status( priv->sub ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }
  priv->sub_handler = subscriber_data_handler;
  priv->sub_callbacks = NULL;

  return 0;
}


#ifdef OLD
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
#endif

