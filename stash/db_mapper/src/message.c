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
topic_subscription_callback( jedex_value *val, const char *json, void *user_data ) {
  if ( json ) {
    printf( "topic subscription callback called %s\n", json );
  }
  request_insert_record_callback( val, json, user_data );
}


static char *
json_to_s( json_t *json ) {
  bool one_line = true;
  return json_dumps( json,
                     JSON_ENCODE_ANY |
                     JSON_INDENT( one_line? 0: 2 ) |
                     JSON_ENSURE_ASCII |
                     JSON_PRESERVE_ORDER );
}


/*
 * Processing of the save_topic request message. It has the following schema:
 * {
 *   "type": "record",
 *   "name": "save_all_data",
 *   "fields": [
 *     { "name": "dbname", "type": "string" },
 *     { "name": "topic", "type": "string" },
 *     { "name": "tables", "type": { "type": "array", "items": "string" } }
 *   ]
 * }
 * The dbname is the database name that is already created using db_mapper's.
 * configuration file upon initialization.
 * For example an entry of database "groceries" found in the config. file is:
 * [db_connection "groceries"]
 *   host = localhost
 *   user = test 
 *   passwd = test
 *   socket = /tmp/mysql.sock
 * The topic is the subscription service name to set to enable db_mapper to
 * receive published data of this service.
 * The tables is an array of tables to create under the database. A table name
 * is mapped to a record name. The preferred way where possible of table names
 * is plural. For example flows not flow.
 */
void
request_save_topic_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( json );
  UNUSED( client_id );
  assert( user_data );
  assert( val );

  mapper *self = user_data;
  const char *db_name = NULL;
  get_string_field( val, "dbname", &db_name );

  db_mapper_error err = set_table_name( db_name, val, self );
  
  const char *topic = NULL;
  get_string_field( val, "topic", &topic );

  db_info *db = db_info_get( db_name, self );
  jedex_schema **schemas = ( jedex_schema ** ) xmalloc ( db->tables_nr * sizeof( jedex_schema * ) * 2 + 1 );
  size_t j;
  for ( size_t i = 0; i < db->tables_nr; i++ ) {
    jedex_schema *tbl_schema = jedex_schema_get_subschema( self->schema, db->tables[ i ]->name );
    jedex_schema *array_schema = jedex_schema_array( tbl_schema );
    j = i;
    schemas[ j++ ] = tbl_schema;
    schemas[ j++ ] = array_schema;
  }
  schemas[ j ] = NULL;

  self->emirates->set_subscription_new( self->emirates, topic, schemas, user_data, topic_subscription_callback );

  db_reply_set( self->reply_val, err );
  self->emirates->send_reply( self->emirates, "save_topic", self->reply_val );
}


static db_mapper_error
unpack_find_record( const char *tbl_name,
                    const char *client_id,
                    const char *json,
                    jedex_value *val,
                    mapper *self ) {
  UNUSED( client_id );
  UNUSED( json );
  strbuf sb = STRBUF_INIT;

  const char *db_name = db_info_dbname( self, tbl_name );
  strbuf_addf( &sb, "select json from %s.%s where ", db_name, tbl_name );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  foreach_primary_key( tinfo, where_clause, &ref );
  if ( !suffixcmp( sb.buf, " and " ) ) {
    strbuf_rtrimn( &sb, strlen( " and " ) );
  }
  else {
    // can not find a record without any primary key setting.
    strbuf_release( &sb );
    return DB_REC_NOT_FOUND;
  }

  db_mapper_error err = NO_ERROR;
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      strbuf_reset( &sb );
      strbuf_addstr( &sb, "[" );
      // store a record row of all fields delimited by ":"
      my_ulonglong num_rows = query_num_rows( qinfo );
      for ( my_ulonglong i = 0; i < num_rows; i++ ) {
        if ( !query_fetch_result( qinfo, &sb ) ) {
          strbuf_addstr( &sb, "," );
        }
      }
      strbuf_rtrimn( &sb, strlen( "," ) );
      strbuf_addstr( &sb, "]" );
      query_free_result( qinfo );
      query_info_reset( qinfo );
      self->emirates->send_reply_raw( self->emirates, "find_record", sb.buf ); 
    }
    else {
      err = DB_REC_NOT_FOUND;
    }
  }
  strbuf_release( &sb );

  return err;
}


