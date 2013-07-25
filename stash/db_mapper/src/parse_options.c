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
  "  -m --db_host=db_host                       set db host to connect to",
  "  -u --db_user=db_user                       set db user to login in",
  "  -p --db_passwd=db_passwd                   set db password to login in",
  "  -b --db_name=db                            set db name to connect to",
  "  -h --help                                  display usage and exit",
  NULL
};


static void
print_usage( const mapper_args *args, int exit_code ) {
  fprintf( stderr, "Usage: %s [options] \n", args->progname );

  int i = 0;
  while ( mapper_usage[ i ] ) {
    fprintf( stderr, "%s\n", mapper_usage[ i++ ] );
  }

  exit( exit_code );
}


static void
set_default_opts( mapper_args *args, const struct option long_options[] ) {
  args->progname = "db_mapper";
  args->db_host = "localhost";
  args->db_user = "";
  args->db_passwd = "";
  args->db_name = "test";
  args->db_socket = "/var/run/mysqld/mysqld.sock";
  args->options = long_options;
}


void
parse_options( mapper_args *args, int argc, char **argv ) {
  static struct option long_options[] = {
    { "logging_level", required_argument, 0, 'l' },
    { "daemonize", no_argument, 0, 'd' },
    { "db_host", required_argument, 0, 'm' },
    { "db_user", required_argument, 0, 'u' },
    { "db_passwd", required_argument, 0, 'p' },
    { "db_name", required_argument, 0, 'b' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 },
  };
  static const char *short_options = "l:dm:u:p:b:h";
  set_default_opts( args, long_options );

  int c, index = 0;
  optind = 0;
  while ( 1 ) {
    c = getopt_long( argc, argv, short_options, args->options, &index );
    if ( c == -1 ) {
      break;
    }
    switch ( c ) {
      case 'h':
        print_usage( args, 0 );
      break;
      case 'l':
        if ( optarg ) {
          args->log_level = optarg;
        }
      break;
      case 'd':
        args->run_as_daemon = true;
      case 'm':
        if ( optarg ) {
          args->db_host = optarg;
        }
      break;
      case 'u':
        if ( optarg ) {
          args->db_user = optarg;
        }
      break;
      case 'p':
        if ( optarg ) {
          args->db_passwd = optarg;
        }
      break;
      case 'b':
        if ( optarg ) {
          args->db_name = optarg;
        }
      break;
    }
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
