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


#include "sc_resource_manager.h"


sc_resource_manager *
sc_resource_manager_initialize( int argc, char **argv, sc_resource_manager **sc_rm_ptr ) {
  size_t nitems = 1;

  sc_resource_manager_args *args = xcalloc( nitems, sizeof( *args ) );
  args->progname = basename( argv[ 0 ] );
  parse_options( argc, argv, args );

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

  sc_resource_manager *self = *sc_rm_ptr;
  self = xcalloc( nitems, sizeof( *self ) );
  self->args = args;

  // connect to redis server
  self->rcontext = redis_cache_connect();
  check_ptr_retval( self->rcontext, NULL, "Connection to redis server failed" );

  self->schema = jedex_initialize( args->schema_fn );
  check_ptr_retval( self->schema, NULL, "Failed to read the main schema" );

  const char *sc_names[] = {
    "common_reply",
    "physical_machine_info",
    "service_module_reg_request",
    "service_remove_request",
    "vm_allocate_request",
    "service_delete_request",
    "vm_migrate_request",
    "statistics_status_reply",
    "physical_machine_status",
    "virtual_machine_status",
    "data_plane_port_status",
    "virtual_machine_service_status"
  };

  jedex_value_iface *union_class = jedex_generic_class_from_schema( self->schema );
  self->uval = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
  jedex_generic_value_new( union_class, self->uval );

  for ( uint32_t i = 0; i < ARRAY_SIZE( sc_names ); i++ ) {
    self->rval[ i ] = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
    jedex_value_get_by_name( self->uval, sc_names[ i ], self->rval[ i ], NULL );
    self->sub_schema[ i ] = jedex_value_get_schema( self->rval[ i ] );
    printf( "schema %s\n", jedex_schema_type_name( jedex_value_get_schema( self->rval[ i ] ) ) );
  }
  self->emirates = emirates_initialize();
  check_ptr_retval( self->emirates, NULL, "Failed to initialize the emirates library" );

  self->schemas = ( jedex_schema ** ) xmalloc( sizeof( jedex_schema * ) * 2 + 1 );
  self->schemas[ 0 ] = sub_schema_find( "physical_machine_info", self->sub_schema );
  self->schemas[ 1 ] = NULL;
  self->emirates->set_subscription( self->emirates,
                                    PM_INFO_PUBLISH, 
                                    self->schemas,
                                    self,
                                    pm_info_handler );
  jedex_schema *tmp_schema = sub_schema_find( "vm_allocate_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates,
                                       VM_ALLOCATE,
                                       tmp_schema,
                                       self,
                                       vm_allocate_handler );

  tmp_schema = sub_schema_find( "service_module_reg_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates,
                                       SERVICE_MODULE_REG,
                                       tmp_schema,
                                       self,
                                       service_module_reg_handler );
  set_ready( self->emirates );

  return self;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
