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
#include "value_json.c"
#include "wrapped_buffer.c"
#include "raw_string.c"
#include "raw_array.c"
#include "raw_map.c"

int
main( int argc, char **argv ) {
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

  jedex_schema *schema = NULL;
  jedex_schema_from_json_literal( json_record_schema, &schema );
  
  if ( schema && schema->type == JEDEX_RECORD ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );

    jedex_value val;
    jedex_generic_value_new( val_iface, &val );

    size_t field_count;
    jedex_value_get_size( &val, &field_count );
    printf( "field count %ul\n", field_count );

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

  jedex_schema_from_json_literal( json_multi_array_schema, &schema );
  if ( schema && schema->type == JEDEX_UNION ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );

    jedex_value val;
    jedex_generic_value_new( val_iface, &val );
    size_t branch_count;
    jedex_value_get_size( &val, &branch_count );

    size_t i;
    for ( i = 0; i < branch_count; i++ ) {
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
    char *json;
    jedex_value_to_json( &val, 1, &json );
    if ( json != NULL ) {
      json_error_t json_error;
      json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
      printf( "root %p\n", ( void * ) root ); 
      jedex_value ret_val;
      jedex_value_from_json_root( root, schema, &ret_val );
    }
  }
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
