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


static void
topic_subscription_callback( void *data ) {
  assert( data );
}


static db_info *
db_info_ptr( mapper *self, const char *name ) {
  uint32_t i;

  for ( i = 0; i < self->dbs_nr; i++ ) {
    if ( !strcmp( self->dbs[ i ]->name, name ) ) {
      return self->dbs[ i ];
    }
  }

  return NULL;
}


static jedex_value *
get_jedex_value( jedex_schema *schema, const char *topic ) {
  jedex_value *val = NULL;

  if ( is_jedex_union( schema ) ) {
    const char *sub_schemas[] = { NULL };
    jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
    assert( parcel );

    val = jedex_parcel_value( parcel, "" );
    assert( val );
  }
  else if ( is_jedex_record( schema ) ) {
    const char *sub_schemas[] = { topic, NULL };
    jedex_parcel *parcel = jedex_parcel_create( schema, sub_schemas );
    assert( parcel );

    val = jedex_parcel_value( parcel, topic );
    assert( val );
  }
  else {
    log_err( "db_mapper can handle either a union or record schema" );
    printf( "db_mapper can handle either a union or record schema\n" );
    return val;
  }

  return val;
}


static jedex_schema *
get_table_schema( jedex_schema *schema, const char *tbl_name ) {
  if ( is_jedex_union( schema ) ) {
    return jedex_schema_get_subschema( schema, tbl_name );
  }
  else if ( is_jedex_record( schema ) ) {
    return schema;
  }
  else {
    log_err( "db_mapper can handle either a union or record schema" );
    printf( "db_mapper can handle either a union or record schema\n" );
    return NULL;
  }

  return NULL;
}


static char *type_str( jedex_schema *fld_schema ) {
  if ( is_jedex_string( fld_schema ) ) {
    return "char";
  }
  else if ( is_jedex_int32( fld_schema ) || is_jedex_int64( fld_schema ) ) {
    return "int";
  }
  else if ( is_jedex_float( fld_schema ) ) {
    return "float";
  }
  else if ( is_jedex_double( fld_schema ) ) {
    return "real";
  }
  else {
    return "";
  }
}


static void
create_table_if_not_exists( mapper *self, const char *db_name, const char *tbl_name ) {
  assert( db_name );
  assert( tbl_name );


  jedex_schema *tbl_schema = get_table_schema( self->schema, tbl_name );
  
  strbuf command = STRBUF_INIT;
  strbuf_addf( &command, "create table %s.%s (", db_name, tbl_name );
  for ( int i = 0; i < ( int ) jedex_schema_record_size( tbl_schema ); i++ ) {
    const char *field_name = jedex_schema_record_field_name( tbl_schema, i );
    if ( jedex_schema_record_field_is_primary( tbl_schema, field_name ) ) {
      strbuf_addf( &command, "%s %s not null,", 
                   field_name,
                   type_str( jedex_schema_record_field_get_by_index( tbl_schema, i ) ) );
    }
  }
}


static void
get_string_field( jedex_value *val, const char **field_name ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "topic", &field, &index );
  size_t size = 0;
  jedex_value_get_string( &field, field_name, &size );
}


static void
set_table_name( mapper *self, jedex_value *val, const char *db_name ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "tables", &field, &index );

  size_t array_size;
  jedex_value_get_size( &field, &array_size );

  jedex_value element;
  db_info *db = db_info_ptr( self, db_name );

  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );

    const char *tbl_name;
    get_string_field( &element, &tbl_name );

    ALLOC_GROW( db->tables, db->tables_nr + 1, db->tables_alloc );
    size_t nitems = 1;
    table_info *tinfo = ( table_info * ) xcalloc( nitems, sizeof( table_info ) );
    tinfo->name = tbl_name;
    db->tables[ db->tables_nr++ ] = tinfo;

    create_table_if_not_exists( self, db_name, tbl_name );
  }
}


void
request_save_topic_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  assert( val );

  mapper *self = user_data;
  const char *db_name = NULL;
  get_string_field( val, &db_name );

  const char *topic = NULL;
  get_string_field( val, &topic );

  set_table_name( self, val, db_name );

  
  const char *schemas[] = { NULL };
  self->emirates->set_subscription( self->emirates, topic, schemas, topic_subscription_callback );

  // TODO send a proper reply back to caller
  self->emirates->send_reply_raw( self->emirates, "save_topic", json );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
