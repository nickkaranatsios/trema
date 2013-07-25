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


#include "emirates_priv.h"
#include "callback.h"


static void
send_reply_timeout( responder_info *self ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addmem( msg, self->client_id, strlen( self->client_id ) );
  zmsg_addstr( msg, "" );
  zmsg_addstr( msg, REPLY_TIMEOUT );
  zframe_t *service_frame = responder_service_frame( self );
  zmsg_addstr( msg, ( const char * ) zframe_data( service_frame ), zframe_size( service_frame ) );
  zmsg_send( &msg, responder_zmq_socket( self ) );
  zframe_destroy( &service_frame );

}


static void
self_msg_recv( responder_info *self, zmsg_t *msg, zframe_t *msg_type_frame ) {
  size_t msg_type_frame_size = zframe_size( msg_type_frame );

  if ( !msg_is( READY, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
  }
  if ( !msg_is( ADD_SERVICE_REQUEST, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    zmsg_send( &msg, responder_pipe_socket( self ) );
    signal_notify_out( responder_notify_out( self ) );
  }
}


static void
process_request( responder_info *self, zmsg_t *msg ) {
  zframe_t *client_id_frame = zmsg_first( msg );
  size_t client_id_frame_size = zframe_size( client_id_frame );
  assert( client_id_frame_size < IDENTITY_MAX );

  memcpy( self->client_id,  ( char * ) zframe_data( client_id_frame ), client_id_frame_size );
  self->client_id[ client_id_frame_size ] = '\0';

  zframe_t *empty_frame = zmsg_next( msg );
  size_t empty_frame_size = zframe_size( empty_frame );
  assert( empty_frame_size == 0 );

  zframe_t *msg_type_frame = zmsg_next( msg );
  size_t msg_type_frame_size = zframe_size( msg_type_frame );

  if ( !msg_is( REQUEST, ( const char * ) zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    zframe_t *service_frame = zmsg_next( msg );
    responder_service_frame( self ) = zframe_dup( service_frame );
    zmsg_send( &msg, responder_pipe_socket( self ) );
    signal_notify_out( responder_notify_out( self ) );
    responder_expiry( self ) = zclock_time() + REPLY_TIMER;
  }
}


static int
responder_input( responder_info *self ) {
  zmsg_t *msg = one_or_more_msg( responder_zmq_socket( self ) );

  size_t nr_frames = zmsg_size( msg );
  printf( "responder <== %zu\n", nr_frames );
  if ( nr_frames == 1 ) {
    zframe_t *msg_type_frame = zmsg_first( msg );
    self_msg_recv( self, msg, msg_type_frame );
  }
  else {
    process_request( self, msg );
  }

  return 0;
}


static int
responder_output( responder_info *self ) {
  zmsg_t *msg = one_or_more_msg( responder_pipe_socket( self ) );
  size_t nr_frames = zmsg_size( msg );
  printf( "responder ==> %zu\n", nr_frames );

  responder_expiry( self ) = 0;
  zmsg_send( &msg, responder_zmq_socket( self ) );

  return 0;
}


static void
start_responder( responder_info *self ) {
  while ( 1 ) {
    zmq_pollitem_t items[] = {
      { responder_pipe_socket( self ), 0, ZMQ_POLLIN, 0 },
      { responder_zmq_socket( self ), 0, ZMQ_POLLIN, 0 }
    };
    int poll_size = 2;
    int rc = zmq_poll( items, poll_size, get_time_left( responder_expiry( self ) ) );
    if ( rc == -1 ) {
      break;
    }
    if ( items[ 0 ].revents & ZMQ_POLLIN ) {
      rc = responder_output( self );
    }
    if ( items[ 1 ].revents & ZMQ_POLLIN ) {
      rc = responder_input( self );
    }
    if ( rc == -1 ) {
      break;
    }
    if ( responder_expiry( self ) ) {
      if ( zclock_time() >= responder_expiry( self ) ) {
        printf( "waiting for reply from application expired\n" );
        send_reply_timeout( self );
        responder_expiry( self ) = 0;
      }
    }
  }
  printf( "out of responder\n" );
}


static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  responder_info *self = args;
  responder_pipe_socket( self ) = pipe;
  uint32_t port = responder_port( self );
  
  responder_zmq_socket( self ) = zsocket_new( ctx, ZMQ_REQ );

  responder_id( self ) = ( char * ) zmalloc( sizeof( char ) * IDENTITY_MAX );
  size_t responder_id_size = IDENTITY_MAX;
  snprintf( responder_id( self ), responder_id_size, "%lld", zclock_time() );
  zsocket_set_identity( responder_zmq_socket( self ), responder_id( self ) );

  int rc = zsocket_connect( responder_zmq_socket( self ), "tcp://localhost:%zu", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( responder_pipe_socket( self ) );
    return;
  }
  
  send_ok_status( responder_pipe_socket( self ) );
  start_responder( self );
}


static int
request_callback_add( responder_info *self, request_callback *cb ) {
  assert( responder_callbacks( self ) );

  int rc = 0;
  request_callback *item = lookup_callback( responder_callbacks( self ), cb->key.service );
  if ( !item ) { 
    rc = zlist_append( responder_callbacks( self ), cb );
  }
  else {
    item->callback = cb->callback;
  }

  return rc;
}


static void
add_request_callback( const char *service , request_handler user_callback, responder_info *self ) {
  request_callback *cb = ( request_callback * ) zmalloc( sizeof( request_callback ) );
  cb->key.service = service;
  cb->responder_id = responder_own_id( self );
  cb->callback = user_callback;
  if ( request_callback_add( self, cb ) ) {
    free( cb );
  }
}


static void
route_msg( const emirates_priv *self, zmsg_t *msg, zframe_t *msg_type_frame ) {
  char service[ IDENTITY_MAX ];

  if ( !msg_is( REQUEST, ( const char * ) zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) {
    zframe_t *service_frame = zmsg_next( msg );
    size_t frame_size = zframe_size( service_frame );

    memcpy( service, zframe_data( service_frame ), frame_size );
    service[ frame_size ] = '\0';

    request_callback *handler = lookup_callback( responder_callbacks( self->responder ), service );
    if ( handler ) {
      zframe_t *data_frame = zmsg_next( msg );
      handler->callback( zframe_data( data_frame ) );
    }
  }
}


static int
responder_poll( const emirates_priv *self ) {
  long timeout = POLL_TIMEOUT;

  zmq_pollitem_t poller = { responder_socket( self->responder ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, timeout );
  if ( ( rc == 1 ) && ( poller.revents & ZMQ_POLLIN  ) ) {
    zmsg_t *msg = zmsg_recv( responder_socket( self->responder ) );
    size_t nr_frames = zmsg_size( msg );
    log_debug( "poll responder(%zu)\n", nr_frames );
    printf( "poll responder(%zu)\n", nr_frames );
    zframe_t *client_id_frame = zmsg_first( msg );
    UNUSED( client_id_frame );
    zframe_t *empty_frame = zmsg_next( msg );
    UNUSED( empty_frame ); 
    zframe_t *msg_type_frame = zmsg_next( msg );
    route_msg( self, msg, msg_type_frame );
  }

  return rc;
}


int
responder_invoke( const emirates_priv *priv ) {
  notify_in_responder( priv );
  return responder_poll( priv );
}


static void
form_send_reply( emirates_priv *priv, const char *service, const char *json ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addmem( msg, priv->responder->client_id, strlen( priv->responder->client_id ) );
  zmsg_addstr( msg, "" );
  zmsg_addstr( msg, REPLY );
  zmsg_addstr( msg, service );
  zmsg_addmem( msg, json, strlen( json ) );
  zmsg_send( &msg, responder_socket( priv->responder ) );
}


void
send_reply( emirates_iface *iface, const char *service, jedex_value *value ) {
  emirates_priv *priv = iface->priv;
  assert( priv );

  char *json;
  jedex_value_to_json( value, true, &json );
  if ( json ) {
    form_send_reply( priv, service, json );
  }
}


void
send_reply_raw( emirates_iface *iface, const char *service, const char *json ) {
  emirates_priv *priv = iface->priv;
  assert( priv );

  form_send_reply( priv, service, json );
}


void
service_request( emirates_iface *iface, const char *service, request_handler callback ) {
  emirates_priv *priv = iface->priv;
  assert( priv );

  add_request_callback( service, callback, priv->responder );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, responder_socket( priv->responder ) );
}


int
responder_init( emirates_priv *priv ) {
  priv->responder = ( responder_info * ) zmalloc( sizeof( responder_info ) );
  memset( priv->responder, 0, sizeof( responder_info ) );
  responder_port( priv->responder ) = RESPONDER_BASE_PORT;
  responder_socket( priv->responder ) = zthread_fork( priv->ctx, responder_thread, priv->responder );
  responder_callbacks( priv->responder ) = zlist_new();
  create_notify( priv->responder );
  if ( responder_notify_in( priv->responder ) == -1 || responder_notify_out( priv->responder ) == -1 ) {
    return EINVAL;
  }
  if ( check_status( responder_socket( priv->responder ) ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


void
responder_finalize( emirates_priv **priv ) {
  emirates_priv *priv_p = *priv;
  if ( priv_p->responder ) {
    responder_info *self = priv_p->responder;
    zlist_destroy( &responder_callbacks( self ) );
    free( responder_id( self ) );
    close( responder_notify_in( self ) );
    close( responder_notify_out( self ) );
    free( priv_p->responder );
    priv_p->responder = NULL;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
