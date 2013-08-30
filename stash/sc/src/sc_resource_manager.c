/*
 * Service controller resource manager
 *
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


#ifdef LATER
enum {
  SUCCEEDED = 0,
  INVALID_ARGUMENT = 1,
  ALREADY_RUNNING = 2,
  OTHER_ERROR = 255,
};

struct {
  bool daemonize;
} config;

static char *program_name = NULL;

static char short_options[] = "dh";

static struct option long_options[] = {
  { "daemonize", no_argument, NULL, 'd' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: nc_db_manager [OPTION]...\n"
          "  OPTIONS:\n"
          "    -d, --daemonize  Daemonize\n"
          "    -h, --help       Display this help and exit\n"
    );
}



static bool
parse_arguments( int argc, char *argv[] ) {

  config.daemonize = false;

  bool ret = true;
  int c;
  while ( ( c = getopt_long( argc, argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'd':
        config.daemonize = true;
        break;

      case 'h':
        usage();
        exit( SUCCEEDED );
        break;

      default:
        ret &= false;
        break;
    }
  }

  return ret;
}


static bool
initialize_sc_resource_manager( const char *name, sc_resource_manager **sc_rm_ptr ) {
  sc_resource_manager *self;
  size_t nitems = 1;
  uint32_t i;
  jedex_value_iface *record_class;
  int flag = 0;
  jedex_schema *tmp_schema;
  jedex_schema *array_schema;
  jedex_schema **schemas;
  const char *sc_names[] = {
    "common_reply",
    "physical_machine_info",
    "service_module_regist_request",
    "service_remove_request",
    "virtual_machine_allocate_request",
    "service_delete_request",
    "virtual_machine_migrate_request",
    "statistics_status_reply",
    "physical_machine_status",
    "virtual_machine_status",
    "data_plane_port_status",
    "virtual_machine_service_status"
  };

  program_name = strdup( name );

  if ( config.daemonize ) {
    daemonize();
  }

  bool ret = create_pid_file( program_name );
  if ( !ret ) {
    log_err( "Failed to create a pid file." );
    return false;
  }

  // Create jedex,emirates
  self = xcalloc( nitems, sizeof( *self ) );
  self->schema = jedex_initialize( "../../../lib/schema/ipc" );
  for ( i = 0; i < ARRAY_SIZE( sc_names ); ++i ) {
    self->sub_schema[ i ] = jedex_schema_get_subschema( self->schema, sc_names[ i ] );
    record_class = jedex_generic_class_from_schema( self->sub_schema[ i ] );
    self->sub_value[ i ] = ( jedex_value * ) xmalloc( sizeof( jedex_value ) );
    jedex_generic_value_new( record_class, self->sub_value[ i ] );
  }

  self->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER | SUBSCRIBER
                                                               | REQUESTER | PUBLISHER ) );
  if ( NULL == self->emirates ) {
    log_err( "Mssaging interface create failed." );
    exit( EXIT_FAILURE );
  }

#if 0
  tmp_schema = sub_schema_find( "physical_machine_info", self->sub_schema );
  self->emirates->set_subscription( self->emirates, SM_PHYSICAL_MACHINE_INFO,
                                    tmp_schema, self,
                                    physical_machine_information_notification_callback );
#endif
  array_schema = jedex_schema_array( self->schema );
  schemas = ( jedex_schema ** ) zmalloc( sizeof( jedex_schema * ) * 2 + 1 );
  schemas[ 0 ] = self->schema;
  schemas[ 1 ] = array_schema;
  schemas[ 2 ] = NULL;
  self->emirates->set_subscription( self->emirates, SM_PHYSICAL_MACHINE_INFO,
                                    schemas, self->emirates,
                                    physical_machine_information_notification_callback );

  tmp_schema = sub_schema_find( "service_module_regist_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, SM_SERVICE_MODULE_REG_REQUEST,
                                       tmp_schema, self, register_service_request_callback );

  tmp_schema = sub_schema_find( "service_remove_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, SM_REMOVE_SERVICE,
                                       tmp_schema, self, remove_service_request_callback );

  tmp_schema = sub_schema_find( "virtual_machine_allocate_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, SRM_VIRTUAL_MACHINE_ALLOCATE,
                                       tmp_schema, self,
                                       virtual_machine_allocate_request_callback );

  tmp_schema = sub_schema_find( "service_delete_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, SRM_DELETE_SERVICE,
                                       tmp_schema, self, delete_service_request_callback );

  tmp_schema = sub_schema_find( "virtual_machine_migrate_request", self->sub_schema );
  self->emirates->set_service_request( self->emirates, SRM_VM_MIGRATE,
                                       tmp_schema, self,
                                       virtual_machine_migrate_request_callback );

  tmp_schema = NULL;
  self->emirates->set_service_request( self->emirates, SRM_STATISTICS_STATUS,
                                       tmp_schema, self, statistics_status_request_callback );

  // DB get requestの応答に関して、設定(self->emirates->set_service_reply())は必要か？

  set_ready( self->emirates );

  *sc_rm_ptr = self;

  // Initialize process end flag
  init_process_end_flag();
  return ret; 
}

static bool
finalize_sc_resource_manager( sc_resource_manager *self ) {
  // Destory jedex,emirates
  emirates_finalize( &self->emirates );
  jedex_finalize( &self->schema );
  xfree( self );

  bool ret = remove_pid_file( program_name );
  if ( !ret ) {
    log_err( "Failed to remove a pid file." );
    return false;
  }
  if ( program_name != NULL ) {
    free( program_name );
  }
  return true;

}

#endif

/**
 * Resource manager process.
 */
int
main( int argc, char *argv[] ) {
  sc_resource_manager *self = NULL;
  self = sc_resource_manager_initialize( argc, argv, &self );
  if ( self != NULL ) {
    emirates_loop( self->emirates );
  }
#ifdef LATER
  int status = SUCCEEDED;
  int flag = 0;

  if ( geteuid() != 0 ) {
    printf( "%s must be run with root privilege.\n", argv[ 0 ] );
    exit( OTHER_ERROR );
  }

  bool ret = parse_arguments( argc, argv );
  if ( !ret ) {
    usage();
    exit( INVALID_ARGUMENT );
  }

  if ( pid_file_exists( basename( argv[ 0 ] ) ) ) {
    printf( "Another %s is running.\n", basename( argv[ 0 ] ) );
    exit( ALREADY_RUNNING );
  }

  ret = initialize_sc_resource_manager( basename( argv[ 0 ] ), &self );
  if ( !ret ) {
    status = OTHER_ERROR;
    goto error;
  }
  
  // Main loop
  emirates_loop( self->emirates );

  log_emerg( "Resource manager process error end ( error = %d ).", flag );

  finalize_sc_resource_manager( self );
  return status;
error:
  remove_pid_file( basename( argv[ 0 ] ) );
  exit( status );
#endif
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
