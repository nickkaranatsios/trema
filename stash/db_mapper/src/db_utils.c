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


/*
 * This function accepts a key information object and a jedex value and 
 * depending on a clause id sets the a primary key to its value.
 * Caveats: Setting the decimal places of a float/double keys might not match 
 * the values set by jansson library or redis cache. At the moment the system
 * does not use float/double keys. 
 */ 
static void
kinfo_to_sql( key_info *kinfo, jedex_value *val, int clause, strbuf *command ) {
  jedex_value field;
  size_t index;
  const char *pk_name = kinfo->name;
  int rc = jedex_value_get_by_name( val, pk_name, &field, &index );
  if ( rc ) {
    return;
  }

  jedex_type key_type = kinfo->schema_type;
  if ( key_type == JEDEX_STRING ) {
    const char *cstr = NULL;
    size_t size = 0;
    jedex_value_get_string( &field, &cstr, &size );
    if ( clause == WHERE_CLAUSE && size > 1 ) {
      strbuf_addf( command, "%s='%s' and ", pk_name, cstr );
    }
    else if ( clause == INSERT_CLAUSE ) {
      strbuf_addf( command, "'%s',", cstr );
    }
    else if ( clause == REDIS_CLAUSE ) {
      strbuf_addf( command, "%s|%s|", pk_name, cstr );
    }
  }
  else if ( key_type == JEDEX_INT32 ) {
    int int_val;
    jedex_value_get_int( &field, &int_val );
    if ( clause == WHERE_CLAUSE && int_val ) {
      strbuf_addf( command, "%s=" PRId32 "and ", pk_name, int_val );
    }
    else if ( clause == INSERT_CLAUSE ) {
      strbuf_addf( command, PRId32",", int_val );
    }
    else if ( clause == REDIS_CLAUSE ) {
      strbuf_addf( command, "%s|"PRId32"|", pk_name, int_val );
    }
  }
  else if ( key_type == JEDEX_INT64 ) {
    int64_t long_val;
    jedex_value_get_long( &field, &long_val );
    if ( clause == WHERE_CLAUSE && long_val ) {
      strbuf_addf( command, "%s=" PRId64 "and ", pk_name, long_val );
    }
    else if ( clause == INSERT_CLAUSE ) {
      strbuf_addf( command, PRId64",", long_val );
    }
    else if ( clause == REDIS_CLAUSE ) {
      strbuf_addf( command, "%s|"PRId64"|", pk_name, long_val );
    }
  }
  else if ( key_type == JEDEX_FLOAT ) {
    float float_val;
    jedex_value_get_float( &field, &float_val );
    if ( clause == WHERE_CLAUSE && ( float_val < 0.0f || float_val > 0.0f ) ) {
      strbuf_addf( command, "%s=%.17f and ", pk_name, float_val );
    }
    else if ( clause == INSERT_CLAUSE ) {
      strbuf_addf( command, "%.17f,", float_val );
    }
    else if ( clause == REDIS_CLAUSE ) {
      strbuf_addf( command, "%s|%.17f|", float_val );
    }
  }
  else if ( key_type == JEDEX_DOUBLE ) {
    double double_val;
    jedex_value_get_double( &field, &double_val );
    if ( clause == WHERE_CLAUSE  && ( double_val < 0.0 || double_val > 0.0 ) ) {
      strbuf_addf( command, "%s=%.17g and ", pk_name, double_val );
    }
    else if ( clause == INSERT_CLAUSE ) {
      strbuf_addf( command, "%.17g,", double_val );
    }
    else if ( clause == REDIS_CLAUSE ) {
      strbuf_addf( command, "%s|%.17g|", double_val );
    }
  }
}


static void
kinfo_clause( key_info *kinfo, void *user_data, int clause ) {
  ref_data *ref = user_data;
  strbuf *command = ref->command;
  jedex_value *val = ref->val;
  
  kinfo_to_sql( kinfo, val, clause, command );
}


static void
create_table_clause( key_info *kinfo, void *user_data ) {
  ref_data *ref = user_data;
  strbuf *command = ref->command;

  const char *pk_name = kinfo->name;
  const char *sql_type_str = kinfo->sql_type_str;
  
  strbuf_addf( command, "%s %s not null,", pk_name, sql_type_str );
}


/*
 * It iterates through the fields of a jedex schema record and creates a key 
 * info information object of each field found to be a primary key.
 */
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
      strbuf_addf( &merge_key, "%s,", fld_name );
      kinfo->schema_type = jedex_typeof( fld_schema );
      kinfo->sql_type_str = fld_type_str_to_sql( fld_schema );
      tinfo->keys[ tinfo->keys_nr++ ] = kinfo;
    }
  }
  strbuf_rtrimn( &merge_key, strlen( "," ) );
  tinfo->merge_key = xstrdup( merge_key.buf );
  strbuf_release( &merge_key );
}



/*
 * Creates a MYSQL database table to a given database.
 */
static void
create_table_if_not_exists( db_info *db, table_info *tinfo ) {
  const char *db_name = db->name;
  const char *tbl_name = tinfo->name;

  strbuf command = STRBUF_INIT;
  strbuf_addf( &command, "CREATE TABLE IF NOT EXISTS %s.%s (", db_name, tbl_name );

  ref_data ref;
  ref.command = &command;

  foreach_primary_key( tinfo, create_table_clause, &ref ); 

  strbuf_addf( &command, "json text, primary key(%s) )ENGINE=InnoDB DEFAULT CHARSET=utf8", tinfo->merge_key );
  query_info *qinfo = query_info_get( tinfo );
  if ( qinfo ) {
    query( db, qinfo, command.buf );
  }
  strbuf_release( &command );
}


/*
 * Converts a primitive jedex field into SQL field. 
 */
