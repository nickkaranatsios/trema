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
#include "linked_list.c"

#include "allocation.c"
#include "st.c"
#include "jedex_schema.c"
#include "generic.c"
#include "generic_array.c"
#include "generic_boolean.c"
#include "generic_bytes.c"
#include "generic_double.c"
#include "generic_float.c"
#include "generic_int.c"
#include "generic_long.c"
#include "generic_map.c"
#include "generic_null.c"
#include "generic_record.c"
#include "generic_string.c"
#include "generic_union.c"
#include "generic_link.c"
#include "value_json.c"
#include "wrapped_buffer.c"
#include "memoize.c"
#include "raw_string.c"
#include "raw_array.c"
#include "raw_map.c"


char *
file_read( const char *dirpath, const char *fn ) {
  char filepath[ PATH_MAX ];
  FILE *fp;
  int rval;

  snprintf( filepath, sizeof( filepath ), "%s/%s", dirpath, fn );
  struct stat buf;
  rval = stat( filepath, &buf );
  if ( rval ) {
    return NULL;
  }
  char *jscontent = ( char * ) jedex_malloc( buf.st_size + 1 );
  fp = fopen( filepath, "r" );
  if ( !fp ) {
    return NULL;
  }
  rval = fread( jscontent, 1, buf.st_size, fp );
  fclose( fp );
  jscontent[ rval ] = '\0';

  return jscontent;
}


void
set_menu_record_value( jedex_value *val ) {
  size_t field_count;

  jedex_value_get_size( val, &field_count );

  for ( size_t i = 0; i < field_count; i++ ) {
    jedex_value field;
    const char *field_name;

    jedex_value_get_by_index( val, i, &field, &field_name ); 
    if ( !strcmp( field_name, "header" ) ) {
      jedex_value_set_string( &field, "Save us menu" );
    }
    if ( !strcmp( field_name, "items" ) ) {
      jedex_value element;
      jedex_value_append( &field, &element, NULL );
      jedex_value_set_string( &element, "save the children" );
      jedex_value_append( &field, &element, NULL );
      jedex_value_set_string( &element, "save the world" );
    }
  }
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


void
any_value_to_json( jedex_value *val ) {
  char *json;

  jedex_value_to_json( val, 1, &json );
  if ( json != NULL ) {
    json_error_t json_error;
    json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
    printf( "root %p\n", ( void * ) root ); 
  }
} 


#define DEFAULT_SCHEMA_DIR "schema"
#define DEFAULT_SCHEMA_FN  "test_schema"


jedex_schema *
jedex_initialize( const char *schema_name ) {
  char schema_fn[ FILENAME_MAX ];
  char *jsontext;

  if ( !strcmp( schema_name, "" ) ) {
    strncpy( schema_fn, DEFAULT_SCHEMA_FN, sizeof( schema_fn ) - 1 );
  }
  else {
    strncpy( schema_fn, schema_name, sizeof( schema_fn ) - 1 );
  }
  schema_fn[ sizeof( schema_fn ) - 1 ] = '\0';

  jsontext = file_read( DEFAULT_SCHEMA_DIR, schema_fn );
  jedex_schema *schema = NULL;
  if ( jsontext != NULL ) {
    jedex_schema_from_json( jsontext, &schema );
    free( jsontext );
  }

  return schema == NULL ? NULL : schema;
}


jedex_parcel *
jedex_parcel_create( jedex_schema *schema, const char *sub_schema_name ) {
  jedex_value_iface *val_iface = NULL;

  if ( !strcmp( sub_schema_name, "" ) ) {
    val_iface = jedex_generic_class_from_schema( schema );
  }
  else {
    jedex_schema *sub_schema = jedex_schema_get_subschema( schema, sub_schema_name );
    if ( sub_schema == NULL ) {
      return NULL;
    }
    val_iface = jedex_generic_class_from_schema( sub_schema );
  }
  if ( val_iface == NULL ) {
    return NULL;
  }
  jedex_parcel *jparcel = ( jedex_parcel * ) jedex_new( jedex_parcel );
  if ( jparcel == NULL ) {
    return NULL;
  }
  jparcel->schema = schema;
  jparcel->values_list = create_list();
  jedex_value *val = ( jedex_value * ) jedex_new( jedex_value );
  check_return( NULL, jedex_generic_value_new( val_iface, val ) );

  append_to_tail( jparcel->values_list, val );
  return jparcel;
}


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


jedex_value *
jedex_parcel_value( const jedex_parcel *parcel, const char *schema_name ) {
  assert( parcel );
  assert( parcel->values_list );

  return lookup_schema_name( parcel, schema_name );
}


int
main( int argc, char **argv ) {
  jedex_schema *schema = jedex_initialize( "test_menu_schema" );

  assert( schema );

  jedex_parcel *parcel = jedex_parcel_create( schema, "" );
  assert( parcel );

  jedex_value *val = jedex_parcel_value( parcel, "" );
  assert( val );

  set_menu_record_value( val );

  any_value_to_json( val );


#ifdef TO_DELETE
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
