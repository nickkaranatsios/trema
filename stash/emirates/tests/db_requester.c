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


static emirates_iface *iface;

static char const * const requester_only_usage[] = {
  "  -s --schema_fn=schema file      set the schema full path name to read",
  "  -h --help                       display usage and exit",
  NULL
};
typedef void ( *parse_fn ) ( int option, char *optarg, void *user_data );


static void
save_all_records_handler( uint32_t tx_id, jedex_value *val, const char *json ) {
  if ( val != NULL ) {
    printf( "save all handler called tx_id(%u) %s\n", tx_id, json );
  }
}


static void
set_save_all_records( jedex_value *val ) {
  jedex_value field;
  size_t index;
  
  jedex_value_get_by_name( val, "topic", &field, &index );
  jedex_value_set_string( &field, "groceries" );
}

#define set_record( value, record_type ) \
  set_##record_type( value ) 

#ifdef TEST
static void
my_timer( void *args ) {
  static int i = 0;

  if ( !i ) {
    jedex_value *val = args;
    iface->send_request( iface, "menu", val );
    i = 1;
  }
  printf( "my timer called\n" );
}
#endif


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
  if ( option == 's' ) {
    char *schema = user_data;
    memcpy( schema, optarg, strlen( optarg ) );
    schema[ strlen( optarg ) ] = '\0';
  }
  else {
    printf( "Unrecongnized option\n" );
  }
}


static void
parse_options( int argc, char **argv, parse_fn fn, void *user_data ) {
   const struct option long_options[] = {
    { "schema_fn", required_argument, 0, 's' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 },
  };
  const char *short_options = "s:h";
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


int
main( int argc, char **argv ) {
  char schema_fn[ PATH_MAX ];
  strcpy( schema_fn, "test_schema" );

  parse_options( argc, argv, handle_option, schema_fn );

  jedex_schema *schema = jedex_initialize( schema_fn );

  const char *sub_schemas[] = { NULL };
  jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
  assert( parcel );

  jedex_value *val = jedex_parcel_value( parcel, "" );
  assert( val );

  set_record( val, save_all_records );

  int flag = 0;
  iface = emirates_initialize_only( ENTITY_SET( flag, REQUESTER ) );
  if ( iface != NULL ) {
    iface->set_service_reply( iface, "save_all_records", menu_handler );
    //iface->set_periodic_timer( iface, 5000,  my_timer, val );
    iface->send_request( iface, "save_all_records", val );
    emirates_loop( iface );
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
