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


static db_info *
db_info_get( mapper *self, const char *name ) {
  uint32_t i;

  for ( i = 0; i < self->dbs_nr; i++ ) {
    if ( !strcmp( self->dbs[ i ]->name, name ) ) {
      return self->dbs[ i ];
    }
  }

  return NULL;
}


static table_info *
table_info_get( db_info *db, const char *name ) {
  uint32_t i;

  for ( i = 0; i < db->tables_nr; i++ ) {
    if ( !strcmp( db->tables[ i ]->name, name ) ) {
      return db->tables[ i ];
    }
  }

  return NULL;
}


static query_info *
query_info_get( table_info *table ) {
  uint32_t i;

  for ( i = 0; i < MAX_QUERIES_PER_TABLE; i++ ) {
    if ( !table->queries[ i ].used ) {
      return &table->queries[ i ];
    }
  }

  return NULL;
}


#ifdef TEST
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
#endif


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


static const char *
fld_type_str_to_sql( jedex_schema *fld_schema ) {
  if ( is_jedex_string( fld_schema ) ) {
    return "varchar(255)";
  }
  else if ( is_jedex_int32( fld_schema ) || is_jedex_int64( fld_schema ) ) {
    return "int";
  }
  else if ( is_jedex_int64( fld_schema ) ) {
    return "bigint";
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


void
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
foreach_primary_key( table_info *tinfo, primary_key_fn fn, void *user_data ) {
  for ( size_t i = 0; i < tinfo->keys_nr; i++ ) {
    fn( tinfo->keys[ i ], user_data );
  }
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
tinfo_keys_set( table_info *tinfo, jedex_schema *tbl_schema ) {
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
set_table_name( mapper *self, jedex_value *val, const char *db_name ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "tables", &field, &index );


  jedex_value element;
  db_info *db = db_info_get( self, db_name );

  size_t array_size;
  jedex_value_get_size( &field, &array_size );

  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );

    // get the table name from the save_topic message
    const char *tbl_name;
    size_t size;
    jedex_value_get_string( &element, &tbl_name, &size );

    
    // get the table schema
    jedex_schema *tbl_schema = get_table_schema( self->schema, tbl_name );
    ALLOC_GROW( db->tables, db->tables_nr + 1, db->tables_alloc );
    size_t nitems = 1;
    table_info *tinfo = ( table_info * ) xcalloc( nitems, sizeof( table_info ) );
    tinfo->name = tbl_name;
    tinfo_keys_set( tinfo, tbl_schema );
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

  set_table_name( self, val, db_name );

  
  const char *schemas[] = { NULL };
  self->emirates->set_subscription( self->emirates, topic, schemas, topic_subscription_callback );

  // TODO send a proper reply back to caller
  self->emirates->send_reply_raw( self->emirates, "save_topic", json );
}


static void
where_clause( key_info *kinfo, void *user_data ) {
  ref_data *ref = user_data;
  strbuf *command = ref->command;
  jedex_value *val = ref->val;
  
  const char *pk_name = kinfo->name;
  const char *type_str = kinfo->sql_type_str;
  jedex_value field;
  size_t index;
  int rc = jedex_value_get_by_name( val, pk_name, &field, &index );
  if ( !rc ) {
    if ( !prefixcmp( type_str, "varchar" ) ) {
      const char *cstr = NULL;
      size_t size = 0;
      jedex_value_get_string( &field, &cstr, &size );
      strbuf_addf( command, "%s='%s' and ", pk_name, cstr );
    }
    else if ( !strcmp( type_str, "int" ) ) {
      int int_val;
      jedex_value_get_int( &field, &int_val );
      strbuf_addf( command, "%s=" PRId32 "and ", pk_name, int_val );
    }
    else if ( !strcmp( type_str, "bigint" ) ) {
      int64_t long_val;
      jedex_value_get_long( &field, &long_val );
      strbuf_addf( command, "%s=" PRId64 "and ", pk_name, long_val );
    }
    else if ( !strcmp( type_str, "float" ) ) {
      float float_val;
      jedex_value_get_float( &field, &float_val );
      strbuf_addf( command, "%s=%.17f and ", pk_name, float_val );
    }
    else if ( !strcmp( type_str, "real" ) ) {
      double double_val;
      jedex_value_get_double( &field, &double_val );
      strbuf_addf( command, "%s=%.17g and ", pk_name, double_val );
    }
  }
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
unpack_find_object( mapper *self, json_t *root, jedex_value *val ) {
  strbuf command = STRBUF_INIT;

  void *iter = json_object_iter( root );
  const char *tbl_name = json_object_iter_key( iter );
  const char *db_name = db_info_dbname( self, tbl_name );
  strbuf_addf( &command, "select json from %s.%s where ", db_name, tbl_name );

  ref_data ref;
  ref.command = &command;
  ref.val = val;

  db_info *db = db_info_get( self, db_name );
  table_info *tinfo = table_info_get( db, tbl_name );

  foreach_primary_key( tinfo, where_clause, &ref );
  strbuf_rtrimn( &command, strlen( "and " ) );

  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo ) {
    if ( !query( db, qinfo, command.buf ) ) {
    }
  }
#ifdef LATER
  row = mysql_fetch_row();
  strbuf merge_key_values = STRBUF_INIT;
  for ( i = 0; i < tinfo->keys_nr; i++ ) {
    key_info *kinfo = tinfo->keys[ i ];
    if ( !strcmp( kinfo->name, qinfo->res->fields[ i ].name ) ) {
      strbuf_addf( &merge_key_values, "%s:",  row[ i ] );
    }
  }
  // last field should be that data.
  strbuf_rtrimn( &merge_key_values, 1 );
  strbuf_release( &result );
  
  
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo != NULL ) {
    if ( !query( db, qinfo, &command.buf ) ) {
      foreach_result( qinfo, fn, self );
    }
  }
#endif
  strbuf_release( &command );
}


void
request_find_record_callback( jedex_value *val, const char *json, void *user_data ) {
  assert( user_data );
  mapper *self = user_data;

  json_t *root = jedex_decode_json( json );
  if ( root ) {
    if ( json_is_array( root ) ) {
      size_t items;
      items = json_array_size( root );
      if ( items ) {
        json_t *element = json_array_get( root, 0 );
        if ( json_is_object( element ) ) {
          void *iter = json_object_iter( element );
          const char *branch = json_object_iter_key( iter );
          jedex_value branch_val;
          size_t index;
          jedex_value_get_by_name( val, branch, &branch_val, &index );
          unpack_find_object( self, element, &branch_val );
        }
      }
    }
    if ( json_is_object( root ) ) {
      unpack_find_object( self, root, val );
    }
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
