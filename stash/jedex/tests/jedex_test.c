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


#include "jedex.h"
#include "jedex_iface.h"


static void
set_simple_array( jedex_value *val ) {
  jedex_value element;

  jedex_value_append( val, &element, NULL );
  jedex_value_set_int( &element, 1 );
  jedex_value_append( val, &element, NULL );
  jedex_value_set_int( &element, 2 );
}


static void
get_simple_array( jedex_value *val ) {
  jedex_value element;
  size_t array_size;

  jedex_value_get_size( val, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( val, i, &element, NULL );
    int ret_val;
    jedex_value_get_int( &element, &ret_val );
    printf( "int(%zu)= %d\n", i, ret_val );
  }
}


static void
set_menu_record_value( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "header", &field, &index );
  jedex_value_set_string( &field, "Save us menu" );
  jedex_value_get_by_name( val, "items", &field, &index );
  jedex_value element;
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "save the children" );
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "save the world" );
}


static void
set_menu_array( jedex_value *val ) {
  jedex_value element;

  jedex_value_append( val, &element, NULL );
  set_menu_record_value( &element );
  jedex_value_append( val, &element, NULL );
  set_menu_record_value( &element );
}


static void
get_menu_record_value( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "header", &field, &index );
  const char *cstr = NULL;
  size_t size = 0;
  jedex_value_get_string( &field, &cstr, &size );
  printf( "header: %s\n", cstr );

  jedex_value_get_by_name( val, "items", &field, &index );
  jedex_value element;
  size_t array_size;
  jedex_value_get_size( &field, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );
    jedex_value_get_string( &element, &cstr, &size );
    printf( "item(%zu): %s\n", i, cstr );
  }
}
  

static void
get_menu_array( jedex_value *val ) {
  jedex_value element;
  size_t array_size;
  jedex_value_get_size( val, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( val, i, &element, NULL );
    get_menu_record_value( &element );
  }
}


static void
set_menu_union_value( jedex_value *val ) {
  jedex_value branch;
  size_t branch_count;

  jedex_value_get_size( val, &branch_count );
  assert( branch_count == 2 );

  size_t index;
  jedex_value_get_by_name( val, "array", &branch, &index );
  jedex_value element;
  jedex_value_append( &branch, &element, NULL );
  set_menu_record_value( &element );
  jedex_value_append( &branch, &element, NULL );
  set_menu_record_value( &element );
}


static void
get_menu_union_value( jedex_value *val ) {
  jedex_value branch;
  size_t index;
  jedex_value_get_by_name( val, "array", &branch, &index );
  size_t array_size;
  jedex_value_get_size( &branch, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value menu_val;
    jedex_value_get_by_index( &branch, i, &menu_val, NULL );
    get_menu_record_value( &menu_val );
  }
}


static void
set_vegetables( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  assert( index == 0 );
  jedex_value_set_string( &field, "brocoli" );
  jedex_value_get_by_name( val, "price", &field, &index );
  assert( index == 1 );
  float price = 2.45f;
  jedex_value_set_float( &field, price );
}


static void
set_union_vegetables( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "vegetables", &branch, &index );
  set_vegetables( &branch );
}


static void
get_vegetables( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "name", &field, &index );

  const char *cstr = NULL;
  size_t size = 0;
  jedex_value_get_string( &field, &cstr, &size );
  printf( "name: %s\n", cstr );

  jedex_value_get_by_name( val, "price", &field, &index );
  float price;
  jedex_value_get_float( &field, &price );
  printf( "price %10.2f\n", price );
}


static void
get_union_vegetables( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "vegetables", &branch, &index );
  get_vegetables( &branch );
}

  
static void
set_meat( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  assert( index == 0 );
  jedex_value_set_string( &field, "hamburger steak" );
  jedex_value_get_by_name( val, "price", &field, &index );
  assert( index == 1 );
  float price = 11.45f;
  jedex_value_set_float( &field, price );
}


static void
set_fruits( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  //assert( index == 0 );
  jedex_value_set_string( &field, "mango" );
  jedex_value_get_by_name( val, "price", &field, &index );
  //assert( index == 1 );
  float price = 2.40f;
  jedex_value_set_float( &field, price );
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

  jedex_value_get_by_name( val, "price", &field, &index );
  float price;
  jedex_value_get_float( &field, &price );
  printf( "price %10.2f\n", price );
}


static void
get_meat( jedex_value *val ) {
  print_groceries( val );
}


static void
get_fruits( jedex_value *val ) {
  print_groceries( val );
}


static void
get_union_meat( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "meat", &branch, &index );
  get_meat( &branch );
}


