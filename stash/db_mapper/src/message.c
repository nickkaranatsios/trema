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
get_topic_schema( jedex_schema *schema, const char *topic ) {
  if ( is_jedex_union( schema ) ) {
    const char *sub_schemas[] = { topic, NULL };
    return jedex_schema_get_subschema( schema, sub_schemas );
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


static void
create_table_if_not_exists( mapper *self, const char *db_name, const char *table_name ) {
  assert( db_name );
  assert( table_name );

  const char *table_schema = jedex_schema_type_name( schema );
  query( db_handle( self, db_name ), 
         "select count(*) as cnt from information_schema.tables where concat(table_schema,".",table_name)='%s.%s'", 
         db_name,
         table_name );

  char create_table_query[ PATH_MAX ];
  snprintf( create_table_query, sizeof( create_table_query ), "create_table "
  for ( int i = 0; i < ( int ) jedex_schema_record_size( schema ); i++ ) {
    const char *field_name = jedex_schema_record_field_name( schema, i );
    if ( jedex_schema_record_field_is_primary( schema, field_name ) ) {
    }
  }
}


static void
get_string_field( jedex_value *val, const char **field ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "topic", &field, &index );
  size_t size = 0;
  jedex_value_get_string( &field, field, &size );
}


void
request_save_topic_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  assert( val );

  mapper *self = user_data;
  const char *db_name = NULL;
  get_string_field( val, &db_name );
  db_info *db = db_info_ptr( self, db_name );

  const char *topic = NULL;
  get_string_field( val, &topic );


  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "tables", &field, &index );
  size_t array_size;
  jedex_value element;
  jedex_value_get_size( field, &array_size );

  db_tables_nr = array_size;
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );
    const char *table_name;
    get_string_field( &element, &table_name );
    ALLOC_GROW( db->tables, db->tables_nr + 1, db->tables_alloc );
    size_t nitems = 1;
    table_info *tifno = ( table_info * ) xcalloc( nitems, sizeof( table_info ) );
    tinfo->name = table_name;
    db->tables[ db->tables_nr++ ] = tinfo;
    create_table_if_not_exists( self, db_name, table_name );
  }
   


  jedex_schema *schema = get_topic_schema( self->schema, topic );
  create_table_if_not_exists( db_name, table_name, schema );

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
