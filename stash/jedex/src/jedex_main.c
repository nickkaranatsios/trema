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

#include "jedex_schema.c"
#include "generic.c"

int
main( int argc, char **argv ) {
#ifdef TEST
  static const char json_schema[] = {
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
#endif
  static const char json_schema[] = {
    "["
       "{\"name\": \"person\","
       " \"type\": \"record\","
       " \"fields\": [ {\"name\": \"height\", \"type\": \"long\"},"
             "{\"name\": \"weight\", \"type\": \"long\"},"
             "{\"name\": \"name\", \"type\": \"string\"}]},"

       "{\"name\": \"person2\","
       " \"type\": \"record\","
       " \"fields\": [ {\"name\": \"height2\", \"type\": \"long\"},"
             "{\"name\": \"weight2\", \"type\": \"long\"},"
             "{\"name\": \"name2\", \"type\": \"string\"}]}"
    "]"
   };

  jedex_schema *schema = NULL;
  jedex_schema_from_json_literal( json_schema, &schema );
  
  if ( schema && schema->type == JEDEX_RECORD ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );

    jedex_value val;
    jedex_generic_value_new( val_iface, &val );

    size_t field_count;
    jedex_value_get_size( &val, &field_count );
    printf( "field count %u\n", field_count );

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

  if ( schema && schema->type == JEDEX_UNION ) {
    jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );

    jedex_value val;
    jedex_generic_value_new( val_iface, &val );

#ifdef UNTESTED
    size_t branch_count;
    jedex_value_get_size( &val, &branch_count );
    jedex_value branch;
    jedex_value_get_branch( &val, 0, &branch );
    
    jedex_schema *record_schema = jedex_value_get_schema( &branch );
    printf( "record_schema type %d\n", record_schema->type );
#endif
    
  }
  
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
