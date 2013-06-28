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
any_value_to_json( jedex_value *val ) {
  char *json;

  jedex_value_to_json( val, 1, &json );
  if ( json != NULL ) {
    json_error_t json_error;
    json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
    if ( root != NULL ) {
      const char *key;
      json_t *value;

      json_object_foreach( root, key, value ) {
        const char *schema = key;
        //unpack_int( schema, value );
      }
    }
    printf( "root %p\n", ( void * ) root ); 
  }
} 


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
    printf( "int(%u)= %d\n", i, ret_val );
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
get_menu_value( jedex_value *val ) {
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
    printf( "item(%u): %s\n", i, cstr );
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
  jedex_value elements;
  jedex_value_append( &branch, &elements, NULL );
  set_menu_record_value( &elements );
  jedex_value_append( &branch, &elements, NULL );
  set_menu_record_value( &elements );
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
    get_menu_value( &menu_val );
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
  jedex_value_set_float( &field, 2.45 );
}


static void
set_union_vegetables( jedex_value *val ) {
  jedex_value branch;
  size_t branch_count;

  jedex_value_get_size( val, &branch_count );
  assert( branch_count == 3 );

  size_t index;
  jedex_value_get_by_name( val, "vegetables", &branch, &index );
  jedex_value elements;
  jedex_value_append( &branch, &elements, NULL );
  set_vegetables( &branch );
  jedex_value_append( &branch, &elements, NULL );
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
  jedex_value_set_float( &field, 11.45 );
}


static void
get_meat( jedex_value *val ) {
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
  jedex_value elements;
  jedex_value_append( &branch, &elements, NULL ); 
  set_meat( &branch );
  jedex_value_append( &branch, &elements, NULL );
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
}


static void
set_ref_to_another( jedex_value *val ) {
  jedex_value branch;
  size_t index;

  jedex_value_get_by_name( val, "second_record", &branch, &index );
  jedex_value element;
  jedex_value_append( &branch, &element, NULL ); 
  set_second_record( &branch );
}


void
set_union_value( jedex_value *val ) {
  size_t branch_count;
  
  jedex_value_get_size( val, &branch_count );
  size_t i;
  for ( i = 0; i < branch_count; i++ ) {
    jedex_value branch;
    jedex_value_get_branch( val, i, &branch );

    size_t field_count;
    jedex_value_get_size( &branch, &field_count );

    for ( size_t j = 0; j < field_count; j++ ) {
       jedex_value field;
       const char *name;
       jedex_value_get_by_index( &branch, j, &field, &name );
       jedex_type type = jedex_value_get_type( &field );
       if ( type == JEDEX_INT32 ) {
         jedex_value_set_int( &field, 1234 );
       }
       if ( type == JEDEX_ARRAY ) {
         // set two values 
         for ( size_t l = 0; l < 2; l++ ) {
           jedex_value element;
           jedex_value_append( &field, &element, NULL );
           type = jedex_value_get_type( &element );
           if ( type == JEDEX_RECORD ) {
             size_t sub_field_count;
             jedex_value_get_size( &element, &sub_field_count );
             for ( size_t k = 0; k < sub_field_count; k++ ) {
               jedex_value sub_field;
               jedex_value_get_by_index( &element, k, &sub_field, &name );
               jedex_type sub_type = jedex_value_get_type( &sub_field );
               if ( sub_type == JEDEX_STRING ) {
                 char buf[ 64 ];
                 snprintf( buf, sizeof( buf ) - 1, "linked attributes %zu", l );
                 jedex_value_set_string( &sub_field, buf );
               }
             }
           }
         }
       }
     }
  }
}




jedex_value_iface *
jedex_value_iface_from_sub_schema( jedex_schema *schema, const char *sub_schema_name ) {
  jedex_value_iface *val_iface = NULL;

  if ( !strcmp( sub_schema_name, "" ) ) {
    val_iface = jedex_generic_class_from_schema( schema );
  }
  else {
    jedex_schema *sub_schema = jedex_schema_get_subschema( schema, sub_schema_name );
    if ( sub_schema != NULL ) {
      val_iface = jedex_generic_class_from_schema( sub_schema );
    }
  }

  return val_iface == NULL ? NULL: val_iface;
}


