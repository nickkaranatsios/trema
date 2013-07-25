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
#include "checks.h"


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
  UNUSED( argc );
  UNUSED( argv );
  jedex_schema *schema = jedex_initialize( "" );

  const char *sub_schemas[] = { NULL };
  jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
  assert( parcel );

  jedex_value *val = jedex_parcel_value( parcel, "" );
  assert( val );

  set_menu_record_value( val );

  int flag = 0;
  emirates_iface *iface = emirates_initialize_only( ENTITY_SET( flag, PUBLISHER ) );
  if ( iface != NULL ) {
    zclock_sleep( 5 * 1000 );
    iface->publish( iface, "menu", parcel );
    zclock_sleep( 5 * 1000 );
  }
  emirates_finalize( &iface );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
