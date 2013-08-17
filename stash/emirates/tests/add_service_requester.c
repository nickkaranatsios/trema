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


#include <getopt.h>
#include "emirates.h"
#include "jedex_iface.h"
#include "checks.h"


typedef struct prog_args {
  char schema_fn[ PATH_MAX ];
  char service[ SERVICE_MAX ];
  double bandwidth;
  uint64_t nr_subscribers;
} prog_args;

static emirates_iface *iface;

static char const * const requester_only_usage[] = {
  " -f --schema_fn=schema file      set the schema full path name to read",
  " -s --service=service name       set the service name to add",
  " -b --bandwidth=bandwidth        set the bandwidth for the service",
  " -u --subscribers=subscribers    set the no.of subscribers for the service",
  "  -h --help                       display usage and exit",
  NULL
};
typedef void ( *parse_fn ) ( int option, char *optarg, void *user_data );


static void
print_usage( const char *progname, int exit_code ) {
  fprintf( stderr, "Usage: %s [options] \n", progname );

  int i = 0;
  while ( requester_only_usage[ i ] ) {
    fprintf( stderr, "%s\n", requester_only_usage[ i++ ] );
  }

  exit( exit_code );
}


void
handle_option( int option, char *optarg, void *user_data ) {
  prog_args *args = user_data;

  if ( option == 'f' ) {
    memcpy( args->schema_fn, optarg, strlen( optarg ) );
    args->schema_fn[ strlen( optarg ) ] = '\0';
  }
  else if ( option == 's' ) {
    memcpy( args->service, optarg, strlen( optarg ) );
    args->service[ strlen( optarg ) ] = '\0';
  }
  else if ( option == 'b' ) {
    args->bandwidth = strtod( optarg, NULL );
  }
  else if ( option == 'u' ) {
    int base = 10;
    args->nr_subscribers = strtol( optarg, NULL, base );
  }
  else {
    printf( "Unrecongnized option\n" );
  }
}


static void
parse_options( int argc, char **argv, parse_fn fn, void *user_data ) {
   const struct option long_options[] = {
    { "schema_fn", required_argument, 0, 'f' },
    { "service", required_argument, 0, 's' },
    { "bandwidth", required_argument, 0, 'b' },
    { "subscribers", required_argument, 0, 'u' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 },
  };
  const char *short_options = "f:s:b:u:h";
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
      fn( c, optarg, user_data );
    }
  }
}


static void
set_add_service_value( prog_args *args, jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "service", &field, &index );
  jedex_value_set_string( &field, args->service );
  jedex_value_get_by_name( val, "bandwidth", &field, &index );
  jedex_value_set_double( &field, args->bandwidth );
  jedex_value_get_by_name( val, "nr_subscribers", &field, &index );
  jedex_value_set_long( &field, ( int64_t ) args->nr_subscribers ); 
}


static void
print_add_service( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "service", &field, &index );
  const char *cstr = NULL;
  size_t size;
  jedex_value_get_string( &field, &cstr, &size );
  printf( "service: %s\n", cstr );
  jedex_value_get_by_name( val, "bandwidth", &field, &index );
  double bandwidth;
  jedex_value_get_double( &field, &bandwidth );
  printf( "bandwidth: %10.2f\n", bandwidth );
  jedex_value_get_by_name( val, "nr_subscribers", &field, &index );
  int64_t nr_subscribers;
  jedex_value_get_long( &field, &nr_subscribers );
  printf( "nr_subscribers: %lld\n", nr_subscribers );
}


static void
add_service_handler( uint32_t tx_id, jedex_value *val, const char *json, void *user_data ) {
  UNUSED( user_data );
  if ( val != NULL ) {
    printf( "add_service handler called tx_id(%u) %s\n", tx_id, json );
  }
}


/*
 * This test program sends two add_service requests one to system resource
 * manager(srm) and another to service manager(sm). This is to simulate the
 * oss/bss requests currently send from cgi_handler.
 */
int
main( int argc, char **argv ) {
  prog_args args;

  strcpy( args.schema_fn, "add_service" ); 
  parse_options( argc, argv, handle_option, &args );

  jedex_schema *schema = jedex_initialize( args.schema_fn );
  const char *sub_schema[] = { "add_service", "add_service_reply", NULL };
  jedex_schema *add_service_schema = jedex_schema_get_subschema( schema, sub_schema[ 0 ] );
  jedex_value_iface *add_service_class = jedex_generic_class_from_schema( add_service_schema );

  jedex_value val;
  jedex_generic_value_new( add_service_class, &val );

  set_add_service_value( &args, &val );
  print_add_service( &val );

  int flag = 0;
  iface = emirates_initialize_only( ENTITY_SET( flag, REQUESTER ) );
  if ( iface != NULL ) {
    iface->set_service_reply( iface, "add_service", NULL, add_service_handler );
    jedex_schema *add_service_reply_schema = jedex_schema_get_subschema( schema, sub_schema[ 1 ] );
    iface->send_request( iface, "add_service_srm", &val, add_service_reply_schema );
    iface->send_request( iface, "add_service_sm", &val, add_service_reply_schema );

    emirates_loop( iface );
    jedex_value_decref( &val );
    jedex_finalize( &schema );
    emirates_finalize( &iface );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
