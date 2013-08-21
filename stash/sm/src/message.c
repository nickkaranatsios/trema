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


// just a discrimination type
#define ADD 1
#define DEL 2


static service_spec *
service_spec_service_find( const char *service, service_profile_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    if ( !strncmp( service, tbl->service_specs[ i ]->name, strlen( tbl->service_specs[ i ]->name ) ) ) {
      return tbl->service_specs[ i ];
    }
  }

  return NULL;
}


static void
dispatch_add_service_to_sc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "service_module_regist_request", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "service_name", &field, &index );
  jedex_value_set_string( &field, spec->name );
  jedex_value_get_by_name( rval, "service_module", &field, &index );
  jedex_value_set_string( &field, spec->service_module );
  jedex_value_get_by_name( rval, "recipe", &field, &index );
  jedex_value_set_string( &field, spec->chef_recipe );
  self->emirates->send_request( self->emirates, SM_SERVICE_MODULE_REGIST, rval, sub_schema_find( "common_reply", self->sub_schema ) );
}


static void
dispatch_del_service_to_sc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "service_remove_request", self->rval );
  
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "service_name", &field, &index );
  jedex_value_set_string( &field, spec->name );
  self->emirates->send_request( self->emirates, SM_REMOVE_SERVICE, rval, sub_schema_find( "common_reply", self->sub_schema ) );
}


static void 
dispatch_add_service_profile_to_nc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "add_service_profile_request", self->rval );

  for ( size_t i = 0; i < spec->service_profile_specs_nr; i++ ) {
    service_profile_spec *sp_spec = spec->service_profile_specs[ i ];

    jedex_value field;
    size_t index;
    jedex_value_get_by_name( rval, "name", &field, &index );
    jedex_value_set_string( &field, spec->name );

    jedex_value_get_by_name( rval, "match_id", &field, &index );
    jedex_value_set_int( &field, ( int ) sp_spec->spec_value.match_id );

    jedex_value_get_by_name( rval, "wildcards", &field, &index );
    jedex_value_set_int( &field, ( int ) sp_spec->spec_value.wildcards );

    jedex_value_get_by_name( rval, "dl_src", &field, &index );
    jedex_value_set_long( &field, ( int ) sp_spec->spec_value.dl_src );

    jedex_value_get_by_name( rval, "dl_dst", &field, &index );
    jedex_value_set_long( &field, ( int64_t ) sp_spec->spec_value.dl_dst );

    jedex_value_get_by_name( rval, "dl_vlan", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.dl_vlan );

    jedex_value_get_by_name( rval, "dl_vlan_pcp", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.dl_vlan_pcp );

    jedex_value_get_by_name( rval, "dl_type", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.dl_type );

    jedex_value_get_by_name( rval, "nw_tos", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.nw_tos );

    jedex_value_get_by_name( rval, "nw_proto", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.nw_proto );

    jedex_value_get_by_name( rval, "nw_src", &field, &index );
    jedex_value_set_int( &field, ( int ) sp_spec->spec_value.nw_src );

    jedex_value_get_by_name( rval, "nw_dst", &field, &index );
    jedex_value_set_int( &field, ( int ) sp_spec->spec_value.nw_dst );

    jedex_value_get_by_name( rval, "tp_src", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.tp_src );

    jedex_value_get_by_name( rval, "tp_src", &field, &index );
    jedex_value_set_int( &field, sp_spec->spec_value.tp_dst );

    self->emirates->send_request( self->emirates, SM_ADD_SERVICE_PROFILE, rval, sub_schema_find( "common_reply", self->sub_schema ) );
  }
}


static void
dispatch_del_service_profile_to_nc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "del_service_profile_request", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "name", &field, &index );
  jedex_value_set_string( &field, spec->name );
  self->emirates->send_request( self->emirates, SM_DEL_SERVICE_PROFILE, rval, sub_schema_find( "common_reply", self->sub_schema ) );
}


static void
dispatch_stop_service_specific_nc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "shutdown_ssnc_request", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "name", &field, &index );
  jedex_value_set_string( &field, spec->name );
  self->emirates->send_request( self->emirates, SM_SHUTDOWN_SSNC, rval, sub_schema_find( "common_reply", self->sub_schema ) );
}


static void
dispatch_start_service_specific_nc( service_spec *spec, service_manager *self ) {
  jedex_value *rval = jedex_value_find( "ignite_ssnc_request", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "name", &field, &index );
  jedex_value_set_string( &field, spec->name );
  self->emirates->send_request( self->emirates, SM_IGNITE_SSNC, rval, sub_schema_find( "common_reply", self->sub_schema ) );
}


static void
dispatch_service_to_sc( int operation, jedex_value *val, service_manager *self ) {
  if ( val && val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *cstr;
    size_t size;
    jedex_value_get_string( &field, &cstr, &size );
    service_spec *spec = service_spec_service_find( cstr, &self->service_profile_tbl );
    if ( spec != NULL ) {
      if ( operation == ADD ) {
      // dispatch service to service_controller/network_controller
        dispatch_add_service_to_sc( spec, self );
      }
      if ( operation == DEL ) {
        dispatch_del_service_to_sc( spec, self );
      }
      //dispatch_service_to_nc( service, spec, self );
    }
  }
}


static void
dispatch_service_to_nc( int operation, jedex_value *val, service_manager *self ) {
  if ( val && val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *cstr;
    size_t size;
    jedex_value_get_string( &field, &cstr, &size );
    service_spec *spec = service_spec_service_find( cstr, &self->service_profile_tbl );
    if ( spec != NULL ) {
       if ( operation == ADD ) {
         dispatch_add_service_profile_to_nc( spec, self );
         dispatch_start_service_specific_nc( spec, self );
       }
       if ( operation == DEL ) {
         dispatch_del_service_profile_to_nc( spec, self );
         dispatch_stop_service_specific_nc( spec, self );
       }
    }
  }
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

  UNUSED( json );
  UNUSED( client_id );

printf( "oss_bss_add_service_handler\n" );
  service_manager *self = user_data;
  dispatch_service_to_sc( ADD, val, self );
  dispatch_service_to_nc( ADD, val, self );
}


void
oss_bss_del_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( json );
  UNUSED( client_id );

printf( "oss_bss_del_service_handler\n" );
  service_manager *self = user_data;
  dispatch_service_to_sc( DEL, val, self );
  dispatch_service_to_nc( DEL, val, self );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
