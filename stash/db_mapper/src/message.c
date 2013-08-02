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
topic_subscription_callback( jedex_value *val, const char *json ) {
  UNUSED( val );
  UNUSED( json );
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



static jedex_schema *
get_table_schema( const char *tbl_name, jedex_schema *schema ) {
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




static void
create_table_clause( key_info *kinfo, void *user_data ) {
  ref_data *ref = user_data;
  strbuf *command = ref->command;

  const char *pk_name = kinfo->name;
  const char *sql_type_str = kinfo->sql_type_str;
  
  strbuf_addf( command, "%s %s not null, PRIMARY KEY(%s),", 
               pk_name,
               sql_type_str,
               pk_name );
}


static void
create_table_if_not_exists( db_info *db, table_info *tinfo ) {
  const char *db_name = db->name;
  const char *tbl_name = tinfo->name;

  strbuf command = STRBUF_INIT;
  strbuf_addf( &command, "CREATE TABLE IF NOT EXISTS %s.%s (", db_name, tbl_name );

  ref_data ref;
  ref.command = &command;

  foreach_primary_key( tinfo, create_table_clause, &ref ); 

  strbuf_rtrimn( &command, 1 );
  strbuf_addstr( &command, ",json text ) ENGINE=InnoDB DEFAULT CHARSET=utf8" );
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo ) {
    query( db, qinfo, command.buf );
  }
  strbuf_release( &command );
}


static void
get_string_field( jedex_value *val, const char *name, const char **field_name ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, name, &field, &index );
  size_t size = 0;
  jedex_value_get_string( &field, field_name, &size );
}


static void
tinfo_keys_set( jedex_schema *tbl_schema, table_info *tinfo ) {
  size_t rec_size = jedex_schema_record_size( tbl_schema );

  strbuf merge_key = STRBUF_INIT;
  for ( int i = 0; i < ( int ) rec_size; i++ ) {
    const char *fld_name = jedex_schema_record_field_name( tbl_schema, i );
    if ( jedex_schema_record_field_is_primary( tbl_schema, fld_name ) ) {
      jedex_schema *fld_schema = jedex_schema_record_field_get_by_index( tbl_schema, i );

      ALLOC_GROW( tinfo->keys, tinfo->keys_nr + 1, tinfo->keys_alloc );
      key_info *kinfo = ( key_info * ) xmalloc( sizeof( key_info ) );
      
      kinfo->name = fld_name;
      strbuf_addf( &merge_key, "%s:", fld_name );
      kinfo->schema_type = jedex_typeof( fld_schema );
      kinfo->sql_type_str = fld_type_str_to_sql( fld_schema );
      tinfo->keys[ tinfo->keys_nr++ ] = kinfo;
    }
  }
  strbuf_rtrimn( &merge_key, 1 );
  tinfo->merge_key = xstrdup( merge_key.buf );
  strbuf_release( &merge_key );
}


static void
set_table_name( const char *db_name, jedex_value *val, mapper *self ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "tables", &field, &index );


  jedex_value element;
  db_info *db = db_info_get( db_name, self );

  size_t array_size;
  jedex_value_get_size( &field, &array_size );

  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );

    // get the table name from the save_topic message
    const char *tbl_name;
    size_t size;
    jedex_value_get_string( &element, &tbl_name, &size );

    
    // get the table schema
    jedex_schema *tbl_schema = get_table_schema( tbl_name, self->schema );
    ALLOC_GROW( db->tables, db->tables_nr + 1, db->tables_alloc );
    size_t nitems = 1;
    table_info *tinfo = ( table_info * ) xcalloc( nitems, sizeof( table_info ) );
    tinfo->name = tbl_name;
    tinfo_keys_set( tbl_schema, tinfo );
    db->tables[ db->tables_nr++ ] = tinfo;

    create_table_if_not_exists( db, tinfo );
  }
}


void
request_save_topic_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  assert( val );

  mapper *self = user_data;
  const char *db_name = NULL;
  get_string_field( val, "dbname", &db_name );

  const char *topic = NULL;
  get_string_field( val, "topic", &topic );

  set_table_name( db_name, val, self );

  
  const char *schemas[] = { NULL };
  self->emirates->set_subscription( self->emirates, topic, schemas, topic_subscription_callback );

  // TODO send a proper reply back to caller
  self->emirates->send_reply_raw( self->emirates, "save_topic", json );
}


  

