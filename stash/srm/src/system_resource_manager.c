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

#ifdef TEST
/**
 * Service Manager process at stop
 */
static void
loc_stop() {
  srm_close_ob_transmission();
  srm_close_sc_transmission();
  srm_close_nc_transmission();
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
static int
loc_initialize() {
  int ret;
  
  IsFinish = 0;
  signal(SIGINT,  loc_sighandler);
  signal(SIGKILL, loc_sighandler);
  signal(SIGTERM, loc_sighandler);
  
  if ( ( ret = srm_load_configuration() ) < 0 ) {
    log_err("CONFIGURATION initialization failed ret=(%d)",ret);
    return -1;
  }
  if ( ( ret = srm_init_ob_transmission() ) < 0 ) {
    log_err("OSS/BSS TRANSMISSION initialization failed ret=(%d)",ret);
    return -1;
  }
  if ( ( ret = srm_init_sc_transmission() ) < 0 ) {
    log_err("SERVICE CONTROLLER TRANSMISSION initialization failed ret=(%d)",ret);
    return -1;
  }
  if ( ( ret = srm_init_nc_transmission() ) < 0 ) {
    log_err("NETWORK CONTROLLER TRANSMISSION initialization failed ret=(%d)",ret);
    return -1;
  }

  return 0;
}

static bool
initialize_system_resource_manager( const char *name ) {
  program_name = strdup( name );

  if ( config.daemonize ) {
    daemonize();
  }

  bool ret = create_pid_file( program_name );
  if ( !ret ) {
    log_err( "Failed to create a pid file." );
    return false;
  }

  if ( ( ret = loc_initialize() ) < 0 ) {
    log_err("INITIALIZATION failed ret=(%d)",ret);
    loc_stop();
    return false;
  }
  return true;
}


static bool
finalize_system_resource_manager() {
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
  system_resource_manager *self = NULL;
  self = system_resource_manager_initialize( argc, argv, &self ); 
  if ( self != NULL ) {
    emirates_loop( self->emirates );
  }
  else {
    log_err( "Initialization failed exiting ..." );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
