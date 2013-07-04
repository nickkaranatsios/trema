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


#define POLL_DFLT 1

#define poll_recv( socket ) \
  recv_reply( ( socket ), POLL_DFLT )

#define poll_recv_timeout( socket, timeout ) \
  recv_reply( ( socket ), ( timeout ) ) 


char *
recv_reply( void *socket, int timeout ) {
  int rc;
  char *reply = NULL;

  zmq_pollitem_t poller = { socket, 0, ZMQ_POLLIN, 0 };
  rc = zmq_poll( &poller, 1, timeout * 1000 );
  if ( rc ) {
    if ( poller.revents & ZMQ_POLLIN ) {
      reply = zstr_recv( socket );
    }
  }

  return reply;
}


static int
wait_for_reply( void *socket ) {
  zmsg_t *msg = zmsg_recv( socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 1 );

  zframe_t *msg_type_frame = zmsg_first( msg );
  size_t frame_size = zframe_size( msg_type_frame );
  if ( !msg_is( ADD_SERVICE_REPLY, ( const char * ) zframe_data( msg_type_frame ), frame_size ) ) {
    printf( "ADD_SERVICE_REPLY\n" );
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
  zmsg_destroy( &msg );
}


void
service_request( const char *service, emirates_priv *priv, request_callback *callback ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->responder );
  zmsg_destroy( &msg );
  wait_for_reply( priv->responder );
}


void
service_reply( const char *service, emirates_priv *priv, reply_callback *callback ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, ADD_SERVICE_REPLY );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->requester );
  zmsg_destroy( &msg );
  wait_for_reply( priv->requester );
}


void
send_request( const char *service, emirates_priv *priv ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, REQUEST );
  zmsg_addstr( msg, service );
  zmsg_send( &msg, priv->requester );
  zmsg_destroy( &msg );
}

void
send_ready( void *socket ) {
  send_single_msg( socket, READY );
}


void
set_ready( emirates_iface *iface ) {
  send_ready( iface->priv->responder );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