int
append_to_parcel( jedex_parcel **parcel, jedex_schema *schema, jedex_value *val ) {
  if ( *parcel == NULL ) {
    *parcel = ( jedex_parcel * ) jedex_new( jedex_parcel );
    if ( *parcel == NULL ) {
      return ENOMEM;
    }
    ( *parcel )->values_list = create_list();
  }
  ( *parcel )->schema = schema;
  if ( val != NULL ) {
    append_to_tail( ( *parcel )->values_list, val );
  }

  return 0;
}


#ifdef TEST
jedex_parcel *
jedex_parcel_create( jedex_schema *schema, const char *sub_schema_names[] ) {
  jedex_value_iface *val_iface = NULL;
  jedex_parcel *parcel = NULL;

  int nr_sub_schema_names = sizeof( sub_schema_names ) / sizeof( sub_schema_names[ 0 ] );
  
  for ( int i = 0; i < nr_sub_schema_names; i++ ) {
    val_iface = jedex_value_iface_from_sub_schema( schema, sub_schema_names[ i ] );
    if ( val_iface != NULL ) {
      jedex_value *val = jedex_value_from_iface( val_iface );
      if ( val != NULL ) {
        if ( append_to_parcel( &parcel, schema, val ) ) {
          return NULL;
        }
      }
    }
  }

  return parcel;
}
#endif


jedex_value *
first_element( const jedex_parcel *parcel ) {
  list_element *e = parcel->values_list->head;

  return e == NULL ? NULL : e->data;
}


jedex_value *
lookup_schema_name( const jedex_parcel *parcel, const char *schema_name ) {
  if ( !strcmp( schema_name, "" ) ) {
    return first_element( parcel );
  }
  for ( list_element *e = parcel->values_list->head; e != NULL; e = e->next ) {
    jedex_value *item = e->data;
    jedex_schema *item_schema = jedex_value_get_schema( item );
    if ( is_jedex_record( item_schema ) ) {
      if ( !strcmp( ( jedex_schema_to_record( item_schema ) )->name, schema_name ) ) { 
        return item;
      }
    }
  }

  return NULL;
}


int
main( int argc, char **argv ) {
  jedex_schema *schema = jedex_initialize( "schema/ref_to_another" );
  assert( schema );

  const char *sub_schemas[] = { NULL };
  jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
  assert( parcel );

  jedex_value *val = jedex_parcel_value( parcel, "" );
  assert( val );

  set_ref_to_another( val );
  //set_simple_array( val );
  //set_union_vegetables( val );
  //set_union_meat( val );
  //set_menu_record_value( val );
  //set_menu_union_value( val );

  char *json;
  jedex_value_to_json( val, 1, &json );

  jedex_value *ret_val = json_to_jedex_value( schema, sub_schemas, json );

  //get_union_vegetables( ret_val );
  //get_union_meat( ret_val );
  //get_menu_union_value( ret_val );
  get_simple_array( ret_val );


  val = jedex_parcel_value( parcel, "meat" );
  assert( val );
  

  set_menu_union_value( val );

  if ( ret_val ) {
    size_t field_count;
    jedex_value_get_size( ret_val, &field_count );    
    assert( field_count == 2 );
    jedex_value field;
    size_t index;
    jedex_value_get_by_name( ret_val, "header", &field, &index );
    assert( index == 0 );
    const char *cstr = NULL;
    size_t size = 0;
    jedex_value_get_string( &field, &cstr, &size );
    printf( "cstr %s\n", cstr );
    jedex_value_get_by_name( ret_val, "items", &field, &index );
    assert( index == 1 );
    jedex_value_get_size( &field, &size );
    assert( size == 2 );
    size_t ssize = 0;
    for ( size_t i = 0; i < size; i++ ) {
      jedex_value array_element;
      jedex_value_get_by_index( &field, i, &array_element, NULL );
      jedex_value_get_string( &array_element, &cstr, &ssize );
      printf( "cstr %s\n", cstr );
    }
  }

//  jedex_value_set_int( val, 10 );
//  any_value_to_json( val );

  set_union_value( val );
  any_value_to_json( val );
  set_menu_union_value( val );
  
#ifdef TO_DELETE
  schema = jedex_initialize( "test_simple_case" );
  assert( schema );

  parcel = jedex_parcel_create( schema, "" );
  assert( parcel );

  val = jedex_parcel_value( parcel, "" );
  assert( val );

  jedex_value_set_int( val, 10 );
  any_value_to_json( val );

  char *jsontext = file_read( "schema", "test_schema" );

  jedex_schema *schema = NULL;

  jedex_schema_from_json( jsontext, &schema );
  free( jsontext );

  jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );
  jedex_value val;

  jedex_generic_value_new( val_iface, &val );
  set_union_value( &val );
  union_to_json( &val );