static db_mapper_error
unpack_find_all_records( const char *tbl_name,
                         const char *client_id,
                         const char *json,
                         jedex_value *val,
                         mapper *self ) {
  UNUSED( val );
  UNUSED( json );
  strbuf sb = STRBUF_INIT;

  const char *db_name = db_info_dbname( self, tbl_name );
  strbuf_addf( &sb, "select json from %s.%s limit 0, 1", db_name, tbl_name );

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  db_mapper_error err = NO_ERROR;
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    qinfo->row_count = 1;
    memcpy( qinfo->client_id, client_id, sizeof( qinfo->client_id ) );
    if ( !query( db, qinfo, sb.buf ) ) {
      strbuf_reset( &sb );
      strbuf_addstr( &sb, "[" );
      // store a record row of all fields delimited by ":"
      my_ulonglong num_rows = query_num_rows( qinfo );
      for ( my_ulonglong i = 0; i < num_rows; i++ ) {
        if ( !query_fetch_result( qinfo, &sb ) ) {
          strbuf_addstr( &sb, "," );
        }
      }
      strbuf_rtrimn( &sb, strlen( "," ) );
      strbuf_addstr( &sb, "]" );
      query_free_result( qinfo );
      self->emirates->send_reply_raw( self->emirates, "find_all_records", sb.buf ); 
    }
    else {
      err = DB_REC_NOT_FOUND;
    }
  }
  strbuf_release( &sb );

  return err;
}


static db_mapper_error
unpack_find_next_record( const char *tbl_name,
                         const char *client_id,
                         const char *json,
                         jedex_value *val,
                         mapper *self ) {
  UNUSED( json );
  UNUSED( val );
  db_mapper_error err = NO_ERROR;
  strbuf sb = STRBUF_INIT;

  const char *db_name = db_info_dbname( self, tbl_name );
  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );
  query_info *qinfo = query_info_get_by_client_id( tinfo, client_id );
  if ( qinfo != NULL ) {
    strbuf_addf( &sb, "select json from %s.%s limit %u, 1", db_name, tbl_name, qinfo->row_count );
    qinfo->row_count++;

    if ( !query( db, qinfo, sb.buf ) ) {
      strbuf_reset( &sb );
      strbuf_addstr( &sb, "[" );
      // store a record row of all fields delimited by ":"
      my_ulonglong num_rows = query_num_rows( qinfo );
      for ( my_ulonglong i = 0; i < num_rows; i++ ) {
        if ( !query_fetch_result( qinfo, &sb ) ) {
          strbuf_addstr( &sb, "," );
        }
      }
      strbuf_rtrimn( &sb, strlen( "," ) );
      strbuf_addstr( &sb, "]" );
      self->emirates->send_reply_raw( self->emirates, "find_next_record", sb.buf ); 
      query_free_result( qinfo );
    }
    else {
      err = DB_REC_NOT_FOUND;
      query_info_reset( qinfo );
    }
  }
  strbuf_release( &sb );

  return err;
}


/*
 * We expect either an array of records or one record object to insert
 */
void
unpack_insert_object( mapper *self, jedex_value *val, const char *json, const char *tbl_name ) {
  const char *db_name = db_info_dbname( self, tbl_name );

  strbuf sb = STRBUF_INIT;
  strbuf_addf( &sb, "insert into %s.%s values(", db_name, tbl_name );

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;
  foreach_primary_key( tinfo, insert_clause, &ref );
  strbuf_addf( &sb, "'%s')", json );
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      // here we want to save the key/value to redis cache.
      // and we have nothing to return to application.
      cache_set( self->rcontext, tinfo, json, &ref, &sb );
    }
  }
  strbuf_release( &sb );
}


static db_mapper_error
unpack_update_record( const char *tbl_name,
                      const char *client_id,
                      const char *json,
                      jedex_value *val,
                      mapper *self ) {
  UNUSED( client_id );
  const char *db_name = db_info_dbname( self, tbl_name );

  strbuf sb = STRBUF_INIT;
  strbuf_addf( &sb, "update %s.%s set json = '%s' where ", db_name, tbl_name, json );
  
  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;
  foreach_primary_key( tinfo, where_clause, &ref );
  if ( !suffixcmp( sb.buf, " and " ) ) {
    strbuf_rtrimn( &sb, strlen( " and " ) );
  }
  else {
    // can not update a record without any primary key setting.
    strbuf_release( &sb );
    return DB_REC_NOT_FOUND;
  }

  db_mapper_error err = DB_REC_NOT_FOUND;
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      cache_set( self->rcontext, tinfo, json, &ref, &sb );
      err = NO_ERROR;
    }
  }
  strbuf_release( &sb );

  return err;
}


static db_mapper_error
unpack_delete_record( const char *tbl_name,
                      const char *client_id,
                      const char *json,
                      jedex_value *val,
                      mapper *self ) {
  UNUSED( client_id );
  UNUSED( json );
  const char *db_name = db_info_dbname( self, tbl_name );

  strbuf sb = STRBUF_INIT;
  strbuf_addf( &sb, "delete from %s.%s where ", db_name, tbl_name );

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;
  foreach_primary_key( tinfo, where_clause, &ref );
  if ( !suffixcmp( sb.buf, " and " ) ) {
    strbuf_rtrimn( &sb, strlen( " and " ) );
  }
  else {
    // can not update a record without any primary key setting.
    strbuf_release( &sb );
    return DB_REC_NOT_FOUND;
  }
  db_mapper_error err = DB_REC_NOT_FOUND;
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      cache_del( self->rcontext, tinfo, &ref, &sb );
      err = NO_ERROR;
    }
  }
  strbuf_release( &sb );

  return err;
}