static void
set_union_meat( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "meat", &branch, &index );
  set_meat( &branch );
}


static void
set_first_record( jedex_value *val ) {
  jedex_value field;
  size_t index;
  
  jedex_value_get_by_name( val, "attribute", &field, &index );
  jedex_value_set_string( &field, "a read only attribute" );
}


static void
set_second_record( jedex_value *val ) {
  jedex_value field;
  size_t index;
  
  jedex_value_get_by_name( val, "value", &field, &index );
  jedex_value_set_int( &field, 1 );
  jedex_value_get_by_name( val, "other", &field, &index );
  jedex_value array_element;
  jedex_value_append( &field, &array_element, NULL );
  set_first_record( &array_element );
  jedex_value_append( &field, &array_element, NULL );
  set_first_record( &array_element );
}


static void
set_ref_to_another( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "second_record", &branch, &index );
  set_second_record( &branch );
}


static void
set_db_record( jedex_value *val ) {
  jedex_value branch;
  size_t index;
  
  jedex_value_get_by_name( val, "fruits", &branch, &index );
  jedex_value db_header_record;

  jedex_value_get_by_name( &branch, "header", &db_header_record, &index );
  jedex_value field;
  jedex_value_get_by_name( &db_header_record, "table_name", &field, &index );
  jedex_value_set_string( &field, "fruits" );
  jedex_value_get_by_name( &db_header_record, "table_size", &field, &index );
  jedex_value_set_long( &field, 12345 );
  
  
  set_fruits( &branch );
}


static void
get_db_record( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "fruits", &branch, &index );
  jedex_value db_header_record;

  jedex_value_get_by_name( &branch, "header", &db_header_record, &index );
  jedex_value field;
  jedex_value_get_by_name( &db_header_record, "table_name", &field, &index );
  
  const char *cstr = NULL;
  size_t size = 0;
  jedex_value_get_string( &field, &cstr, &size );
  printf( "table_name: %s\n", cstr );

  jedex_value_get_by_name( &db_header_record, "table_size", &field, &index );
  int64_t table_size;
  jedex_value_get_long( &field, &table_size );
  printf( "table_size(%zu) : %lld\n", index, table_size );
  get_fruits( &branch );
}


static void
get_second_record( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "value", &field, &index );
  int int_val;
  jedex_value_get_int( &field, &int_val );
  printf( "value %d\n", int_val );

  jedex_value_get_by_name( val, "other", &field, &index );
  jedex_value other_element;
  size_t array_size;
  const char *cstr;
  size_t size;
  jedex_value_get_size( &field, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &other_element, NULL );
    jedex_value rec_field;
    jedex_value_get_by_name( &other_element, "attribute", &rec_field, &index );
    jedex_value_get_string( &rec_field, &cstr, &size );
    printf( "attribute:(%zu): %s\n", i, cstr );
  }
}


static void
get_ref_to_another( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "second_record", &branch, &index );
  get_second_record( &branch );
}


double
rand_number( double from, double to ) {
  double range = to - from;
  return from + ( ( double ) rand() / ( RAND_MAX + 1.0 ) ) * range;
}


static void
set_simple_map_double( jedex_value *val ) {
  char key[ 64 ];

  strcpy( key, "double_1" );

  jedex_value element;
  size_t new_index;
  int is_new = 0;

  jedex_value_add( val, key, &element, &new_index, &is_new );

  double value_double = rand_number( -1e10, 1e10 );
  jedex_value_set_double( &element, value_double );
  
  strcpy( key, "double_2" );
  jedex_value_add( val, key, &element, &new_index, &is_new );
  value_double = rand_number( -1e10, 1e10 );
  jedex_value_set_double( &element, value_double );
}


