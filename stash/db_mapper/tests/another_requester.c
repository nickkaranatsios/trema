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
#include <hiredis/hiredis.h>
#include "emirates.h"
#include "jedex_iface.h"
#include "generic.h"
#include "cache.h"
#include "checks.h"


typedef struct db_requester {
  emirates_iface *iface;
  redisContext *rcontext;
  jedex_schema *request_reply_schema;
  jedex_schema *sc_db_schema;
} db_requester;


static char const * const requester_only_usage[] = {
  "  -s --schema_fn=schema file      set the schema full path name to read",
  "  -d --data_schema_fn=schema file set the data schema full path name to read",
  "  -h --help                       display usage and exit",
  NULL
};
typedef void ( *parse_fn ) ( int option, char *optarg, void *user_data );


static jedex_schema *
physical_machine_statuses_schema_get( jedex_schema *schema ) {
  return jedex_schema_get_subschema( schema, "physical_machine_statuses" );
}


static jedex_schema *
data_plane_port_statuses_schema_get( jedex_schema *schema ) {
  return jedex_schema_get_subschema( schema, "data_plane_port_statuses" );
}


static jedex_schema *
virtual_machine_statuses_schema_get( jedex_schema *schema ) {
  return jedex_schema_get_subschema( schema, "virtual_machine_statuses" );
}


static jedex_schema *
reply_schema_get( jedex_schema *schema ) {
  return jedex_schema_get_subschema( schema, "db_mapper_reply" );
}


static void
print_groceries( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );

  const char *cstr = NULL;
  size_t size = 0;
  jedex_value_get_string( &field, &cstr, &size );
  printf( "name: %s\n", cstr );

  jedex_value_get_by_name( val, "country", &field, &index );
  jedex_value_get_string( &field, &cstr, &size );
  printf( "country: %s\n", cstr );

  jedex_value_get_by_name( val, "price", &field, &index );
  float price;
  jedex_value_get_float( &field, &price );
  printf( "price %10.2f\n", price );
}


static int
print_groceries_array( jedex_value *val ) {
  jedex_value element;
  size_t array_size;
  jedex_value_get_size( val, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( val, i, &element, NULL );
    print_groceries( &element );
    return 0;
  }

  return -1;
}


static void
print_db_reply( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "error", &field, &index );
  int error;
  jedex_value_get_int( &field, &error );
  printf( "error %d\n", error );
}


static void
set_physical_machine_status( jedex_value *val, int pm_id ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "physical_machine_id", &field, &index );
  jedex_value_set_int( &field, pm_id );
  jedex_value_get_by_name( val, "physical_ip_address", &field, &index );
  jedex_value_set_int( &field, 168430081 );
  jedex_value_get_by_name( val, "total_memory", &field, &index );
  jedex_value_set_int( &field, 16 );
  jedex_value_get_by_name( val, "available_memory", &field, &index );
  jedex_value_set_int( &field, 10 );
  jedex_value_get_by_name( val, "port_count", &field, &index );
  jedex_value_set_int( &field, 2 );
  jedex_value_get_by_name( val, "cpu_count", &field, &index );
  jedex_value_set_int( &field, 4 );
  jedex_value_get_by_name( val, "cpu0_load", &field, &index );
  jedex_value_set_double( &field, 11.22 );
  jedex_value_get_by_name( val, "cpu1_load", &field, &index );
  jedex_value_set_double( &field, 22.32 );
  jedex_value_get_by_name( val, "cpu2_load", &field, &index );
  jedex_value_set_double( &field, 32.42 );
  jedex_value_get_by_name( val, "cpu3_load", &field, &index );
  jedex_value_set_double( &field, 42.52 );
  jedex_value_get_by_name( val, "virtual_machine_count", &field, &index );
  jedex_value_set_int( &field, 0 );
}


static void
set_physical_machine_statuses( jedex_value *val ) {
  jedex_value element;
  jedex_value_append( val, &element, NULL );
  set_physical_machine_status( &element, 1 );
}


static jedex_schema *
jedex_schema_physical_machine_statuses( jedex_schema *schema ) {
  jedex_schema *pm_statuses_schema = physical_machine_statuses_schema_get( schema );
  assert( pm_statuses_schema );

  return jedex_schema_array( pm_statuses_schema );
}
  

static void
insert_publish_data( emirates_iface *iface, jedex_schema *schema ) {
  jedex_schema *array_schema = jedex_schema_physical_machine_statuses( schema );

  jedex_value_iface *array_class = jedex_generic_class_from_schema( array_schema );
  assert( array_class );
  
  jedex_value val;
  jedex_generic_value_new( array_class, &val );
  set_physical_machine_statuses( &val );
  publish_value( iface, "save_scdb", &val );
}


