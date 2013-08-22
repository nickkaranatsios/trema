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


#include "system_resource_manager.h"
#include "system_resource_manager_priv.h"



static void
publish_pm_info( system_resource_manager *self ) {
  pm_table *tbl = &self->pm_tbl;
  jedex_value *rval = jedex_value_find( "physical_machine_info", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "ip_address", &field, &index );

  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    jedex_value_set_int( rval, ( int32_t ) tbl->pm_specs[ i ]->ip_address ); 
    self->emirates->publish_value( self->emirates, SM_PHYSICAL_MACHINE_INFO, rval );
  }
}


#ifdef LATER
void
dispatch_service_to_sc( int operation, jedex_value *val, system_resource_manager *self ) {
  if ( val && val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *service;
    size_t size;
    jedex_value_get_string( &field, &service, &size );

    double bandwidth;
    jedex_value_get_by_name( val, "band_width", &field, &index ); 
    jedex_value_get_double( &field, &bandwidth );

    uint64_t subscribers;
    jedex_value_get_by_name( val, "subscribers", &field, &index );
    jedex_value_set_long( &field, ( int64_t ) &subscribers );
  }
}
#endif


void
oss_bss_add_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( json );
  UNUSED( client_id );
  UNUSED( val );
  UNUSED( user_data );
#ifdef LATER
  system_resource_manager *self = user_data;
  dispatch_service_to_sc( ADD, val, self );
#endif
}


void
periodic_timer_handler( void *user_data ) {
  system_resource_manager *self = user_data;
  // assert( self != NULL );

  if ( !self->should_publish ) {
    publish_pm_info( self );
    self->should_publish = 1;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