#endif

  static const char json_record_schema[] = {
    "{"
    "  \"type\": \"record\","
    "  \"name\": \"test\","
    "  \"fields\": ["
    "    { \"name\": \"b\", \"type\": \"boolean\" },"
    "    { \"name\": \"i\", \"type\": \"int\" },"
    "    { \"name\": \"s\", \"type\": \"string\" },"
    "    { \"name\": \"ds\", \"type\": "
    "      { \"type\": \"array\", \"items\": \"double\" } },"
    "    { \"name\": \"sub\", \"type\": "
    "      {"
    "        \"type\": \"record\","
    "        \"name\": \"subtest\","
    "        \"fields\": ["
    "          { \"name\": \"f\", \"type\": \"float\" },"
    "          { \"name\": \"l\", \"type\": \"long\" }"
    "        ]"
    "      }"
    "    },"
    "    { \"name\": \"nested\", \"type\": [\"null\", \"test\"] }"
    "  ]"
    "}"
  };
  static const char json_multi_array_schema[] = {
    "["
       "{\"name\": \"person\","
       " \"type\": \"record\","
       " \"fields\": [ {\"name\": \"height\", \"type\": \"long\"},"
             "{\"name\": \"weight\", \"type\": \"float\"},"
             "{ \"name\": \"shares\", \"type\": "
             "   { \"type\": \"array\", \"items\": \"double\" } },"
             "{\"name\": \"name\", \"type\": \"string\"}]},"

       "{\"name\": \"person2\","
       " \"type\": \"record\","
       " \"fields\": [ {\"name\": \"height2\", \"type\": \"long\"},"
             "{\"name\": \"weight2\", \"type\": \"float\"},"
             "{\"name\": \"name2\", \"type\": \"string\"}]}"
    "]"
   };
   static const char json_plain_array_schema[] = {
     "{"
     "  \"type\": \"array\", \"items\": \"int\""
     "}"
   };
  static const char json_complex_array_schema[] = {
    "["
   "{\"name\": \"person\","
       " \"type\": \"record\","
       " \"fields\": [ {\"name\": \"height\", \"type\": \"long\"},"
             "{\"name\": \"weight\", \"type\": \"float\"},"
             "{ \"name\": \"shares\", \"type\": "
             "   { \"type\": \"array\", \"items\": \"double\" } },"
             "{\"name\": \"name\", \"type\": \"string\"}]},"
    "{"
    "\"type\": \"array\", \"items\": \"person\""
    "}"
  };
    

  jedex_schema_from_json_literal( json_complex_array_schema, &schema );

  if ( schema && schema->type == JEDEX_UNION ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );
    jedex_value val;
    jedex_generic_value_new( val_iface, &val );
    size_t branch_count;
    jedex_value_get_size( &val, &branch_count );
    size_t i;
    for ( i = 0; i < branch_count - 1; i++ ) {
      jedex_value branch;
      jedex_value_get_branch( &val, i, &branch );
      size_t field_count;
      jedex_value_get_size( &branch, &field_count );
      for ( size_t j = 0; j < field_count; j++ ) {
        jedex_value field;
        const char *name;
       jedex_value_get_by_index( &branch, j, &field, &name );
        if ( !strcmp( name, "height" ) ) {
          jedex_value_set_long( &field, 170 );
        }
        if ( !strcmp( name, "weight" ) ) {
          jedex_value_set_float( &field, 61.2 );
        }
        if ( !strcmp( name, "name" ) ) {
          jedex_value_set_string( &field, "foo-bar" );
        }
        if ( !strcmp( name, "shares" ) ) {
          jedex_value element;
          jedex_value_append( &field, &element, NULL );
          jedex_value_set_double( &element, 100.2222 );
          jedex_value_append( &field, &element, NULL );
          jedex_value_set_double( &element, 100.3333 );
        }
      }
    }
    jedex_value branch;
    jedex_value_get_branch( &val, i, &branch );
    size_t item_count;
    jedex_value_get_size( &branch, &item_count );
    jedex_value element;
    jedex_value_append( &branch, &element, NULL );
    printf( "item_count %zu\n", item_count );
    jedex_value field;
    const char *name;
    jedex_value_get_by_index( &element, 0, &field, &name );
    printf( "%s\n", name );
  }
    
  jedex_schema_from_json_literal( json_record_schema, &schema );
  
  if ( schema  && schema->type == JEDEX_RECORD ) {
      jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );

      jedex_value val;
      jedex_generic_value_new( val_iface, &val );

      size_t field_count;
      jedex_value_get_size( &val, &field_count );
      printf( "field count %zu\n", field_count );

      jedex_value field;
      jedex_value_get_by_index( &val, 0, &field, NULL );
      jedex_value_set_boolean( &field, true );

      const char *name;
      jedex_value_get_by_index( &val, 1, &field, &name ),

      jedex_value_set_int( &field, 42 );

      jedex_value_get_by_index( &val, 2, &field, NULL );
      jedex_value_set_string( &field, "Hello World" );

      size_t index;
      jedex_value_get_by_name( &val, "i", &field, &index );

      jedex_value_get_by_index( &val, 3, &field, &name );

      jedex_value element;
      jedex_value_append( &field, &element, NULL );
 
      jedex_value_set_double( &element, 10.2 );
  }

