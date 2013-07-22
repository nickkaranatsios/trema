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
  sub_callback *item = zlist_first( sub_callbacks );

  while ( item != NULL ) {
    if ( !strcmp( item->service, service ) ) {
      return item;
    }
    item = zlist_next( sub_callbacks );
  }

  return NULL;
}


static int
subscription_callback_add( subscriber_info *self, sub_callback *cb ) {
  if ( subscriber_callbacks( self ) == NULL ) {
    subscriber_callbacks( self ) = zlist_new();
  }

  int rc = 0;
  sub_callback *item = lookup_subscription_service( subscriber_callbacks( self ), cb->service );
  if ( !item ) { 
    rc = zlist_append( subscriber_callbacks( self ), cb );
  }
  else {
    item->callback = cb->callback;
  }

  return rc;
}


static void
subscription_callback_free( zlist_t *callbacks ) {
  sub_callback *item = zlist_first( callbacks );

  while ( item != NULL ) {
    if ( *item->schemas ) {
      free_array( item->schemas );
    }
    item = zlist_next( callbacks );
  }
  zlist_destroy( &callbacks );
}


static int
subscriber_data_handler( zmq_pollitem_t *poller, void *arg ) {
  subscriber_info *self = arg;

  
  zmsg_t *msg = one_or_more_msg( poller->socket );
  
  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  int rc = 0;

  zframe_t *sub_frame = zmsg_first( msg );
  if ( sub_frame != NULL ) {
    // match subscription and send to application
    sub_callback *item = lookup_subscription_service( subscriber_callbacks( self ), ( const char * ) zframe_data( sub_frame ) );  
    if ( item != NULL ) {
      zframe_t *service_frame = zmsg_next( msg );
      const char *json_data = ( const char * ) zframe_data( service_frame );
      int i = 0;
      while ( *( item->schemas + i ) ) {
        jedex_value *ret_val = json_to_jedex_value( *( item->schemas + i ), json_data );
        if ( ret_val != NULL ) {
           item->callback( service_frame );
           break;
        }
        i++;
      }
    }
  }
  zmsg_destroy( &msg );

  return rc;
}


static int
subscriber_input( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  UNUSED( loop );
  subscriber_info *self = arg;

  zmsg_t *msg = one_or_more_msg( poller->socket );

  int rc = zmsg_send( &msg, subscriber_pipe_socket( self ) );
  if ( !rc ) {
    signal_notify_out( subscriber_notify_out( self ) );
  }
  else {
    log_err( "Failed to send subscribed topic to parent thread" );
  }
  zmsg_destroy( &msg );

  return rc;
}


static int
subscriber_output( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  UNUSED( loop );
  subscriber_info *self = arg;

  zmsg_t *msg = one_or_more_msg( poller->socket );
  int rc = 0;

  zframe_t *msg_type_frame = zmsg_first( msg );

  if ( zframe_streq( msg_type_frame, SUBSCRIPTION_MSG ) && zframe_more( msg_type_frame ) ) {
    zframe_t *subscription_frame = zmsg_next( msg );
    if ( subscription_frame ) {
      zsockopt_set_subscribe( subscriber_zmq_socket( self ), ( char * ) zframe_data( subscription_frame ) );
    }
  }
  else {
   if ( zframe_streq( msg_type_frame, EXIT ) ) {
     rc = EINVAL;
   }
  }
  zmsg_destroy( &msg );

  return rc;
}


static void
start_subscribing( subscriber_info *self ) {
  zloop_t *sub_loop = zloop_new();

  zmq_pollitem_t poller = { subscriber_zmq_socket( self ), 0, ZMQ_POLLIN, 0 };
  // wait for any matched publishing data
  zloop_poller( sub_loop, &poller, subscriber_input, self );
  poller.socket = subscriber_pipe_socket( self );
  zloop_poller( sub_loop, &poller, subscriber_output, self );

  zloop_set_verbose( sub_loop, true );
  int rc = zloop_start( sub_loop );
  if ( rc == -1 ) {
    log_err( "Subscriber child thread stopped" );
  }
  send_ng_status( subscriber_pipe_socket( self ) );
}


static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  subscriber_info *self = args;
  subscriber_pipe_socket( self ) = pipe;
  int rc;

  subscriber_zmq_socket( self ) = zsocket_new( ctx, ZMQ_SUB );
  rc = zsocket_connect( subscriber_zmq_socket( self ), "tcp://localhost:%zu", subscriber_port( self ) );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XPUB %d %u", rc, subscriber_port( self ) );
    send_ng_status( subscriber_pipe_socket( self ) );
    return;
  }
  send_ok_status( subscriber_pipe_socket( self ) );
  start_subscribing( self );
}


void
subscribe_to_service( const char *service,
                      subscriber_info *self,
                      const char **schema_names,
                      subscriber_callback *user_callback ) {
  sub_callback *cb = ( sub_callback * ) zmalloc( sizeof( sub_callback ) );
  cb->callback = user_callback;
  cb->service = service;

  if ( *schema_names ) {
    size_t nr_schema_names = 0;
    while ( *( schema_names + nr_schema_names ) ) {
      nr_schema_names++;
    }
    size_t schema_size = sizeof( void * ) * nr_schema_names + 1;
    cb->schemas = ( void ** ) zmalloc( schema_size );
    size_t i;
    for ( i = 0; i < nr_schema_names; i++ ) {
      cb->schemas[ i ] = jedex_initialize( schema_names[ i ] );
    }
    cb->schemas[ i ] = NULL;
  }
  else {
    size_t schema_size = sizeof( void * ) * 2;
    cb->schemas = ( void ** ) zmalloc( schema_size );
    cb->schemas[ 0 ] = jedex_initialize( "" );
    cb->schemas[ 1 ] = NULL;
  }
    
  if ( !subscription_callback_add( self, cb ) ) {
    zmsg_t *set_subscription = zmsg_new();
    zmsg_addstr( set_subscription, SUBSCRIPTION_MSG );
    zmsg_addstr( set_subscription, service );
    zmsg_send( &set_subscription, subscriber_socket( self ) );
    zmsg_destroy( &set_subscription );
  }
  else {
    free_array( cb->schemas );
    free( cb );
  }
}


int
subscriber_init( emirates_priv *priv ) {
  priv->subscriber = ( subscriber_info * ) zmalloc( sizeof( subscriber_info ) );
  memset( priv->subscriber, 0, sizeof( subscriber_info ) );
  subscriber_port( priv->subscriber ) = SUB_BASE_PORT;
  subscriber_socket( priv->subscriber ) = zthread_fork( priv->ctx, subscriber_thread, priv->subscriber );
  create_notify( priv->subscriber );
  if ( subscriber_notify_in( priv->subscriber ) == -1 || subscriber_notify_out( priv->subscriber ) == -1 ) {
    return EINVAL;
  }
  if( check_status( subscriber_socket( priv->subscriber ) ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }
  subscriber_handler( priv->subscriber )= subscriber_data_handler;
  subscriber_callbacks( priv->subscriber ) = NULL;

  return 0;
}


void
subscriber_finalize( emirates_priv **priv ) {
  emirates_priv *priv_p = *priv;
  if ( priv_p->subscriber ) {
    subscriber_info *self = priv_p->subscriber;
    if ( self->callbacks ) {
      subscription_callback_free( self->callbacks );
    }
    free( priv_p->subscriber );
    priv_p->subscriber = NULL;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
