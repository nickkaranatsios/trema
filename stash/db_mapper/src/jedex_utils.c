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


#include "db_mapper.h"
#include "mapper_priv.h"


const char *
table_name_get( const char *jscontent, jedex_value *val ) {
  const char *tbl_name = NULL;

  const char *key = NULL;
  json_t *root = jedex_decode_json( jscontent );
  if ( json_is_object( root ) ) {
    void *iter = json_object_iter( root );
    if ( iter ) {
      key = json_object_iter_key( iter );
    }
  }
  jedex_schema *root_schema = jedex_value_get_schema( val );
  if ( is_jedex_union( root_schema ) ) {
    jedex_value branch_val;
    size_t idx = 0;
    jedex_value_get_by_name( val, key, &branch_val, &idx );

    jedex_schema *bschema = jedex_value_get_schema( &branch_val );
    tbl_name = jedex_schema_type_name( bschema );
  }
  else if ( is_jedex_record( root_schema ) ) {
    tbl_name = jedex_schema_type_name( root_schema );
  }

  return tbl_name;
}


/*
 * It returns a jedex schema object for a given table name.
 * A table name is the record name in a jedex schema terms
 */
jedex_schema *
table_schema_get( const char *tbl_name, jedex_schema *schema ) {
  if ( is_jedex_union( schema ) ) {
    return jedex_schema_get_subschema( schema, tbl_name );
  }
  else if ( is_jedex_record( schema ) ) {
    return schema;
  }
  else {
    log_err( "db_mapper can handle either a union or a record schema" );
    printf( "db_mapper can handle either a union or a record schema\n" );
    return NULL;
  }

  return NULL;
}


/*
 * Assigns the string value of a given field name. It assumes that the jedex
 * value object is a record object and such field exists.
 */
void
get_string_field( jedex_value *val, const char *name, const char **field_name ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, name, &field, &index );
  size_t size = 0;
  jedex_value_get_string( &field, field_name, &size );
}


/*
 * Sets the error field of db_mapper_reply record.
 */
void
db_reply_set( jedex_value *val, db_mapper_error err ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "error", &field, &index );
  jedex_value_set_int( &field, ( int ) err );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