static const char *
db_info_dbname( mapper *self, const char *tbl_name ) {
  for ( uint32_t i = 0; i < self->dbs_nr; i++ ) {
    for ( uint32_t j = 0; j < self->dbs[ i ]->tables_nr; j++ ) {
      if ( !strcmp( self->dbs[ i ]->tables[ j ]->name, tbl_name ) ) {
        return self->dbs[ i ]->name;
      }
    }
  }

  return NULL;
}


static void
unpack_find_object( mapper *self, jedex_value *val, const char *tbl_name ) {
  strbuf sb = STRBUF_INIT;

  const char *db_name = db_info_dbname( self, tbl_name );
  strbuf_addf( &sb, "select * from %s.%s where ", db_name, tbl_name );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  foreach_primary_key( tinfo, where_clause, &ref );
  strbuf_rtrimn( &sb, strlen( "and " ) );

  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      strbuf_reset( &sb );
      // store a record row of all fields delimited by ":"
      my_ulonglong num_rows = query_num_rows( qinfo );
      for ( my_ulonglong i = 0; i < num_rows; i++ ) {
        if ( !query_fetch_result( qinfo, &sb ) ) {
          char *json = strbuf_rsplit( &sb, '|' );
          *( json - 1 ) = '\0';
          char *keys = sb.buf;
          printf( "keys %s json %s\n", json, keys );
        }
      }
      query_free_result( qinfo );
        //self->emirates->send_reply_raw( self->emirates, "find_record_reply", json ); 
    }
  }
  strbuf_release( &sb );
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


void
unpack_update_object( mapper *self, jedex_value *val, const char *json, const char *tbl_name ) {
  const char *db_name = db_info_dbname( self, tbl_name );

  strbuf sb = STRBUF_INIT;
  strbuf_addf( &sb, "update %s.%s set json = '%s' where ", db_name, tbl_name, json );
  
  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;
  foreach_primary_key( tinfo, where_clause, &ref );
  strbuf_rtrimn( &sb, strlen( "and " ) );
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      cache_set( self->rcontext, tinfo, json, &ref, &sb );
    }
  }
  strbuf_release( &sb );
}


void
unpack_delete_object( mapper *self, jedex_value *val, const char *tbl_name ) {
  const char *db_name = db_info_dbname( self, tbl_name );

  strbuf sb = STRBUF_INIT;
  strbuf_addf( &sb, "delete from %s.%s where ", db_name, tbl_name );

  db_info *db = db_info_get( db_name, self );
  table_info *tinfo = table_info_get( tbl_name, db );

  ref_data ref;
  ref.command = &sb;
  ref.val = val;
  foreach_primary_key( tinfo, where_clause, &ref );
  strbuf_rtrimn( &sb, strlen( "and " ) );
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, sb.buf ) ) {
      cache_del( self->rcontext, tinfo, &ref, &sb );
    }
  }
  strbuf_release( &sb );
}


const char *
table_name_get( jedex_value *val ) {
  const char *tbl_name = NULL;

  jedex_schema *root_schema = jedex_value_get_schema( val );
  if ( is_jedex_union( root_schema ) ) {
    size_t branch_count;
    jedex_value_get_size( val, &branch_count ); 
    // should only be one find record
    assert( branch_count == 1 );

    jedex_value branch_val;
    size_t idx = 0;
    jedex_value_get_branch( val, idx, &branch_val );
    jedex_schema *bschema = jedex_value_get_schema( &branch_val );
    tbl_name = jedex_schema_type_name( bschema );
  }
  else if ( is_jedex_record( root_schema ) ) {
    tbl_name = jedex_schema_type_name( root_schema );
  }

  return tbl_name;
}




void
request_find_record_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  mapper *self = user_data;

  const char *tbl_name = table_name_get( val );
  if ( tbl_name != NULL ) {
    unpack_find_object( self, val, tbl_name );
  }
}


void
request_insert_record_callback( jedex_value *val, const char *json, void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

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
    // should only be one find record
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
request_update_record_callback( jedex_value *val, const char *json, void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  const char *tbl_name = table_name_get( val );
  if ( tbl_name != NULL ) {
    unpack_update_object( self, val, json, tbl_name );
  }
}


void
request_delete_record_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  mapper *self = user_data;

  const char *tbl_name = table_name_get( val );
  if ( tbl_name != NULL ) {
    unpack_delete_object( self, val, tbl_name );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
