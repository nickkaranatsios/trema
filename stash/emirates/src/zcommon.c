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


static int
wait_for_reply( void *socket ) {
  zmsg_t *msg = zmsg_recv( socket );
  if ( msg == NULL ) {
    return EINVAL;
  }
  zframe_t *msg_type_frame = zmsg_first( msg );
  size_t frame_size = zframe_size( msg_type_frame );
  if ( !msg_is( ADD_SERVICE_REPLY, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
    return 0;
  }
  if ( !msg_is( ADD_SERVICE_REQUEST, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
    return 0;
  }

  return EINVAL;
}


static void
send_single_msg( void *socket, const char *content ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, content );
  zmsg_send( &msg, socket );
}


void
service_request( const char *service, emirates_iface *iface, request_handler callback ) {
  UNUSED( callback );
  emirates_priv *priv = iface->priv;
  assert( priv );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, responder_socket( priv->responder ) );
  wait_for_reply( responder_socket( priv->responder ) );
}


void
service_reply( const char *service, emirates_priv *priv, reply_callback *callback ) {
  requester_inc_timeout_id( priv->requester );
  add_reply_callback( service, callback, priv->requester );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REPLY );
  zmsg_addmem( msg, &requester_timeout_id( priv->requester ), sizeof( requester_timeout_id( priv->requester ) ) );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, requester_socket( priv->requester ) );
  wait_for_reply( requester_socket( priv->requester ) );
}


uint32_t
send_request( const char *service, emirates_priv *priv ) {
  requester_inc_timeout_id( priv->requester );
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, REQUEST );
  zmsg_addmem( msg, &requester_timeout_id( priv->requester ), sizeof( requester_timeout_id( priv->requester ) ) );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, requester_socket( priv->requester ) );

  return requester_timeout_id( priv->requester );
}


void
send_ready( void *socket ) {
  send_single_msg( socket, READY );
}


void
set_ready( emirates_iface *iface ) {
  send_ready( responder_socket( ( priv( iface ) )->responder ) );
}


zmsg_t *
one_or_more_msg( void *socket ) {
  zmsg_t *msg = zmsg_recv( socket );
  if ( msg == NULL ) {
    return NULL;
  }

  size_t nr_frames = zmsg_size( msg );
  if ( nr_frames == 0 ) {
    zmsg_destroy( &msg );
    return NULL;
  }

  return msg;
}


long
get_time_left( int64_t expiry ) {
  long time_left;

 if ( !expiry ) {
    time_left = -1;
  }
  else {
    time_left = ( long ) ( expiry - zclock_time() );
    if ( time_left < 0 ) {
      time_left = 0;
    }
    time_left *= ZMQ_POLL_MSEC;
  }

  return time_left;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
