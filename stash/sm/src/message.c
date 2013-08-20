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


#include "service_manager.h"
#include "service_manager_priv.h"


static service_spec *
service_spec_service_find( const char *service, service_profile_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    if ( !strncmp( service, tbl->service_specs[ i ]->name, strlen( tbl->service_specs[ i ]->name ) ) ) {
      return tbl->service_specs[ i ];
    }
  }

  return NULL;
}


void
service_module_registration_handler( const uint32_t tx_id,
                                     jedex_value *val,
                                     const char *json,
                                     void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
service_module_remove_handler( const uint32_t tx_id,
                               jedex_value *val,
                               const char *json,
                               void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
add_service_profile_handler( const uint32_t tx_id,
                             jedex_value *val,
                             const char *json,
                             void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
del_service_profile_handler( const uint32_t tx_id,
                             jedex_value *val,
                             const char *json,
                             void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
start_service_specific_nc_handler( const uint32_t tx_id,
                                   jedex_value *val,
                                   const char *json,
                                   void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
oss_bss_add_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  UNUSED( user_data );
  service_manager *self = user_data;
  if ( val && val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *cstr;
    size_t size;
    jedex_value_get_string( &field, &cstr, &size );
    service_spec *spec = service_spec_service_find( cstr, &self->service_profile_tbl );
    if ( spec != NULL ) {
      // dispatch service to service_controller/network_controller
      dispatch_service_to_sc( service );
      dispatch_service_to_nc( service );
    }
  }
}


void
oss_bss_del_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  service_manager *self = user_data;
  UNUSED( self );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
