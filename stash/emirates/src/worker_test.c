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
#include "jedex_iface.h"



static void
request_menu_callback( void *args ) {
}


static void
reply_menu_callback( void *args ) {
}

static void
request_profile_callback( void *args ) {
}


static void
reply_profile_callback( void *args ) {
}


static int
poll_responder( emirates_priv *priv ) {
  zmq_pollitem_t poller = { responder_socket( priv->responder ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 1000 ); 
  if ( rc == 1 ) {
    printf( "should call responder callback\n" );
    zmsg_t *msg = zmsg_recv( poller.socket );
    zframe_t *client_id_frame = zmsg_first( msg );
    zframe_t *empty_frame = zmsg_next( msg );
    zframe_t *msg_type_frame = zmsg_next( msg );
    zframe_t *service_frame = zmsg_next( msg );
    zmsg_t *reply = zmsg_new();
    zmsg_addmem( reply, ( const char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
    zmsg_addstr( reply, "" );
    zmsg_addstr( reply, REPLY );
    zmsg_addmem( reply, ( const char * ) zframe_data( service_frame ), zframe_size( service_frame ) );
    zmsg_send( &reply, poller.socket );
    zmsg_destroy( &msg );
    zmsg_destroy( &reply );
  }

  return rc;
}


static int
poll_requester( emirates_priv *priv ) {
  zmq_pollitem_t poller = { requester_socket( priv->requester ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 1000 ); 
  if ( rc == 1 ) {
    printf( "client %s received reply\n", requester_id( priv->requester ) );
    // expect reply frame + service frame
    zmsg_t *msg = zmsg_recv( poller.socket );
    zframe_t *reply_frame  = zmsg_first( msg );
    size_t frame_size = zframe_size( reply_frame );
    assert( frame_size != 0 );
    zframe_t *service_frame = zmsg_next( msg );
    frame_size = zframe_size( service_frame );
    assert( frame_size != 0 );
    //send_profile_request( priv );
    //send_menu_request( priv );
  }

  return rc;
}


int
main( int argc, char **argv ) {
  emirates_iface *iface = emirates_initialize();
  zclock_sleep( 2 * 1000 );
  if ( iface != NULL ) {
    iface->set_service_request( "menu", iface, request_menu_callback );
    set_ready( iface );
    //set_profile_request( iface, request_profile_callback );
    //set_profile_reply( iface, reply_profile_callback );
    //send_profile_request( iface->priv );

    int i = 0;
    while( !zctx_interrupted ) {
      poll_responder( iface->priv );
      //poll_requester( iface->priv );
//      zclock_sleep( 1 * 1000 );
    }

    emirates_finalize( &iface );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
