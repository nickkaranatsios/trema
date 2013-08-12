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
  jedex_schema *groceries_schema;
} db_requester;


static char const * const requester_only_usage[] = {
  "  -s --schema_fn=schema file      set the schema full path name to read",
  "  -d --data_schema_fn=schema file set the data schema full path name to read",
  "  -h --help                       display usage and exit",
  NULL
};
typedef void ( *parse_fn ) ( int option, char *optarg, void *user_data );


static jedex_schema *
fruits_schema_get( jedex_schema *schema ) {
  return jedex_schema_get_subschema( schema, "fruits" );
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
set_fruits( jedex_value *val, const char *name, const char *country, float price ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  jedex_value_set_string( &field, name );
  jedex_value_get_by_name( val, "country", &field, &index );
  jedex_value_set_string( &field, country );
  jedex_value_get_by_name( val, "price", &field, &index );
  jedex_value_set_float( &field, price );
}


static void
set_many_fruits( jedex_value *val ) {
  jedex_value element;
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "jackfruit", "Thailand", 5.38f );
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "pineapple", "Thailand", 2.66f );
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "mango", "Malaysia", 3.25f );
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "apples", "Japan", 1.55f );
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "bananas", "Mexico", 0.85f );
}


static jedex_value *
set_find_record( jedex_schema *schema ) {
  jedex_value_iface *val_iface;

  jedex_schema *fruits_schema = fruits_schema_get( schema );
  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  set_fruits( val, "", "Thailand", 0.0f );

  return val;
}


static jedex_value *
set_find_all_records( jedex_schema *schema ) {
  jedex_value_iface *val_iface;

  jedex_schema *fruits_schema = fruits_schema_get( schema );
  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );

  return val;
}


static jedex_schema *
jedex_schema_fruits( jedex_schema *schema ) {
  jedex_schema *fruits_schema = fruits_schema_get( schema );
  assert( fruits_schema );

  return jedex_schema_array( fruits_schema );
}
  

static void
insert_publish_data( emirates_iface *iface, jedex_schema *schema ) {
  jedex_schema *array_schema = jedex_schema_fruits( schema );

  jedex_value_iface *array_class = jedex_generic_class_from_schema( array_schema );
  assert( array_class );
  
  jedex_value val;
  jedex_generic_value_new( array_class, &val );
  set_many_fruits( &val );
  publish_value( iface, "save_groceries", &val );
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
    insert_publish_data( db_req->iface, db_req->groceries_schema );
    //jedex_value *find_record_val = set_find_record( schema );

    jedex_schema *array_schema = jedex_schema_fruits( db_req->groceries_schema );
    
    //iface->send_request( iface, "find_record", find_record_val, array_schema );
    jedex_value *find_all_records_val = set_find_all_records( db_req->groceries_schema );
    db_req->iface->send_request( db_req->iface, "find_all_records", find_all_records_val, array_schema ); 
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
find_next_record_handler( const uint32_t tx_id,
                          jedex_value *val,
                          const char *json,
                          void *user_data ) {
  if ( val != NULL ) {
    printf( "find_next_record_handler called tx_id(%u) %s\n", tx_id, json );
    db_requester *db_req = user_data;
    if ( !print_groceries_array( val ) ) {
      jedex_value *find_next_record_val = set_find_all_records( db_req->groceries_schema );
      jedex_schema *array_schema = jedex_schema_fruits( db_req->groceries_schema );
      db_req->iface->send_request( db_req->iface, "find_next_record", find_next_record_val, array_schema ); 
    }
    else {
      redisReply *reply = redis_cache_get( db_req->rcontext, "fruits|name|jackfruit|country|Thailand" );
      if ( reply != NULL ) {
        jedex_value *update_val = json_to_jedex_value( fruits_schema_get( db_req->groceries_schema ), reply->str );
        assert( update_val );
        printf( "redis json data %s\n", reply->str );
        freeReplyObject( reply );

        jedex_value field;
        size_t index;
        jedex_value_get_by_name( update_val, "price", &field, &index );
        float price = 4.44f;
        jedex_value_set_float( &field, price );
        db_req->iface->send_request( db_req->iface, "update_record", update_val, reply_schema_get( db_req->request_reply_schema ) );
      }
    }
  }
}


static void
find_all_records_handler( const uint32_t tx_id,
                          jedex_value *val,
                          const char *json,
                          void *user_data ) {
  if ( val != NULL ) {
    printf( "find_all_records_handler called tx_id(%u) %s\n", tx_id, json );
    db_requester *db_req = user_data;
    print_groceries_array( val );

    jedex_value *find_next_record_val = set_find_all_records( db_req->groceries_schema );
    jedex_schema *array_schema = jedex_schema_fruits( db_req->groceries_schema );
    db_req->iface->send_request( db_req->iface, "find_next_record", find_next_record_val, array_schema ); 
  }
}


static void
update_record_handler( const uint32_t tx_id,
                       jedex_value *val,
                       const char *json,
                       void *user_data ) {
  if ( val != NULL ) {
    printf( "update_record_handler called tx_id(%u) %s\n", tx_id, json );
    db_requester *db_req = user_data;
    redisReply *reply = redis_cache_get( db_req->rcontext, "fruits|name|apples|country|Japan" );
    if ( reply != NULL ) {
      jedex_value *delete_val = json_to_jedex_value( fruits_schema_get( db_req->groceries_schema ), reply->str );
      assert( delete_val );
      printf( "redis json data %s\n", reply->str );
      freeReplyObject( reply );

      db_req->iface->send_request( db_req->iface, "delete_record", delete_val, reply_schema_get( db_req->request_reply_schema ) );
    }
  }
}


static void
delete_record_handler( const uint32_t tx_id,
                       jedex_value *val,
                       const char *json,
                       void *user_data ) {
  if ( val != NULL ) {
    printf( "delete_record_handler called tx_id(%u) %s\n", tx_id, json );
    db_requester *db_req = user_data;
    UNUSED( db_req );
    print_db_reply( val );
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
  jedex_value_set_string( &field, "groceries" );
  jedex_value_get_by_name( val, "topic", &field, &index );
  jedex_value_set_string( &field, "save_groceries" );
  jedex_value_get_by_name( val, "tables", &field, &index );
  jedex_value element; 
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "fruits" );
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
  strcpy( &schema_fn[ 1 ][ 0 ], "groceries" );

  parse_options( argc, argv, handle_option, &schema_fn[ 0 ] );

  db_requester *db_req = ( db_requester * ) malloc( sizeof( db_requester ) );

  db_req->request_reply_schema = jedex_initialize( schema_fn[ 0 ] );
  db_req->groceries_schema = jedex_initialize( schema_fn[ 1 ] );

  int flag = 0;
  db_req->iface = emirates_initialize_only( ENTITY_SET( flag, REQUESTER ) );
  if ( db_req->iface != NULL ) {
    db_req->rcontext = redis_cache_connect();
    if ( db_req->rcontext == NULL ) {
      printf( "Failed to connect to cache exiting ...\n" );
      return -1;
    }
    db_req->iface->set_service_reply( db_req->iface, "find_record", db_req, find_record_handler );

    jedex_value *find_record_val = set_find_record( db_req->groceries_schema );
    jedex_schema *array_schema = jedex_schema_fruits( db_req->groceries_schema );
    db_req->iface->send_request( db_req->iface, "find_record", find_record_val, array_schema );

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
