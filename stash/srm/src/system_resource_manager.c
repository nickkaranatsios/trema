/*
 * System Resource Manager
 *
 * The main process of Service Manager
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */


/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <libgen.h>
#include "log_writer.h"
#include "srm_data_pool.h"
#include "ob_transmission.h"
#include "sc_transmission.h"
#include "nc_transmission.h"
#include "daemon.h"

static int IsFinish = 0;


struct {
  bool daemonize;
} config;

static char *program_name = NULL;

enum {
  SUCCEEDED = 0,
  INVALID_ARGUMENT = 1,
  ALREADY_RUNNING = 2,
  OTHER_ERROR = 255,
};

static char short_options[] = "dh";

static struct option long_options[] = {
  { "daemonize", no_argument, NULL, 'd' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: system_resource_manager [OPTION]...\n"
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


/**
 * Service Manager main loop
 */
static void
loc_start() {
  while( IsFinish == 0 ) {
    // no need to do anyting here
    usleep(1);
  }
}


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


int
main( int argc, char *argv[] ) {

  int status = SUCCEEDED;

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

  ret = initialize_system_resource_manager( basename( argv[ 0 ] ) );
  if ( !ret ) {
    status = OTHER_ERROR;
    goto error;
  }

  loc_start();

  finalize_system_resource_manager();
  return status;
error:
  remove_pid_file( basename( argv[ 0 ] ) );
  exit( status );
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
