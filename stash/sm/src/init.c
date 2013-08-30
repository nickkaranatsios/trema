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


static void
service_spec_set( const char *subkey, const char *value, service_spec *spec ) {
  if ( !prefixcmp( subkey, ".name" ) ) {
    spec->name = strdup( value );
  }
  else if ( !prefixcmp( subkey, ".service_module" ) ) {
    spec->service_module = strdup( value );
  }
  else if ( !prefixcmp( subkey, ".chef_recipe" ) ) {
    spec->chef_recipe = strdup( value );
  }
}


static void
service_profile_spec_set( const char *subkey, const char *value, service_profile_spec_value *sp_spec_value ) {
  int base = 0;

  if ( !prefixcmp( subkey, ".match_id" ) ) {
    sp_spec_value->match_id = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".wildcards" ) ) {
    sp_spec_value->wildcards = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_src" ) ) {
    sp_spec_value->dl_src = ( uint64_t ) strtoll( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_dst" ) ) {
    sp_spec_value->dl_dst = ( uint64_t ) strtoll( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_vlan" ) ) {
    sp_spec_value->dl_vlan = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_vlan_pcp" ) ) {
    sp_spec_value->dl_vlan_pcp = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_type" ) ) {
    sp_spec_value->dl_type = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_tos" ) ) {
    sp_spec_value->nw_tos = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_proto" ) ) {
    sp_spec_value->nw_proto = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_src" ) ) {
    sp_spec_value->nw_src = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_dst" ) ) {
    sp_spec_value->nw_dst = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".tp_src" ) ) {
    sp_spec_value->tp_src = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".tp_dst" ) ) {
    sp_spec_value->tp_dst = ( uint16_t ) strtol( value, NULL, base );
  }
  else {
    log_err( "Failed to assign subkey(%s) with value(%s)", subkey, value );
  }
}


static service_spec *
service_spec_create( const char *key, size_t key_len, service_profile_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    if ( !strncmp( tbl->service_specs[ i ]->key, key, key_len ) ) {
      return tbl->service_specs[ i ];
    }
  }

  ALLOC_GROW( tbl->service_specs, tbl->service_specs_nr + 1, tbl->service_specs_alloc );
  size_t nitems = 1;
  service_spec *spec = xcalloc( nitems, sizeof( *spec ) );
  spec->key = strdup( key );
  spec->key[ key_len ] = '\0';
  tbl->service_specs[ tbl->service_specs_nr++ ] = spec;

  return spec;
}


static service_profile_spec *
service_profile_spec_create( const char *key, size_t key_len, service_spec *spec ) {
  for ( uint32_t i = 0; i < spec->service_profile_specs_nr; i++ ) {
    if ( !strncmp( spec->service_profile_specs[ i ]->key, key, key_len ) ) {
      return spec->service_profile_specs[ i ];
    }
  }

  ALLOC_GROW( spec->service_profile_specs, spec->service_profile_specs_nr + 1, spec->service_profile_specs_alloc );
  size_t nitems = 1;
  service_profile_spec *sp_spec = xcalloc( nitems, sizeof( *sp_spec ) );
  sp_spec->key = strdup( key );
  sp_spec->key[ key_len ] = '\0';
  spec->service_profile_specs[ spec->service_profile_specs_nr++ ] = sp_spec;

  return sp_spec;
}


static int
handle_config( const char *key, const char *value, void *user_data ) {
  service_profile_table *tbl = user_data;
  const char *subkey;
  const char *name;

  // [service_spec "internet_access_service"]
  const char *keyword = "service_spec.";
  if ( !prefixcmp( key, keyword ) ) {
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );
    service_spec *spec = service_spec_create( name, ( size_t ) ( subkey - name ), tbl );
    if ( spec && subkey ) {
      service_spec_set( subkey, value, spec );
    }
  }
  keyword = "service_profile_spec.";
  // [service_profile_spec "internet_access_service_0"]
  if ( !prefixcmp( key, keyword ) ) {
    // internet_access_service_0
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );
    service_spec *spec = service_spec_create( name, ( size_t ) ( subkey - name - 2 ), tbl );
    if ( spec && subkey ) {
      service_profile_spec *sp_spec = service_profile_spec_create( name, ( size_t ) ( subkey - name ), spec );
      if ( sp_spec ) {
        service_profile_spec_set( subkey, value, &sp_spec->spec_value );
      }
    }
  }

  return 0;
}