#ifdef LATER
  jedex_schema_from_json_literal( json_multi_array_schema, &schema );
  jedex_schema *iter = *( jedex_schema ** ) schema;
  if ( nr_schemas > 1 ) {
    iter = * ( jedex_schema ** ) schema;
  }
  else {
    iter = schema;
  }
  for ( size_t i = 0; i < nr_schemas; i++, iter = ( ( char * ) iter + sizeof( jedex_schema * ) ) ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( iter );
    jedex_value val;
    jedex_generic_value_new( val_iface, &val );

    size_t field_count;
    jedex_value_get_size( &val, &field_count );
    for ( size_t j = 0; j < field_count; j++ ) {
      jedex_value field;
      const char *name;
      jedex_value_get_by_index( &val, j, &field, &name );
      if ( !strcmp( name, "height" ) ) {
        jedex_value_set_long( &field, 170 );
      }
      if ( !strcmp( name, "weight" ) ) {
        jedex_value_set_float( &field, 61.2 );
      }
      if ( !strcmp( name, "name" ) ) {
        jedex_value_set_string( &field, "foo-bar" );
      }
      if ( !strcmp( name, "shares" ) ) {
        jedex_value element;
        jedex_value_append( &field, &element, NULL );
        jedex_value_set_double( &element, 100.2222 );
        jedex_value_append( &field, &element, NULL );
        jedex_value_set_double( &element, 100.3333 );
      }
    }
  }
  char *json;
  jedex_value_to_json( &val, 1, &json );
  if ( json != NULL ) {
    json_error_t json_error;
    json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
    printf( "root %p\n", ( void * ) root ); 
    jedex_value ret_val;
    jedex_value_from_json_root( root, schema, &ret_val );
  }
#endif

  jedex_schema_from_json_literal( json_plain_array_schema, &schema );
  if ( schema && schema->type == JEDEX_ARRAY ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );
    jedex_value val;
    jedex_generic_value_new( val_iface, &val );
    for ( int j = 222; j < 222 + 10; j++ ) {
      size_t index;
      jedex_value element;
      jedex_value_append( &val, &element, &index );
      jedex_value_set_int( &element, j );
    }
    char *json;
    jedex_value_to_json( &val, 1, &json );
    if ( json != NULL ) {
      json_error_t json_error;
      json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
      printf( "root %p\n", ( void * ) root ); 
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