const char *
fld_type_str_to_sql( jedex_schema *fld_schema ) {
  if ( is_jedex_string( fld_schema ) ) {
    return "varchar(255)";
  }
  else if ( is_jedex_int32( fld_schema ) ) {
    return "int";
  }
  else if ( is_jedex_int64( fld_schema ) ) {
    return "bigint";
  }
  else if ( is_jedex_float( fld_schema ) ) {
    return "float";
  }
  else if ( is_jedex_double( fld_schema ) ) {
    return "double";
  }
  else {
    return "";
  }
}


/*
 * Extracts primary key information for an insert SQL statement.
 */
void
insert_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, INSERT_CLAUSE );
}


/*
 * Extracts primary key information for a where clause of an SQL statement
 */
void
where_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, WHERE_CLAUSE );
}


/*
 * Extracts primary key information for a redis comamnd either set or delete
 */
void
redis_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, REDIS_CLAUSE );
}


/*
 * Iterates through table's primary keys calling a user supplied function.
 */
void
foreach_primary_key( table_info *tinfo, primary_key_fn fn, void *user_data ) {
  for ( size_t i = 0; i < tinfo->keys_nr; i++ ) {
    fn( tinfo->keys[ i ], user_data );
  }
}


/*
 * Given a database name returns db_mapper's db_info object.
 */
db_info *
db_info_get( const char *name, mapper *self ) {
  uint32_t i;

  for ( i = 0; i < self->dbs_nr; i++ ) {
    if ( !strcmp( self->dbs[ i ]->name, name ) ) {
      return self->dbs[ i ];
    }
  }

  return NULL;
}


/*
 * Returns a table_info object for a given table name
 * Db_info maintains a list of all tables as specified in the save_topic
 * request mesage.
 */
table_info *
table_info_get( const char *name, db_info *db ) {
  uint32_t i;

  for ( i = 0; i < db->tables_nr; i++ ) {
    if ( !strcmp( db->tables[ i ]->name, name ) ) {
      return db->tables[ i ];
    }
  }

  return NULL;
}




/*
 * This is processing of the field "tables" of the save_topic message.
 * "tables" is an array of tables to be created corresponding to the given db.
 * Returns NO_ERROR(0) when no processing errors found.
 */
db_mapper_error
set_table_name( const char *db_name, jedex_value *val, mapper *self ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "tables", &field, &index );


  jedex_value element;
  db_info *db = db_info_get( db_name, self );
  check_ptr_return( db, DB_NAME_NOT_FOUND, "Given db name not found" );

  size_t array_size;
  jedex_value_get_size( &field, &array_size );

  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value_get_by_index( &field, i, &element, NULL );

    // get the table name from the save_topic message
    const char *tbl_name;
    size_t size;
    jedex_value_get_string( &element, &tbl_name, &size );
    // get the table schema
    jedex_schema *tbl_schema = table_schema_get( tbl_name, self->schema );
    // check if invalid schema is not found.
    if ( tbl_schema != NULL ) {
      ALLOC_GROW( db->tables, db->tables_nr + 1, db->tables_alloc );
      size_t nitems = 1;
      table_info *tinfo = ( table_info * ) xcalloc( nitems, sizeof( table_info ) );
      tinfo->name = tbl_name;
      tinfo_keys_set( tbl_schema, tinfo );
      db->tables[ db->tables_nr++ ] = tinfo;

      create_table_if_not_exists( db, tinfo );
    }
    check_ptr_return( tbl_schema, DB_TABLE_SCHEMA_NOT_FOUND, "Table schema not found" );
  }

  return NO_ERROR;
}


/*
 * Returns a database name for a given table name or NULL if not found.
 * Caveats: This means that a table name must be unique across all databases.
 */
const char *
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


void
cache_set( redisContext *rcontext,
           table_info *tinfo,
           const char *json,
           ref_data *ref,
           strbuf *sb ) {
  strbuf_reset( sb );
  strbuf_addf( sb, "%s|", tinfo->name );
  ref->command = sb;
  foreach_primary_key( tinfo, redis_clause, ref );
  size_t slen = 1;
  strbuf_rtrimn( sb, slen );
  size_t klen = sb->len; 
  strbuf_addf( sb, " %s", json );

  redis_cache_set( rcontext, sb->buf, klen, sb->buf + klen + 1, sb->len - klen - 1 );
}


void
cache_del( redisContext *rcontext,
           table_info *tinfo,
           ref_data *ref,
           strbuf *sb ) {
  strbuf_reset( sb );
  strbuf_addf( sb, "%s|", tinfo->name );
  ref->command = sb;
  foreach_primary_key( tinfo, redis_clause, ref );
  size_t slen = 1;
  strbuf_rtrimn( sb, slen );
  size_t klen = sb->len; 

  redis_cache_del( rcontext, sb->buf, klen );
}


/*
 * Returns a query informatiion object for a given table name.
 * We don't keep a fixed number of queries per table. (MAX_QUERIES_PER_TABLE).
 */
query_info *
query_info_get( table_info *table ) {
  uint32_t i;

  for ( i = 0; i < MAX_QUERIES_PER_TABLE; i++ ) {
    if ( !table->queries[ i ].used ) {
      return &table->queries[ i ];
    }
  }

  return NULL;
}


query_info *
query_info_get_by_client_id( table_info *table, const char *client_id ) {
  uint32_t i;

  for ( i = 0; i < MAX_QUERIES_PER_TABLE; i++ ) {
    if ( table->queries[ i ].used && !strncmp( table->queries[ i ].client_id, client_id, strlen( client_id ) ) ) {
      return &table->queries[ i ];
    }
  }

  return NULL;
}


void
query_info_reset( query_info *qinfo ) {
  memset( qinfo, 0, sizeof( qinfo ) );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
