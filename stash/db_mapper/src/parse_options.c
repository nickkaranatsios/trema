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


#include "db_mapper.h"


static char const * const mapper_usage[] = {
  "  -l --logging_level=level                   set the logging level",
  "  -d --daemonize                             run as a daemon",
  "  -c --config_fn=config_fn                   set config fn to read",
  "  -s --schema_fn=schema_fn                   set schema fn to read",
  "  -h --help                                  display usage and exit",
  NULL
};


static void
print_usage( const char *progname, int exit_code ) {
  fprintf( stderr, "Usage: %s [options] \n", progname );

  int i = 0;
  while ( mapper_usage[ i ] ) {
    fprintf( stderr, "%s\n", mapper_usage[ i++ ] );
  }

  exit( exit_code );
}


static void
handle_option( int c, char *optarg, void *user_data ) {
  mapper_args *args = user_data;

  switch( c ) {
    case 'l':
      if ( optarg ) {
        args->log_level = optarg;
      }
    break;
    case 'd':
      args->run_as_daemon = true;
    break;
    case 'c':
      if ( optarg ) {
        args->config_fn = optarg;
      }
    break;
    case 's':
      if ( optarg ) {
        args->schema_fn = optarg;
      }
    break;
    default:
      log_err( "Unrecognized option skip" );
    break;
  }
}


void
parse_options( int argc, char **argv, void *user_data ) {
  const struct option long_options[] = {
    { "logging_level", required_argument, 0, 'l' },
    { "daemonize", no_argument, 0, 'd' },
    { "config_fn", required_argument, 0, 'c' },
    { "schema_fn", required_argument, 0, 's' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 },
  };
  const char *short_options = "l:dc:s:h";

  int c;
  int index = 0;
  optind = 0;
  while ( 1 ) {
    c = getopt_long( argc, argv, short_options, long_options, &index );
    if ( c == -1 ) {
      break;
    }
    if ( c == 'h' ) {
      print_usage( argv[ 0 ], 0 );
    }
    else {
      handle_option( c, optarg, user_data );
    }
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */