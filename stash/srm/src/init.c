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


#ifdef LATER
static char *
ip_address_to_s( uint32_t ip_address_i ) {
  struct in_addr in_addr;
  
  in_addr.s_addr = htonl( ip_address_i );
  
  return inet_ntoa( in_addr );
}
#endif


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
    pm_spec *spec = tbl->pm_specs[ tbl->selected_pm ];
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
  // start the selection from the first pm.
  self->pm_tbl.selected_pm = 0;

  /*
   * read in all the schema information that the system resource manager
   * would use. Start off with the main schema and proceed to sub schemas.
   */
  self->schema = jedex_initialize( args->schema_fn );
  const char *sc_names[] = {
    "oss_bss_add_service_request",
    "oss_bss_del_service_request",
    "physical_machine_info",
    "vm_allocate_request",
    "service_delete_request",
    "nc_statistics_status_request",
    "service_delete_request",
    "virtual_machine_migrate_request",
    "statistics_status_reply",
    "common_reply"
  };

  jedex_value_iface *union_class = jedex_generic_class_from_schema( self->schema );
  self->uval = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
  jedex_generic_value_new( union_class, self->uval );

  for ( uint32_t i = 0; i < ARRAY_SIZE( sc_names ); i++ ) {
    self->rval[ i ] = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
    size_t index;
    jedex_value_get_by_name( self->uval, sc_names[ i ], self->rval[ i ], &index );
    self->sub_schema[ i ] = jedex_value_get_schema( self->rval[ i ] );
  }

  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, REQUESTER | RESPONDER | PUBLISHER ) );
  self->emirates->set_periodic_timer( self->emirates, 5000, periodic_timer_handler, self );

  jedex_schema *tmp_schema = sub_schema_find( "oss_bss_add_service_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, OSS_BSS_ADD_SERVICE, tmp_schema, self, oss_bss_add_service_handler );
  tmp_schema = sub_schema_find( "oss_bss_del_service_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, OSS_BSS_DEL_SERVICE, tmp_schema, self, oss_bss_del_service_handler );

  self->emirates->set_service_reply( self->emirates, VM_ALLOCATE, self, vm_allocate_handler );
  self->emirates->set_service_reply( self->emirates, SERVICE_DELETE, self, service_delete_handler );
  self->emirates->set_service_reply( self->emirates, STATS_COLLECT, self, stats_collect_handler );

  set_ready( self->emirates );
 
  return self;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