service_manager *
service_manager_initialize( int argc, char **argv, service_manager **sm_ptr ) {
  size_t nitems = 1;

  service_manager_args *args = xcalloc( nitems, sizeof( *args ) );
  args->progname = basename( argv[ 0 ] );
  parse_options( argc, argv, args );

  if ( args->config_fn == NULL || !strlen( args->config_fn ) ) {
    args->config_fn = "service_manager.conf";
  }
  if ( args->schema_fn == NULL || !strlen( args->schema_fn ) ) {
    args->schema_fn = "ipc";
  }
  if ( args->daemonize ) {
    // only check for root priviledges if daemonize
    if ( geteuid() != 0 ) {
      printf( "%s must be run with root privilege.\n", args->progname );
      return NULL;
    }
    if ( pid_file_exists( args->progname ) ) {
      log_err( "Another %s is running", args->progname );
      return NULL;
    }
    bool ret = create_pid_file( args->progname );
    if ( !ret ) {
      log_err( "Failed to create a pid file." );
      return NULL;
    }
  }

  service_manager *self = *sm_ptr;
  self = xcalloc( nitems, sizeof( *self ) );
  // load/parse the service manager configuration file.
  read_config( args->config_fn, handle_config, &self->service_profile_tbl );
  /*
   * read in all the schema information that the service manager would use.
   * Start off with the main schema and proceed to sub schemas.
   */
  self->schema = jedex_initialize( args->schema_fn );
  const char *sc_names[] = {
    "common_reply",
    "ob_add_service_request",
    "ob_del_service_request",
    "service_module_regist_request",
    "service_remove_request",
    "add_service_profile_request",
    "del_service_profile_request",
    "ignite_ssnc_request",
    "shutdown_ssnc_request"
  };

  for ( uint32_t i = 0; i < ARRAY_SIZE( sc_names ); i++ ) {
    self->sub_schema[ i ] = jedex_schema_get_subschema( self->schema, sc_names[ i ] );
    jedex_value_iface *record_class = jedex_generic_class_from_schema( self->sub_schema[ i ] );
    self->rval[ i ] = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
    jedex_generic_value_new( record_class, self->rval[ i ] );
  }
  
  // TODO: add check_ptr_return 

  /*
   * initialize the emirates library. Service manager acts as responder/worker
   * to oss/bss and as a requester/client to service controller and network
   * controller
   */
  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER | REQUESTER ) );
 
  jedex_schema *tmp_schema = sub_schema_find( "ob_add_service_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, OSS_BSS_ADD_SERVICE, tmp_schema, self, oss_bss_add_service_handler );

  tmp_schema = sub_schema_find( "ob_del_service_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, OSS_BSS_DEL_SERVICE, tmp_schema, self, oss_bss_del_service_handler );

  self->emirates->set_service_reply( self->emirates, SERVICE_MODULE_REG, self, service_module_registration_handler );
  self->emirates->set_service_reply( self->emirates, SM_REMOVE_SERVICE, self, service_module_remove_handler );
  // TODO: should rename IGNITE => start
  self->emirates->set_service_reply( self->emirates, SM_IGNITE_SSNC, self, start_service_specific_nc_handler );
  self->emirates->set_service_reply( self->emirates, SM_ADD_SERVICE_PROFILE, self, add_service_profile_handler );
  self->emirates->set_service_reply( self->emirates, SM_DEL_SERVICE_PROFILE, self, del_service_profile_handler );

  set_ready( self->emirates );

  return self;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
