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


#include <ctype.h>
#include <string.h>
#include "jansson.h"
#include "../log_writer.h"

#include "allocation.h"
#include "schema_priv.h"
#include "st.h"


#define DEFAULT_TABLE_SIZE 32

static void
jedex_schema_init( jedex_schema *schema, jedex_type type ) {
  schema->type = type;
  schema->class_type = JEDEX_SCHEMA;
}
  

static int
is_jedex_id( const char *name ) {
  size_t i;
  size_t len;

  if ( name ) {
    len = strlen( name );
    if ( len < 1 ) {
      return 0;
    }
    for ( i = 0; i < len; i++ ) {
      if ( !( isalpha( name[ i ] )
              || name[ i ] == '_' || ( i && isdigit( name[ i ] ) ) ) ) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


static void
jedex_schema_record_free( struct jedex_record_schema *record ) {
  if ( record->name ) {
    jedex_free( record->name );
  }
  if ( record->space ) {
    jedex_free( record->space );
  }
  if ( record->fields ) {
     st_free_table( record->fields );
  }
  if ( record->fields_byname ) {
    st_free_table( record->fields_byname );
  }
  jedex_free( record );
}


jedex_schema *
jedex_schema_record( const char *name, const char *space ) {
  if ( !is_jedex_id( name ) ) {
    log_err( "Invalid record schema name %s", name );
    return NULL;
  }
  struct jedex_record_schema *record = ( struct jedex_record_schema * ) jedex_new( struct jedex_record_schema );
  if ( record == NULL ) {
    log_err( "Can not allocate memory for new record schema" );
    return NULL;
  }
  record->name = strdup( name );
  record->space = space ? strdup( space ) : NULL;
  record->fields = st_init_numtable_with_size( DEFAULT_TABLE_SIZE );
  if ( !record->fields ) {
    log_err( "Can not allocate memory for new record schema" );
    jedex_schema_record_free( record );
    return NULL;
  }
  record->fields_byname = st_init_strtable_with_size( DEFAULT_TABLE_SIZE );
  if ( !record->fields_byname ) {
    log_err( "Can not allocate memory for new record schema" );
    jedex_schema_record_free( record );
    return NULL;
  }
  jedex_schema_init( &record->obj, JEDEX_RECORD );

  return &record->obj;
}


static jedex_schema *
find_named_schemas( const char *name, st_table *st ) {
  union {
    jedex_schema *schema;
    st_data_t data;
  } val;

  if ( st_lookup( st, ( st_data_t ) name, &( val.data ) ) ) {
    return val.schema;
  }
  log_err( "No schema type named %s\n", name );
  return NULL;
}


static int
jedex_type_from_json_t( json_t *json, jedex_type *type,
  st_table *named_schemas, jedex_schema **named_type ) {
  json_t *json_type;

  if ( json_is_array( json ) ) {
    *type = JEDEX_UNION;
    return 0;
  }
  else if ( json_is_object( json ) ) {
    json_type = json_object_get( json, "type" );
  }
  else {
    json_type = json;
  }
  if ( !json_is_string( json_type ) ) {
    log_err( "\"type\" field must be a string" );
    return EINVAL;
  }

  const char *type_str;

  type_str = json_string_value( json_type );
  if ( type_str == NULL ) {
    log_err( "\"type\" field must be a string" );
    return EINVAL;
  }

  if ( !strcmp( type_str, "string" ) ) {
    *type = JEDEX_STRING;
  }
  else if ( !strcmp( type_str, "bytes" ) ) {
    *type = JEDEX_BYTES;
  }
  else if ( !strcmp( type_str, "int" ) ) {
    *type = JEDEX_INT;
  }
  else if ( !strcmp( type_str, "long" ) ) {
    *type = JEDEX_LONG;
  }
  else if ( !strcmp( type_str, "float" ) ) {
    *type = JEDEX_FLOAT;
  }
  else if ( !strcmp( type_str, "double" ) ) {
    *type = JEDEX_DOUBLE;
  }
  else if ( !strcmp( type_str, "boolean" ) ) {
    *type = JEDEX_BOOLEAN;
  }
  else if ( !strcmp( type_str, "null" ) ) {
    *type = JEDEX_NULL;
  }
  else if ( !strcmp( type_str, "record" ) ) {
    *type = JEDEX_RECORD;
  }
  else if ( !strcmp( type_str, "array" ) ) {
    *type = JEDEX_ARRAY;
  }
  else if ( !strcmp( type_str, "map" ) ) {
    *type = JEDEX_MAP;
  }
  else if ( ( *named_type = find_named_schemas( type_str, named_schemas ) ) ) {
    *type = JEDEX_LINK;
  }
  else {
    log_err( "Unknown jedex schema \"type\": %s", type_str ); 
    return EINVAL;
  }

  return 0;
}
  

jedex_schema *
jedex_schema_string( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_STRING );

  return obj;
}


jedex_schema *
jedex_schema_bytes( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_BYTES );

  return obj;
}


jedex_schema *
jedex_schema_int( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_INT32 );

  return obj;
}


