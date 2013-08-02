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
    if ( clause == WHERE_CLAUSE ) {
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
    if ( clause == WHERE_CLAUSE ) {
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
    if ( clause == WHERE_CLAUSE ) {
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
    if ( clause == WHERE_CLAUSE ) {
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
    if ( clause == WHERE_CLAUSE ) {
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


void
insert_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, INSERT_CLAUSE );
}


void
where_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, WHERE_CLAUSE );
}


void
redis_clause( key_info *kinfo, void *user_data ) {
  kinfo_clause( kinfo, user_data, REDIS_CLAUSE );
}


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



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