static void
save_topic_handler( const uint32_t tx_id,
                    jedex_value *val,
                    const char *json,
                    void *user_data ) {
  if ( val != NULL ) {
    printf( "save_topic_handler called tx_id(%u) %s\n", tx_id, json );
    print_db_reply( val );
    db_requester *db_req = user_data;
    insert_publish_data( db_req->iface, db_req->sc_db_schema );
    //jedex_value *find_record_val = set_find_record( schema );

    //iface->send_request( iface, "find_record", find_record_val, array_schema );
  }
}


static void
find_record_handler( const uint32_t tx_id,
                     jedex_value *val,
                     const char *json,
                     void *user_data ) {
  UNUSED( user_data );
  // we expect the "val" pointer to be array of fruits.
  if ( val != NULL ) {
     printf( "find_record_handler called tx_id(%u) %s\n", tx_id, json );
     print_groceries_array( val );
  }
}



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
    char **schema = user_data;
    memcpy( schema[ 0 ], optarg, strlen( optarg ) );
    schema[ 0 ][ strlen( optarg ) ] = '\0';
  }
  else if ( option == 'd' ) {
    char **schema = user_data;
    memcpy( schema[ 1 ], optarg, strlen( optarg ) );
    schema[ 1 ][ strlen( optarg ) ] = '\0';
  }
  else {
    printf( "Unrecongnized option\n" );
  }
}


static void
parse_options( int argc, char **argv, parse_fn fn, void *user_data ) {
   const struct option long_options[] = {
    { "schema_fn", required_argument, 0, 's' },
    { "data_schema_fn", required_argument, 0, 'd' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 },
  };
  const char *short_options = "s:d:h";
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
set_save_topic( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "dbname", &field, &index );
  jedex_value_set_string( &field, "scdb" );
  jedex_value_get_by_name( val, "topic", &field, &index );
  jedex_value_set_string( &field, "save_scdb" );
  jedex_value_get_by_name( val, "tables", &field, &index );
  jedex_value element; 
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "data_plane_port_statuses" );
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "physical_machine_statuses" );
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "virtual_machine_statuses" );
}


static jedex_value *
set_topic( jedex_schema *schema ) {
  jedex_value_iface *val_iface;
  
  jedex_schema *request_schema = jedex_schema_get_subschema( schema, "save_all" );
  val_iface = jedex_generic_class_from_schema( request_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  set_save_topic( val ); 

  return val;
}


/*
 * This test program is used in addition to db_requester test program
 * to ensure that db_mapper can handle more than one find_record service
 * requests originating from different processes.:wq
 */
int
main( int argc, char **argv ) {
  char schema_fn[ 2 ][ PATH_MAX ];
  strcpy( &schema_fn[ 0 ][ 0 ], "request_reply" );
  strcpy( &schema_fn[ 1 ][ 0 ], "sc_db" );

  parse_options( argc, argv, handle_option, &schema_fn[ 0 ] );

  db_requester *db_req = ( db_requester * ) malloc( sizeof( db_requester ) );

  db_req->request_reply_schema = jedex_initialize( schema_fn[ 0 ] );
  db_req->sc_db_schema = jedex_initialize( schema_fn[ 1 ] );

  jedex_value *val = set_topic( db_req->request_reply_schema );

  int flag = 0;
  db_req->iface = emirates_initialize_only( ENTITY_SET( flag, REQUESTER | PUBLISHER ) );
  if ( db_req->iface != NULL ) {
    db_req->rcontext = redis_cache_connect();
    if ( db_req->rcontext == NULL ) {
      printf( "Failed to connect to cache exiting ...\n" );
      return -1;
    }
    db_req->iface->set_service_reply( db_req->iface, "save_topic", db_req, save_topic_handler );
    jedex_schema *reply_schema = reply_schema_get( db_req->request_reply_schema );
    db_req->iface->send_request( db_req->iface, "save_topic", val, reply_schema );

#ifdef LATER
    db_req->iface->set_service_reply( db_req->iface, "find_record", db_req, find_record_handler );

    jedex_value *find_record_val = set_find_record( db_req->sc_db_schema );
    jedex_schema *array_schema = jedex_schema_fruits( db_req->sc_db_schema );
    db_req->iface->send_request( db_req->iface, "find_record", find_record_val, array_schema );
#endif

    emirates_loop( db_req->iface );
    emirates_finalize( &db_req->iface );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
