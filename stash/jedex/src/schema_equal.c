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


static int
schema_record_equal( struct jedex_record_schema *a, struct jedex_record_schema *b ) {
  long i;

  if ( strcmp( a->name, b->name ) ) {
    /*
     * They have different names
     */
    return 0;
  }
  if ( a->space && b->space ) {
    /* They have different namespaces */
    if ( strcmp( a->space, b->space ) ) {
      return 0;
    }
  }
  else if ( a->space || b->space ) {
    /* One has a namespace, one doesn't */
    return 0;
  }
  for ( i = 0; i < a->fields->num_entries; i++ ) {
    union {
      st_data_t data;
      struct jedex_record_field *f;
    } fa, fb;
    st_lookup( a->fields, ( st_data_t ) i, &fa.data );
    if ( !st_lookup( b->fields, ( st_data_t ) i, &fb.data ) ) {
      return 0;
    }
    if ( strcmp( fa.f->name, fb.f->name ) ) {
      /*
       * They have fields with different names
       */
      return 0;
    }
    if ( !jedex_schema_equal( fa.f->type, fb.f->type ) ) {
      /*
       * They have fields with different schemas
       */
      return 0;
    }
  }

  return 1;
}


static int
schema_map_equal( struct jedex_map_schema *a, struct jedex_map_schema *b ) {
  return jedex_schema_equal( a->values, b->values );
}


static int
schema_array_equal( struct jedex_array_schema *a, struct jedex_array_schema *b ) {
  return jedex_schema_equal( a->items, b->items );
}


static int
schema_union_equal( struct jedex_union_schema *a, struct jedex_union_schema *b ) {
  long i;

  for ( i = 0; i < a->branches->num_entries; i++ ) {
    union {
      st_data_t data;
      jedex_schema *schema;
    } ab, bb;
    st_lookup( a->branches, ( st_data_t ) i, &ab.data );
    if ( !st_lookup( b->branches, ( st_data_t ) i, &bb.data ) ) {
      return 0;
    }
    if ( !jedex_schema_equal( ab.schema, bb.schema ) ) {
      /*
       * They don't have the same schema types
       */
      return 0;
    }
  }

  return 1;
}


static int
schema_link_equal( struct jedex_link_schema *a, struct jedex_link_schema *b ) {
  /*
   * NOTE: links can only be used for named types. They are used in
   * recursive schemas so we just check the name of the schema pointed
   * to instead of a deep check.  Otherwise, we recurse forever...
   */
  return ( strcmp( jedex_schema_name( a->to ), jedex_schema_name( b->to ) ) == 0 );
}


int
jedex_schema_equal( jedex_schema *a, jedex_schema *b ) {
  if ( a == NULL || b == NULL ) {
    return 0;
  }
  else if ( a == b ) {
    return 1;
  }
  else if ( jedex_typeof( a ) != jedex_typeof( b ) ) {
    return 0;
  }
  else if ( is_jedex_record( a ) ) {
    return schema_record_equal( jedex_schema_to_record( a ), jedex_schema_to_record( b ) );
  }
  else if ( is_jedex_map( a ) ) {
    return schema_map_equal( jedex_schema_to_map( a ), jedex_schema_to_map( b ) );
  }
  else if ( is_jedex_array( a ) ) {
    return schema_array_equal( jedex_schema_to_array( a ), jedex_schema_to_array( b ) );
  }
  else if ( is_jedex_union( a ) ) {
    return schema_union_equal( jedex_schema_to_union( a ), jedex_schema_to_union( b ) );
  }
  else if ( is_jedex_link( a ) ) {
    return schema_link_equal( jedex_schema_to_link( a ), jedex_schema_to_link( b ) );
  }

  return 1;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