static void
get_simple_map_double( jedex_value *val ) {
  
  size_t map_size;
  jedex_value_get_size( val, &map_size );
  printf( "map access by index\n" );
  for ( size_t i = 0; i < map_size; i++ ) {
    jedex_value element;
    const char *map_key;

    jedex_value_get_by_index( val, i, &element, &map_key );
    double map_value;
    jedex_value_get_double( &element, &map_value );
    printf( "key: %s value %10.2f\n", map_key, map_value ); 
  }
  printf( "map access by name\n" );
  jedex_value element;
  size_t index;
  const char *map_key = "double_1";
  jedex_value_get_by_name( val, map_key, &element, &index );
  double map_value;
  jedex_value_get_double( &element, &map_value );
  printf( "key: %s value %10.2f\n", map_key, map_value ); 

  map_key = "double_2";
  jedex_value_get_by_name( val, map_key, &element, &index );
  jedex_value_get_double( &element, &map_value );
  printf( "key: %s value %10.2f\n", map_key, map_value ); 
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );
  const char *test_schemas[] =  { 
    "simple_menu_array",
    "db_record",
    "simple_map_double",
    "simple_array",
    "ref_to_another",
    "groceries",
    "menu_array",
    "menu_record",
    "groceries_fruits",
    "groceries_fruits_meat",
    NULL
  };
  
  const char **iter;
  for ( iter = test_schemas; *iter; iter++ ) {
    const char *test_schema = *iter;
    
    if ( !strcmp( test_schema, "simple_menu_array" ) ) {
      jedex_schema *menu_schema = jedex_initialize( "schema/menu_record" );
      assert( menu_schema );

      jedex_schema *array_schema = jedex_schema_array( menu_schema );
      jedex_value_iface *array_class = jedex_generic_class_from_schema( array_schema );

      jedex_value val;
      jedex_generic_value_new( array_class, &val );
      set_menu_array( &val );

      char *json;
      jedex_value_to_json( &val, false, &json );
      printf( "json: %s\n", json );
      jedex_value_reset( &val );
     
      jedex_value *ret_val = json_to_jedex_value( array_schema, json );
      free( json );
      assert( ret_val );

      get_menu_array( ret_val );

      jedex_value_reset( ret_val );
      jedex_finalize( &menu_schema );
    }
    if ( !strcmp( test_schema, "db_record" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/db_record" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_db_record( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );
      
      get_db_record( ret_val );

      jedex_finalize( &schema );
    }
    if ( !strcmp( test_schema, "simple_map_double" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/simple_map_double" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_simple_map_double( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_simple_map_double( ret_val );

      jedex_finalize( &schema );
    }
    if ( !strcmp( test_schema, "simple_array" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/simple_array" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_simple_array( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_simple_array( ret_val );

      jedex_finalize( &schema );
    }
    else if ( !strcmp( test_schema, "ref_to_another" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/ref_to_another" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_ref_to_another( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_ref_to_another( ret_val );

      jedex_finalize( &schema );
    }
    else if ( !strcmp( test_schema, "groceries" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/groceries" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_union_vegetables( val );
      set_union_meat( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_union_vegetables( ret_val );
      get_union_meat( ret_val );

      jedex_finalize( &schema );
    }
    else if ( !strcmp( test_schema, "menu_array" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/menu_array" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_menu_union_value( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_menu_union_value( ret_val );
    }
    else if ( !strcmp( test_schema, "menu_record" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/menu_record" );
      assert( schema );

      const char *sub_schemas[] = { NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "" );
      assert( val );

      set_menu_record_value( val );

      jedex_parcel_destroy( &parcel );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_value *ret_val = json_to_jedex_value( schema, json );
      free( json );

      get_menu_record_value( ret_val );

      jedex_finalize( &schema );
    }
    else if ( !strcmp( test_schema, "groceries_fruits" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/groceries" );
      assert( schema );

      const char *sub_schemas[] = { "fruits", NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, "fruits" );
      assert( val );

      set_fruits( val );

      jedex_parcel_destroy( &parcel );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_schema *sub_schema = jedex_schema_get_subschema( schema, sub_schemas[ 0 ] );
      jedex_value *ret_val = json_to_jedex_value( sub_schema, json );
      assert( ret_val );
      free( json );

      get_fruits( ret_val );

      jedex_finalize( &schema );
    }
    else if ( !strcmp( test_schema, "groceries_fruits_meat" ) ) {
      jedex_schema *schema = jedex_initialize( "schema/groceries" );
      assert( schema );

      const char *sub_schemas[] = { "fruits", "meat", NULL };
      jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
      assert( parcel );

      jedex_value *val = jedex_parcel_value( parcel, sub_schemas[ 0 ] );
      assert( val );

      set_fruits( val );

      char *json;
      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      jedex_schema *sub_schema = jedex_schema_get_subschema( schema, sub_schemas[ 0 ] );
      jedex_value *ret_val = json_to_jedex_value( sub_schema, json );
      assert( ret_val );

      get_fruits( ret_val );

      val = jedex_parcel_value( parcel, sub_schemas[ 1 ] );
      assert( val );

      jedex_parcel_destroy( &parcel );

      set_meat( val );

      jedex_value_to_json( val, false, &json );
      printf( "json: %s\n", json );

      sub_schema = jedex_schema_get_subschema( schema, sub_schemas[ 1 ] );
      ret_val = json_to_jedex_value( sub_schema, json );
      assert( ret_val );
      free( json );

      get_meat( ret_val );

      jedex_finalize( &schema );
    }
  }
   
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
