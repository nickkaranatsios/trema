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
#include <uuid/uuid.h>
#include <poll.h>
#include <trema.h>


static emirates_iface *iface;


static void
request_menu_callback( void *args ) {
  UNUSED( args );
}


static void
reply_menu_callback( void *args ) {
  UNUSED( args );
}


static int
poll_subscriber( emirates_priv *priv ) {
  zmq_pollitem_t poller = { subscriber_socket( priv->subscriber ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 10 ); 
  if ( ( rc == 1 ) && ( poller.revents & ZMQ_POLLIN ) ) {
    printf( "poll subscriber\n" );
    subscriber_handler( priv->subscriber )( &poller, priv->subscriber );
  }

  return rc;
}


static void
poll_responder( emirates_priv *self ) {
  zmq_pollitem_t poller = { responder_socket( self->responder ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 10 );
  if ( ( rc == 1 ) && ( poller.revents & ZMQ_POLLIN  ) ) {
    zmsg_t *msg = zmsg_recv( responder_socket( self->responder ) );
    size_t nr_frames = zmsg_size( msg );
    printf( "poll responder(%d)\n", nr_frames );
    zframe_t *client_id_frame = zmsg_first( msg );
    zframe_t *empty_frame = zmsg_next( msg );
    UNUSED( empty_frame );
    zframe_t *msg_type_frame = zmsg_next( msg );
    UNUSED( msg_type_frame );
    zframe_t *service_frame = zmsg_next( msg );
    UNUSED( service_frame );
    zmsg_t *reply = zmsg_new();
    zmsg_addmem( reply, ( const char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
    zmsg_addstr( reply, "" );
    zmsg_addstr( reply, REPLY );
    zmsg_addmem( reply, ( const char * ) zframe_data( service_frame ), zframe_size( service_frame ) );
    zmsg_send( &reply, responder_socket( self->responder ) );
    zmsg_destroy( &msg );
    zmsg_destroy( &reply );
  }
}


static void
poll_requester( emirates_priv *self ) {
  zmq_pollitem_t poller = { requester_socket( self->requester ), 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 10 );
  if ( ( rc == 1 ) &&  ( poller.revents & ZMQ_POLLIN ) ) {
    printf( "client %s received reply\n", requester_id( self->requester ) );
    // expect reply frame + service frame
    zmsg_t *msg = one_or_more_msg( requester_socket( self->requester ) );
    if ( msg ) {
      zframe_t *msg_type_frame = zmsg_first( msg );
      bool next_service_frame = false;
      if ( ( !msg_is( REPLY, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) ||
           ( !msg_is( REPLY_TIMEOUT, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) ) {
        next_service_frame = true; 
      } 
      if ( next_service_frame ) {
        zframe_t *service_frame = zmsg_next( msg );
        size_t frame_size = zframe_size( service_frame );
        assert( frame_size != 0 );
        char service[ IDENTITY_MAX ];
        memcpy( service, zframe_data( service_frame ), frame_size );
        service[ frame_size ] = '\0';
        rep_callback *handler = lookup_reply_callback( requester_callbacks( self->requester ), service );

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
          // TESTING send next request
          //send_menu_request( self );
        }
      }
    }
  }
}
  
  
#ifdef TEST
static void
poll_notify_in( int fd, void *user_data ) {
  UNUSED( fd );
  emirates_priv *priv = user_data;
  struct pollfd pfd;
  nfds_t nfds = 1;
  pfd.fd = requester_notify_in( priv->requester );
  pfd.events = POLLIN;
  
  int rc = poll( &pfd, nfds, 0 ); 
  if ( rc >= 0 ) {
    if ( pfd.revents & POLLIN ) {
      char dummy;
      read( requester_notify_in( priv->requester ), &dummy, sizeof( dummy ) );
      return poll_requester( priv );
    }
    if ( pfd.revents & POLLERR ) {
      log_err( "error on returned event" );
    }
    if ( rc < 0) {
      if ( errno != EINTR ) {
        log_err( "poll failed %s", strerror( errno ) );
      }
    }
  }
}
#endif


static void
check_requester_notify_in( int fd, void *user_data ) {
  UNUSED( fd );
  notify_in_requester( fd, user_data );
}


static void
check_responder_notify_in( int fd, void *user_data ) {
  UNUSED( fd );
  notify_in_responder( fd, user_data );
}


static void
check_subscriber_notify_in( int fd, void *user_data ) {
  UNUSED( fd );
  notify_in_subscriber( fd, user_data );
}


static char *
generate_uuid( void ) {
  char hex_char[] = "0123456789ABCDEF";
  char *uuidstr = zmalloc( sizeof( uuid_t ) * 2 + 1 );
  uuid_t uuid;
  uuid_generate( uuid );
  size_t byte_nbr;
  for ( byte_nbr = 0; byte_nbr < sizeof( uuid_t ); byte_nbr++ ) {
    uuidstr[ byte_nbr * 2 + 0] = hex_char[ uuid[ byte_nbr ] >> 4 ];
    uuidstr[ byte_nbr * 2 + 1 ] = hex_char [uuid[ byte_nbr ] & 15 ];
  }

  return uuidstr;
}


static void
initialize_emirates( void ) {
  iface = emirates_initialize();
  assert( iface );
  set_fd_handler( subscriber_notify_in( ( priv( iface ) )->subscriber ), check_subscriber_notify_in, iface->priv, NULL, NULL );
  set_fd_handler( responder_notify_in( ( priv( iface ) )->responder ), check_responder_notify_in, iface->priv, NULL, NULL );
  set_fd_handler( requester_notify_in( ( priv( iface ) )->requester ), check_requester_notify_in, iface->priv, NULL, NULL );
}


static void
service_profile_callback( void *args ) {
  UNUSED( args );
}


static void
set_menu_record_value( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "header", &field, &index );
  jedex_value_set_string( &field, "Save us menu" );
  jedex_value_get_by_name( val, "items", &field, &index );
  jedex_value element;
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "save the children" );
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "save the world" );
}


static void
handle_timer_event( void *user_data ) {
  UNUSED( user_data );
  if ( iface ) {
    iface->set_service_request( "menu", iface, request_menu_callback );
    set_menu_reply( iface, reply_menu_callback );
    set_ready( iface );
    set_readable( responder_notify_in( ( priv( iface ) )->responder ), true );
    uint32_t tx_id = send_menu_request( priv( iface ) );
    UNUSED( tx_id );
    set_readable( requester_notify_in( ( priv( iface ) )->requester ), true );
    delete_timer_event( handle_timer_event, NULL );

    set_readable( subscriber_notify_in( ( priv( iface ) )->subscriber ), true );
    jedex_schema *schema = jedex_initialize( "" );
    const char *sub_schemas[] = { NULL };
    jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
    assert( parcel );
    jedex_value *val = jedex_parcel_value( parcel, "" );
    assert( val );
    set_menu_record_value( val );
    subscribe_service_profile( iface, sub_schemas, service_profile_callback );
    publish_service_profile( iface, parcel );
#ifdef TEST
#endif
  }
}


int
main( int argc, char **argv ) {
  char *uuidstr = generate_uuid();
  assert( uuidstr );
  init_trema( &argc, &argv );
  set_external_callback( initialize_emirates );

  struct itimerspec interval;
  interval.it_interval.tv_sec = 1;
  interval.it_interval.tv_nsec = 0;
  interval.it_value.tv_sec = 0;
  interval.it_value.tv_nsec = 0;
  add_timer_event_callback( &interval, handle_timer_event, NULL );

  start_trema();

#ifdef TEST
  zclock_sleep( 2 * 1000 );
  if ( iface != NULL ) {
    iface->set_service_request( "menu", iface, request_menu_callback );
    set_menu_request( iface, request_menu_callback );
    set_menu_reply( iface, reply_menu_callback );
    set_ready( iface );
    uint32_t tx_id = send_menu_request( iface->priv );
printf( "tx_id = %zu\n", tx_id );

    int i = 0;
    while( !zctx_interrupted ) {
      poll_responder( iface->priv );
      //poll_requester( iface->priv );
      poll_notify_in( iface->priv );
      // inject extra messages into the queue.
//      if  ( ( i++ % 10 ) == 0 ) 
//        send_menu_request( iface->priv );
//      zclock_sleep( 1 * 1000 );
    }

    emirates_finalize( &iface );
  }
#endif

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */