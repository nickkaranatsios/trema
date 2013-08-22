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


static uint32_t
ip_address_to_i( const char *ip_address_s ) {
  struct in_addr in_addr;
  uint32_t ip_address;
  
  if ( inet_aton( ip_address_s, &in_addr ) != 0 ) {
    ip_address = ntohl( in_addr.s_addr );
  }
  else {
    ip_address = 0;
  }
  
  return ip_address;
}


#ifdef LATER
static char *
ip_address_to_s( uint32_t ip_address_i ) {
  struct in_addr in_addr;
  
  in_addr.s_addr = htonl( ip_address_i );
  
  return inet_ntoa( in_addr );
}
#endif


static void
pm_spec_set( const char *subkey, const char *value, pm_spec *spec ) {
  int base = 0;
  if ( !prefixcmp( subkey, ".igmp_group_address_format" ) ) {
    spec->igmp_group_address_format = strdup( value );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_start" ) ) {
    spec->igmp_group_address_start = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_end" ) ) {
    spec->igmp_group_address_end = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_format" ) ) {
    spec->vm_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_start" ) ) {
    spec->vm_ip_address_start = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_end" ) ) {
    spec->vm_ip_address_end = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".data_plane_ip_address_format" ) ) {
    spec->data_plane_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".data_plane_mac_address" ) ) {
    spec->data_plane_mac_address = ( uint64_t ) strtoll( value, NULL, base );
  }
}


static void
service_spec_set( const char *subkey, const char *value, service_spec *spec ) {
  int base = 0;

  if ( !prefixcmp( subkey, ".cpu_count" ) ) {
    spec->cpu_count = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".memory_size" ) ) {
    spec->memory_size = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".user_count" ) ) {
    spec->user_count = ( uint32_t ) strtol( value, NULL, base );
  }
}

static void
pm_create( uint32_t ip_address, pm_table *tbl ) {
  ALLOC_GROW( tbl->pms, tbl->pms_nr + 1, tbl->pms_alloc );
  pm *self = xmalloc( sizeof( *self ) );
  self->ip_address = ip_address;
  self->status = 0;
  self->cpu_count = 0;
}


static pm_spec *
pm_spec_create( const char *key, size_t key_len, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    if ( !strncmp( tbl->pm_specs[ i ]->key, key, key_len ) ) {
      return tbl->pm_specs[ i ];
    }
  }
  ALLOC_GROW( tbl->pm_specs, tbl->pm_specs_nr + 1, tbl->pm_specs_alloc );
  size_t nitems = 1;
  pm_spec *spec = xcalloc( nitems, sizeof( *spec ) );
  tbl->cur_selected_pm = ( int ) tbl->pm_specs_nr;
  spec->key = strdup( key );
  spec->key[ key_len ] = '\0';
  spec->ip_address = ip_address_to_i( spec->key );
  tbl->pm_specs[ tbl->pm_specs_nr++ ] = spec;
  pm_create( spec->ip_address, tbl );

  return spec;
}


static service_spec *
service_spec_create( const char *key, size_t key_len, service_class_table *tbl ) {
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


static int
handle_config( const char *key, const char *value, void *user_data ) {
  pm_table *tbl = user_data;
  const char *subkey;
  const char *name;

  // [physical_machine_spec "172.16.4.2"]
  const char *keyword = "physical_machine_spec.";
  if ( !prefixcmp( key, keyword ) ) {
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );

    // for each physical_machine_spec keyword found we create one record
    pm_spec *spec = pm_spec_create( name, ( size_t ) ( subkey - name ), tbl );
    if ( spec && subkey ) {
      pm_spec_set( subkey, value, spec );
    }
  }
  keyword = "class_of_service.";
  if ( !prefixcmp( key, keyword ) ) {
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );
    // select the previous created pm_spec 
    pm_spec *spec = tbl->pm_specs[ tbl->cur_selected_pm ];
    if ( spec && subkey ) {
      service_spec *service_spec = service_spec_create( name, ( size_t ) ( subkey - name ), &spec->service_class_tbl );
      if ( service_spec ) {
        service_spec_set( subkey, value, service_spec );
      }
    }
  }

  return 0;
}


system_resource_manager *
system_resource_manager_initialize( int argc, char **argv, system_resource_manager **srm_ptr ) {
  size_t nitems = 1;

  system_resource_manager_args *args = xcalloc( nitems, sizeof( *args ) );
  args->progname = basename( argv[ 0 ] );
  parse_options( argc, argv, args );

  if ( args->config_fn == NULL || !strlen( args->config_fn ) ) {
    args->config_fn = "srm.conf";
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

  system_resource_manager *self = *srm_ptr;
  self = xcalloc( nitems, sizeof( *self ) );
  
  // load/read the system resource manager configuration file.
  read_config( args->config_fn, handle_config, &self->pm_tbl );
  // reset the cur_selected_pm to undefine value.
  self->pm_tbl.cur_selected_pm = -1;

  /*
   * read in all the schema information that the system resource manager
   * would use. Start off with the main schema and proceed to sub schemas.
   */
  self->schema = jedex_initialize( args->schema_fn );
  const char *sc_names[] = {
    "physical_machine_info",
    "port_info",
    "virtual_machine_info",
    "virtual_machine_allocate_request",
    "service_delete_request",
    "virtual_machine_migrate_request",
    "statistics_status_reply"
  };

  for ( uint32_t i = 0; i < ARRAY_SIZE( sc_names ); i++ ) {
    self->sub_schema[ i ] = jedex_schema_get_subschema( self->schema, sc_names[ i ] );
    jedex_value_iface *record_class = jedex_generic_class_from_schema( self->sub_schema[ i ] );
    self->rval[ i ] = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
    jedex_generic_value_new( record_class, self->rval[ i ] );
  }

  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, PUBLISHER ) );
  self->emirates->set_periodic_timer( self->emirates, 5000, periodic_timer_handler, self );

#ifdef LATER
  jedex_schema *tmp_schema = sub_schema_find( "ob_add_service_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, OSS_BSS_ADD_SERVICE, tmp_schema, self, oss_bss_add_service_handler );

  self->emirates->set_service_reply( self->emirates, SRM_VIRTUAL_MACHINE_ALLOCATE, self, virtual_machine_allocate_handler );
  self->emirates->set_service_reply( self->emirates, SRM_SERVICE_DELETE, self, service_delete_handler );
  self->emirates->set_service_reply( self->emirates, SRM_STATISTICS_STATUS_SERVICE, self, statistics_status_handler );
#endif

  //set_ready( self->emirates );
 
  return self;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
