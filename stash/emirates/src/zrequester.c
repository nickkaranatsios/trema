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
send_reply_timeout_to_self( requester_info *self ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, REPLY_TIMEOUT );
  zmsg_addstr( msg, requester_service_name( self ), strlen( requester_service_name( self ) ) );
  zmsg_addmem( msg , &requester_outstanding_id( self ), sizeof( requester_outstanding_id( self ) ) );
  requester_outstanding_id( self ) = 0;
  enable_output( requester_output_ctrl( self ) );
  zmsg_send( &msg, requester_pipe_socket( self ) );
  signal_notify_out( requester_notify_out( self ) );
}


static int
requester_output( requester_info *self ) {
  zmsg_t *msg = one_or_more_msg( requester_pipe_socket( self ) );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester ==> %zu\n", nr_frames );
    zframe_t *msg_type_frame = zmsg_first( msg );
    UNUSED( msg_type_frame );
    zframe_t *tx_id_frame = zmsg_next( msg );
    memcpy( &requester_outstanding_id( self ), ( const uint32_t * ) zframe_data( tx_id_frame ), zframe_size( tx_id_frame ) );
    zmsg_remove( msg, tx_id_frame );
    zframe_t *service_frame = zmsg_next( msg );
    memcpy( requester_service_name( self ), zframe_data( service_frame ), zframe_size( service_frame ) );
    requester_service_name( self )[ zframe_size( service_frame ) ] = '\0';

    disable_output( requester_output_ctrl( self ) );
    zmsg_send( &msg, requester_zmq_socket( self ) );
    requester_expiry( self ) = zclock_time() + REQUEST_HEARTBEAT;
    return 0;
  }

  return EINVAL;
}


static int
requester_input( requester_info *self ) {
  zmsg_t *msg = one_or_more_msg( requester_zmq_socket( self ) );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester <== %zu\n", nr_frames );
    zmsg_addmem( msg, &requester_outstanding_id( self ), sizeof( requester_outstanding_id( self ) ) );
    requester_outstanding_id( self ) = 0;
    enable_output( requester_output_ctrl( self ) );
    zmsg_send( &msg, requester_pipe_socket( self ) );
    signal_notify_out( requester_notify_out( self ) );
    requester_expiry( self ) = 0;
    return 0;
  }

  return EINVAL;
}


static void
start_requester( requester_info *self ) {
  requester_output_ctrl( self ) = 0;
  requester_expiry( self ) = 0;

  enable_output( self->output_ctrl );
  zmq_pollitem_t items[ 2 ];
  while ( 1 ) {
    int poll_size;

    items[ 0 ].events = items[ 1 ].events = ZMQ_POLLIN;
    items[ 0 ].fd = items[ 1 ].fd = 0;
    items[ 0 ].revents = items[ 1 ].revents = 0;
    if ( use_output( requester_output_ctrl( self ) ) ) {
      items[ 0 ].socket = requester_pipe_socket( self );
      items[ 1 ].socket = requester_zmq_socket( self );
      poll_size = 2;
    }
    else {
      items[ 0 ].socket = requester_zmq_socket( self );
      poll_size = 1;
    }

    int rc = zmq_poll( items, poll_size, get_time_left( requester_expiry( self ) ) );
    if ( rc == -1 ) {
      break;
    }
    if ( use_output( requester_output_ctrl( self ) ) ) {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        rc = requester_output( self );
      }
      if ( items[ 1 ].revents & ZMQ_POLLIN ) {
        rc = requester_input( self );
      }
    }
    else {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        rc = requester_input( self );
      }
    }
    if ( rc == -1 ) {
      break;
    }
    if ( requester_expiry( self ) ) { 
      if ( zclock_time() >= requester_expiry( self ) ) {
        printf( "request timeout do something about\n" );
        send_reply_timeout_to_self( self );
        requester_expiry( self ) = 0;
      }
    }
  }
  printf( "out of requester\n" );
}


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  requester_info *self = args;
  requester_pipe_socket( self ) = pipe;

  uint32_t port = requester_port( self );

  requester_zmq_socket( self ) = zsocket_new( ctx, ZMQ_REQ );

  requester_id( self ) = ( char * ) zmalloc( sizeof( char ) * IDENTITY_MAX );
  size_t requester_id_size = IDENTITY_MAX;
  snprintf( requester_id( self ), requester_id_size, "%lld", zclock_time() );
  zsocket_set_identity( requester_zmq_socket( self ), requester_id( self ) );

  int rc = zsocket_connect( requester_zmq_socket( self ), "tcp://localhost:%zu", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( requester_pipe_socket( self ) );
    return;
  }

  send_ok_status( requester_pipe_socket( self ) );
  start_requester( self );
}