jedex_schema *
jedex_schema_long( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_INT64 );

  return obj;
}


jedex_schema *
jedex_schema_float( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_FLOAT );

  return obj;
}


jedex_schema *
jedex_schema_double( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_DOUBLE );

  return obj;
}


jedex_schema *
jedex_schema_boolean( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_BOOLEAN );

  return obj;
}


jedex_schema *
jedex_schema_null( void ) {
  jedex_obj *obj = ( jedex_obj * ) jedex_new( jedex_obj );
  jedex_schema_init( obj, JEDEX_NULL );

  return obj;
}


jedex_schema *
jedex_schema_link( jedex_schema *to ) {
  // TODO named linked types not implemented.
  return NULL;
}


static int
jedex_schema_from_json_t( json_t *json, jedex_schema **schema, st_table *named_schemas ) {
  jedex_type type = JEDEX_INVALID;
  uint32_t i;
  jedex_schema *named_type = NULL;

  if ( jedex_type_from_json_t( json, &type, named_schemas, &named_type ) ) {
    return EINVAL;
  }
  switch ( type ) {
    case JEDEX_LINK:
      *schema = jedex_schema_link();
    break;
    case JEDEX_STRING:
      *schema = jedex_schema_string();
    break;
    case JEDEX_BYTES:
      *schema = jedex_schema_bytes();
    break;
    case JEDEX_INT32:
      *schema = jedex_schema_int();
    break;
    case JEDEX_INT64:
      *schema = jedex_schema_long();
    break;
    case JEDEX_FLOAT:
      *schema = jedex_schema_float();
    break;
    case JEDEX_DOUBLE:
      *schema = jedex_schema_double();
    break;
    case JEDEX_BOOLEAN:
      *schema = jedex_schema_boolean();
    break;
    case JEDEX_NULL:
      *schema = jedex_schema_null();
    break;
    case JEDEX_RECORD: {
      json_t *json_name = json_object_get( json, "name" );
      json_t *json_namespace = json_object_get( json, "namespace" );
      json_t *json_fields = json_object_get( json, "fields" );
      uint32_t num_fields;
      const char *record_name;
      const char *record_namespace;
    

      if ( !json_is_string( json_name ) ) {
        log_err( "Record schema type must have a name" );
        return EINVAL;
      }
      if ( !json_is_array( json_fields ) ) {
        log_err( "Record schema type must have fields" );
        return EINVAL;
      }
      num_fields = json_array_size( json_fields );
      if ( !num_fields ) {
        log_err( "Record schema type must have at least one field" );
        return EINVAL;
      }
      record_name = json_string_value( json_name );
      if ( !record_name ) {
        log_err( "Record schema type must have a name" );
        return EINVAL;
      }
      if ( json_is_string( json_namespace ) ) {
        record_namespace = json_string_value( json_namespace );
      }
      else {
        record_namespace = NULL;
      }
      *schema = jedex_schema_record( record_name, record_namespace );
      if ( save_named_schemas( record_name, *schema, named_schemas ) ) {
        log_err( "Can not save record schema" );
        return ENOMEM;
      }
      for ( i = 0; i < num_fields; i++ ) {
        json_t *json_field = json_array_get( json_fields, i );
        json_t *json_field_name;
        json_t *json_field_type;
        jedex_schema json_field_type_name;
        int field_rval;
        
        if ( !json_is_object( json_field ) ) {
          log_err( "Record schema field must be an array" );
          return EINVAL;
        }
        json_field_name = json_object_get( json_field, "name" );
        if ( !json_field_name ) {
          log_err( "Record field %d must have a name", i );
          return EINVAL;
        }
        json_field_type = json_object_get( json_field, "type" );

      }
}


static int
jedex_schema_from_json_root( json_t *root, jedex_schema **schema ) {
  int rval;
  st_table *named_schemas;

  named_schemas = st_init_strtable_with_size( DEFAULT_TABLE_SIZE );
  if ( named_schemas == NULL ) {
    log_err( "Failed to allocated named schema map" );
    json_decref( root );
    return ENOMEM;
  }

  rval = jedex_schema_from_json_t( root, schema, named_schemas );
  json_decref( root );
  st_free_table( named_schemas );
  
  return rval;
}


int
jedex_schema_from_json_length( const char *jsontext, size_t length, jedex_schema **schema ) {
  json_t *root;
  json_error_t json_error;
  
  root = json_loadb( jsontext, length, 0, &json_error );
  if ( root == NULL ) {
    log_err( "Error parsing JSON %s", jsontext );
    return EINVAL;
  }

  return jedex_schema_from_json_root( root, schema );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