static db_mapper_error
unpack_record( jedex_value *val,
               const char *json,
               const char *client_id,
               mapper *self,
               unpack_record_fn fn ) {
  const char *tbl_name = table_name_get( json, val );
  db_mapper_error err = DB_TABLE_NOT_FOUND;
  if ( tbl_name != NULL ) {
    jedex_schema *root_schema = jedex_value_get_schema( val );
    if ( is_jedex_union( root_schema ) ) {
      jedex_value branch_val;
      size_t idx = 0;
      jedex_value_get_by_name( val, tbl_name, &branch_val, &idx );
      err = fn( tbl_name, client_id, json, &branch_val, self );
    }
    else {
      err = fn( tbl_name, client_id, json, val, self );
    }
  }

  return err;
}


/*
 * Processing of the find record message. Expected a record with the primary
 * key information filled in. Multiple key records concatenated together in 
 * the where clause of the SQL statement. If have multiple keys and a key is
 * not set it is ignored. By not set we mean that it has a value that if it is
 * a string is an empty string or if it is numeric it has the value of zero.
 */
void
request_find_record_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( client_id );
  assert( user_data );
  mapper *self = user_data;

  db_mapper_error err = unpack_record( val, json, client_id, self, unpack_find_record );
  if ( err != NO_ERROR ) {
    self->emirates->send_reply_raw( self->emirates, "find_record", json );
  }
}


void
request_find_all_records_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  db_mapper_error err = unpack_record( val, json, client_id, self, unpack_find_all_records );
  if ( err != NO_ERROR ) {
    self->emirates->send_reply_raw( self->emirates, "find_all_records", json );
  }
}


void
request_find_next_record_callback( jedex_value *val,
                                   const char *json,
                                   const char *client_id,
                                   void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  db_mapper_error err = unpack_record( val, json, client_id, self, unpack_find_next_record );
  if ( err != NO_ERROR ) {
    self->emirates->send_reply_raw( self->emirates, "find_next_record", json );
  }
}


void
request_insert_record_callback( jedex_value *val, const char *json, void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

printf( "insert record %s\n", json );
  jedex_schema *root_schema = jedex_value_get_schema( val );
  if ( is_jedex_record( root_schema ) ) {
    const char *tbl_name = jedex_schema_type_name( root_schema );
    unpack_insert_object( self, val, json, tbl_name );
  }
  else if ( is_jedex_array( root_schema ) ) {
    size_t array_size;
    jedex_value element;

    jedex_value_get_size( val, &array_size );
    json_t *jarray = jedex_decode_json( json );
    for ( size_t i = 0; i < array_size; i++ ) {
      jedex_value_get_by_index( val, i, &element, NULL );
      jedex_schema *rschema = jedex_value_get_schema( &element );
      const char *tbl_name = jedex_schema_type_name( rschema );

      json_t *jelement = json_array_get( jarray, i );
      char *json_str = json_to_s( jelement );
      unpack_insert_object( self, &element, json_str, tbl_name );
      free( json_str );
      json_decref( jelement );
    }
  }
  else if ( is_jedex_union( root_schema ) ) {
    size_t branch_size;
    jedex_value_get_size( val, &branch_size ); 
    json_t *jarray = jedex_decode_json( json );
    for ( size_t i = 0; i < branch_size; i++ ) {
      jedex_value branch_val;
      size_t idx = 0;
      jedex_value_get_branch( val, idx, &branch_val );

      jedex_schema *bschema = jedex_value_get_schema( &branch_val );
      const char *tbl_name = jedex_schema_type_name( bschema );
      // now unpack the find
      json_t *jelement = json_array_get( jarray, i );
      char *json_str = json_to_s( jelement );
      unpack_insert_object( self, val, json, tbl_name );
      free( json_str );
      json_decref( jelement );
    }
  }
}


void
request_update_record_callback( jedex_value *val,
                                const char *json,
                                const char *client_id,
                                void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  db_mapper_error err = unpack_record( val, json, client_id, self, unpack_update_record );
  db_reply_set( self->reply_val, err );
  self->emirates->send_reply( self->emirates, "update_record", self->reply_val );
}


void
request_delete_record_callback( jedex_value *val,
                                const char *json,
                                const char *client_id,
                                void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  db_mapper_error err = unpack_record( val, json, client_id, self, unpack_delete_record );
  db_reply_set( self->reply_val, err );
  self->emirates->send_reply( self->emirates, "delete_record", self->reply_val );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