static int
reply_callback_add( requester_info *self, reply_callback *cb ) {
  assert( requester_callbacks( self ) );

  int rc = 0;
  reply_callback *item = lookup_callback( requester_callbacks( self ), cb->key.service );
  if ( !item ) { 
    rc = zlist_append( requester_callbacks( self ), cb );
  }
  else {
    item->callback = cb->callback;
  }

  return rc;
}


static void
add_reply_callback( const char *service , reply_handler user_callback, requester_info *self ) {
  reply_callback *cb = ( reply_callback * ) zmalloc( sizeof( reply_callback ) );
  cb->key.service = service;
  cb->requester_id = requester_own_id( self );
  cb->callback = user_callback;
  if ( reply_callback_add( self, cb ) ) {
    free( cb );
  }
}

static int
requester_poll( const emirates_priv *self ) {
  long timeout = POLL_TIMEOUT;
  zmq_pollitem_t poller = { requester_socket( self->requester ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, timeout );
  if ( ( rc == 1 ) &&  ( poller.revents & ZMQ_POLLIN ) ) {
    // expect reply frame + service frame
    zmsg_t *msg = one_or_more_msg( requester_socket( self->requester ) );
    if ( msg ) {
      zframe_t *msg_type_frame = zmsg_first( msg );
      bool next_service_frame = false;
      if ( ( !msg_is( REPLY, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) ||
         ( !msg_is( REPLY_TIMEOUT, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) ) {
        next_service_frame = true;
      }
      if ( !msg_is( ADD_SERVICE_REPLY, ( const char * ) zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) {
        return rc;
      }
      if ( next_service_frame ) {
        zframe_t *service_frame = zmsg_next( msg );
        size_t frame_size = zframe_size( service_frame );
        assert( frame_size != 0 );

        char service[ IDENTITY_MAX ];
        memcpy( service, zframe_data( service_frame ), frame_size );
        service[ frame_size ] = '\0';
        reply_callback *handler = lookup_callback( requester_callbacks( self->requester ), service );

        size_t nr_frames = zmsg_size( msg );
        if ( nr_frames > 2 ) {
          zframe_t *tx_id_frame = zmsg_next( msg );
          uint32_t tx_id;
          memcpy( &tx_id, ( const uint32_t * ) zframe_data( tx_id_frame ), zframe_size( tx_id_frame ) );
          assert( tx_id );
          // handler should include the same tx_id
          if ( handler ) {
            handler->callback( zframe_data( service_frame ) );
          }
        }
      }
    }
  }

  return rc;
}


int
requester_invoke( const emirates_priv *priv ) {
#ifdef TEST
  char dummy;
  ssize_t bytes_read = read( priv->requester->notify_in, &dummy, sizeof( dummy ) );
  UNUSED( bytes_read );
#endif
  notify_in_requester( priv );
  return requester_poll( priv );
}


void *
get_requester_socket( emirates_priv *priv ) {
  return requester_socket( priv->requester );
}


void
service_reply( emirates_iface *iface, const char *service, reply_handler callback ) {
  emirates_priv *priv = iface->priv;
  assert( priv );

  requester_inc_timeout_id( priv->requester );
  add_reply_callback( service, callback, priv->requester );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REPLY );
  zmsg_addmem( msg, &requester_timeout_id( priv->requester ), sizeof( requester_timeout_id( priv->requester ) ) );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, requester_socket( priv->requester ) );
}


uint32_t
send_request( emirates_iface *iface, const char *service, jedex_parcel *parcel ) {
  UNUSED( parcel );
  emirates_priv *priv = iface->priv;
  assert( priv );
  requester_inc_timeout_id( priv->requester );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, REQUEST );
  zmsg_addmem( msg, &requester_timeout_id( priv->requester ), sizeof( requester_timeout_id( priv->requester ) ) );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, requester_socket( priv->requester ) );

  return requester_timeout_id( priv->requester );
}


int
requester_init( emirates_priv *priv ) {
  priv->requester = ( requester_info * ) zmalloc( sizeof( requester_info ) );
  memset( priv->requester, 0, sizeof( requester_info ) );
  requester_port( priv->requester ) = REQUESTER_BASE_PORT;
  requester_socket( priv->requester ) = zthread_fork( priv->ctx, requester_thread, priv->requester );
  requester_callbacks( priv->requester ) = zlist_new();
  create_notify( priv->requester );
  if ( requester_notify_in( priv->requester ) == -1 || requester_notify_out( priv->requester ) == -1 ) {
    return EINVAL;
  }
  if ( check_status( requester_socket( priv->requester ) ) ) {
    return EINVAL;
  }

  return 0;
}


void
requester_finalize( emirates_priv **priv ) {
  emirates_priv *priv_p = *priv;
  if ( priv_p->requester ) {
    requester_info *self = priv_p->requester;
    zlist_destroy( &requester_callbacks( self ) );
    free( requester_id( self ) );
    close( requester_notify_in( self ) );
    close( requester_notify_out( self ) );
    free( priv_p->requester );
    priv_p->requester = NULL;
  }
}
  

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
