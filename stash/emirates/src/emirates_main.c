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


#define STATUS_MSG "STATUS:"
#define STATUS_NG  "NG"
#define STATUS_OK "OK"
#define SUBSCRIPTION_MSG "SET_SUBSCRIPTION"
#define EXIT_MSG "EXIT"
#define POLL_TIMEOUT 500


static void
send_status( void *socket, const char *status ) {
  zmsg_t *msg = zmsg_new();
  assert( msg );
  zmsg_addstr( msg, STATUS_MSG );
  zmsg_addstr( msg, status );
  zmsg_send( &msg, socket );
  zmsg_destroy( &msg );
}


static void
send_ng_status( void *socket ) {
  send_status( socket, STATUS_NG );
}


static void
send_ok_status( void *socket ) {
  send_status( socket, STATUS_OK );
}
  

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
publish_output( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *pub = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  //zmsg_t *dup_msg = zmsg_dup( msg );

  zframe_t *service_frame = zmsg_first( msg );
  zframe_t *service_data = zmsg_next( msg );
  if ( service_data != NULL ) {
    zmsg_send( &msg, pub );
  }
  else {
    return EINVAL;
  }
  zmsg_destroy( &msg );
  //zmsg_destroy( &dup_msg );

  return 0;
}


static void
start_publishing( void *pipe, void *pub ) {
  zloop_t *loop = zloop_new();
  zmq_pollitem_t poller = { pipe, 0, ZMQ_POLLIN, 0 };
  zloop_poller( loop, &poller, publish_output, pub );
  zloop_start( loop );
}
 

static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  uint32_t *port = args;

  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( pub, "tcp://localhost:%u", *port );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XSUB %d", rc );
    send_ng_status( pipe );
    return;
  }

  send_ok_status( pipe );
  start_publishing( pipe, pub );
}


static int
subscriber_handler( zmq_pollitem_t *poller, void *arg ) {
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
      zframe_t *service_data = zmsg_next( msg );
      item->callback( iface->priv );
    }
  }
  zmsg_destroy( &msg );

  return rc;
}


static int
subscriber_parent_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
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


static int
subscriber_child_recv( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
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


static void
start_subscribing( void *sub, void *pipe ) {
  zloop_t *sub_loop = zloop_new();

  zmq_pollitem_t poller = { sub, 0, ZMQ_POLLIN, 0 };
  // wait for any matched publishing data
  zloop_poller( sub_loop, &poller, subscriber_child_recv, pipe );
  poller.socket = pipe;
  zloop_poller( sub_loop, &poller, subscriber_parent_recv, sub );

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


static int
check_status( void *socket ) {
  zmsg_t *status_msg = zmsg_recv( socket );
  
  if ( status_msg == NULL ) {
    return EINVAL;
  }
  size_t nr_frames = zmsg_size( status_msg );
  assert( nr_frames == 2 );
  bool status_ok = false;

  zframe_t *status_frame = zmsg_first( status_msg );
  if ( zframe_streq( status_frame, STATUS_MSG ) ) {
    zframe_t *what = zmsg_next( status_msg );
    if ( zframe_streq( what, STATUS_OK ) ) {
      status_ok = true;
    }
  }
  zmsg_destroy( &status_msg );

  return status_ok == false ? EINVAL : 0;
}




emirates_iface *
emirates_initialize( void ) {
  emirates_iface *iface = ( emirates_iface * ) zmalloc( sizeof( emirates_iface ) );
  iface->priv = ( emirates_priv * ) zmalloc( sizeof( emirates_priv ) );

  zctx_t *ctx;
  ctx = zctx_new();
  if ( ctx == NULL ) {
    return NULL;
  }
  iface->priv->ctx = ctx;
  iface->priv->pub_port = PUB_BASE_PORT;
  iface->priv->pub = zthread_fork( iface->priv->ctx, publisher_thread, &iface->priv->pub_port );
  if ( check_status( iface->priv->pub ) ) {
    zctx_destroy( &iface->priv->ctx );
    return NULL;
  }
    
  iface->priv->sub_port = SUB_BASE_PORT;
  iface->priv->sub = zthread_fork( iface->priv->ctx, subscriber_thread, &iface->priv->sub_port );
  if( check_status( iface->priv->sub ) ) {
    zctx_destroy( &iface->priv->ctx );
    return NULL;
  }
  iface->priv->sub_handler = subscriber_handler;
  iface->priv->sub_callbacks = NULL;
  
  return iface;
}


void
emirates_finalize( emirates_iface **iface ) {
  assert( iface );

  if ( *iface ) {
    assert( ( *iface )->priv );
    
    zctx_destroy( &( *iface )->priv->ctx );
    *iface = NULL;
  }
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
subscribe_to_service( const char *service, emirates_iface *iface, subscriber_callback *user_callback ) {
  sub_callback *cb = ( sub_callback * ) zmalloc( sizeof( sub_callback ) );
  if ( cb == NULL ) {
    return;
  }
  cb->callback = user_callback;
  cb->service = service;
  subscription_callback_add( iface, cb );
  
  emirates_priv *priv = iface->priv;

  zmsg_t *set_subscription = zmsg_new();
  zmsg_addstr( set_subscription, SUBSCRIPTION_MSG );
  zmsg_addstr( set_subscription, service );
  zmsg_send( &set_subscription, priv->sub );
  zmsg_destroy( &set_subscription );
  
  return;
}


static char *
any_value_to_json( jedex_value *val ) {
  char *json;

  jedex_value_to_json( val, 1, &json );

  return json;
}


static void
publish_service( const char *service, emirates_iface *iface, jedex_parcel *parcel ) {
  emirates_priv *priv = iface->priv;

  for ( list_element *e = parcel->values_list->head; e != NULL; e = e->next ) {
    jedex_value *item = e->data;
    char *json;
    jedex_value_to_json( item, 1, &json );
    if ( json ) {
      zmsg_t *msg = zmsg_new();
      zmsg_addmem( msg, service, strlen( service ) );
      zmsg_addmem( msg, json, strlen( json ) );
      zmsg_send( &msg, priv->pub );
      zmsg_destroy( &msg );
    }
  }

  return;
}


void
subscribe_service_profile( emirates_iface *iface, subscriber_callback *user_callback ) {
  subscribe_to_service( "service_profile", iface, user_callback );
}


void
subscribe_user_profile( emirates_iface *iface, subscriber_callback *user_callback ) {
  subscribe_to_service( "user_profile", iface, user_callback );
}


void
publish_service_profile( emirates_iface *iface, jedex_parcel *parcel ) {
  publish_service( "service_profile", iface, parcel );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
