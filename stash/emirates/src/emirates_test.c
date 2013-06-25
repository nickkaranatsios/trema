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
service_profile_callback( void *args ) {
}


static void
user_profile_callback( void *args ) {
}


static int
poll_subscriber( emirates_iface *iface ) {
  zmq_pollitem_t poller = { iface->priv->sub, 0, ZMQ_POLLIN, 0 };
  int rc = zmq_poll( &poller, 1, 0 ); 
  if ( rc == 1 ) {
    iface->priv->sub_handler( &poller, iface );
  }
  return rc;
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


int
main( int argc, char **argv ) {
  jedex_schema *schema = jedex_initialize( "" );

  const char *sub_schemas[] = { "" };
  jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
  assert( parcel );

  jedex_value *val = jedex_parcel_value( parcel, "" );
  assert( val );

  set_menu_record_value( val );

  emirates_iface *iface = emirates_initialize();
  if ( iface != NULL ) {
    printf( "GOOD iface ptr\n" );

    subscribe_service_profile( iface, service_profile_callback );
    int i = 0;
    while( !zctx_interrupted ) {
      poll_subscriber( iface );
      if ( !i ) {
        subscribe_user_profile( iface, user_profile_callback );
      }
      if ( i++ == 5 ) {
        publish_service_profile( iface, parcel );
      }
      zclock_sleep( 1 * 1000 );
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
