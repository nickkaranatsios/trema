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


#ifdef TEST
/**
 * Service Manager process at stop
 */
static void
loc_stop() {
  sm_close_ob_transmission();
  sm_close_sc_transmission();
  sm_close_nc_transmission();
}


/**
 * Service Manager process at receiving signal
 */
static void
loc_sighandler( int signum ) {
  log_info("RECEIVE SIGNAL = (%d) ===> stop application",signum);
  loc_stop();
  IsFinish = 1;
}


/**
 * Service Manager process at start
 */
static bool
loc_initialize() {
  int ret;
  
  IsFinish = 0;
  signal( SIGINT,  loc_sighandler );
  signal( SIGKILL, loc_sighandler );
  signal( SIGTERM, loc_sighandler );
  
  if ( ( ret = sm_load_configuration() ) < 0 ) {
    log_err("CONFIGURATION initialization failed ret=(%d)",ret);
    return false;
  }
  if ( ( ret = sm_init_ob_transmission() ) < 0 ) {
    log_err("OSS/BSS TRANSMISSION initialization failed ret=(%d)",ret);
    return false;
  }
  if ( ( ret = sm_init_sc_transmission() ) < 0 ) {
    log_err("SERVICE CONTROLLER TRANSMISSION initialization failed ret=(%d)",ret);
    return false;
  }
  if ( ( ret = sm_init_nc_transmission() ) < 0 ) {
    log_err("NETWORK CONTROLLER TRANSMISSION initialization failed ret=(%d)",ret);
    return false;
  }

  return true;
}


static bool
initialize_service_manager( sm_args *args ) {
  if ( args->daemonize ) {
    daemonize();
  }

  bool ret = create_pid_file( args->progname );
  if ( !ret ) {
    log_err( "Failed to create a pid file." );
    return false;
  }
  if ( ( ret = loc_initialize() ) < 0 ) {
    log_err( "INITIALIZATION failed ret=(%d)", ret );
    loc_stop();
    return false;
  }

  return true;
}


static bool
finalize_service_manager() {
  loc_stop();
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


int
main( int argc, char *argv[] ) {
#ifdef TEST
  if ( geteuid() != 0 ) {
    printf( "%s must be run with root privilege.\n", argv[ 0 ] );
    exit( OTHER_ERROR );
  }
#endif
  service_manager *self = NULL;
  self = service_manager_initialize( argc, argv, &self ); 
  if ( self != NULL ) {
    emirates_loop( self->emirates );
  }
  else {
    log_err( "Initialization failed exiting ..." );
  }
#ifdef TEST
  args->progname = basename( argv[ 0 ] );
  parse_options( argc, argv, args );

  if ( pid_file_exists( args ) ) {
    printf( "Another %s is running.\n", args->progname );
    exit( ALREADY_RUNNING );
  }

  ret = initialize_service_manager( args );
  if ( !ret ) {
    status = OTHER_ERROR;
    goto error;
  }
  xfree( args );

  service_manager_start();

  finalize_service_manager();
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
