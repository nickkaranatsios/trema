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
#include "log_writer.h"

#include "priv.h"
#include "allocation.h"
#include "basics.h"
#include "schema_priv.h"
#include "st.h"

#include "allocation.c"
#include "st.c"


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


const char *
jedex_schema_type_name( const jedex_schema *schema ) {
  if ( is_jedex_record( schema ) ) {
    return ( jedex_schema_to_record( schema ) )->name;
  }
  else if ( is_jedex_union( schema ) ) {
    return "union";
  }
  else if ( is_jedex_array( schema ) ) {
    return "array";
  }
  else if ( is_jedex_map( schema ) ) {
    return "map";
  }
  else if ( is_jedex_int32( schema ) ) {
    return "int";
  }
  else if ( is_jedex_int64( schema ) ) {
    return "long";
  }
  else if ( is_jedex_float( schema ) ) {
    return "float";
  }
  else if ( is_jedex_double( schema ) ) {
    return "double";
  }
  else if ( is_jedex_boolean( schema ) ) {
    return "boolean";
  }
  else if ( is_jedex_null( schema ) ) {
    return "null";
  }
  else if ( is_jedex_string( schema ) ) {
    return "string";
  }
  else if ( is_jedex_bytes( schema ) ) {
    return "bytes";
  }
  log_err( "Unknown schema type" );

  return NULL;
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
    log_err( "Can not allocate new record schema" );
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


size_t
jedex_schema_record_size( const jedex_schema *record ) {
  return jedex_schema_to_record( record )->fields->num_entries;
}


jedex_schema *
jedex_schema_record_field_get( const jedex_schema *record, const char *field_name ) {
  union {
    st_data_t data;
    struct jedex_record_field *field;
  } val;
  st_lookup( jedex_schema_to_record( record )->fields_byname, ( st_data_t ) field_name, &val.data );

  return val.field->type;
}


int
jedex_schema_record_field_get_index( const jedex_schema *record, const char *field_name ) {
  union {
    st_data_t data;
    struct jedex_record_field *field;
  } val;
  st_lookup( jedex_schema_to_record( record )->fields_byname, ( st_data_t ) field_name, &val.data );

  return val.field->index;
}
  

const char *
jedex_schema_record_field_name( const jedex_schema *record, int index ) {
  union {
    st_data_t data;
    struct jedex_record_field *field;
  } val;
  st_lookup( jedex_schema_to_record( record )->fields, index, &val.data );

  return val.field->name;
}


jedex_schema *
jedex_schema_record_field_get_by_index( const jedex_schema *record, int index ) {
  union {
    st_data_t data;
    struct jedex_record_field *field;
  } val;
  st_lookup( jedex_schema_to_record( record )->fields, index, &val.data );

  return val.field->type;
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
jedex_type_from_json_t( json_t *json,
                        jedex_type *type,
                        st_table *named_schemas,
                        jedex_schema **named_type ) {
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
    *type = JEDEX_INT32;
  }
  else if ( !strcmp( type_str, "long" ) ) {
    *type = JEDEX_INT64;
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
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_STRING );

  return obj;
}


jedex_schema *
jedex_schema_bytes( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_BYTES );

  return obj;
}


jedex_schema *
jedex_schema_int( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_INT32 );

  return obj;
}


jedex_schema *
jedex_schema_long( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_INT64 );

  return obj;
}


jedex_schema *
jedex_schema_float( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_FLOAT );

  return obj;
}


jedex_schema *
jedex_schema_double( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_DOUBLE );

  return obj;
}


jedex_schema *
jedex_schema_boolean( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_BOOLEAN );

  return obj;
}


jedex_schema *
jedex_schema_null( void ) {
  struct jedex_obj *obj = ( struct jedex_obj * ) jedex_new( struct jedex_obj );
  jedex_schema_init( obj, JEDEX_NULL );

  return obj;
}


jedex_schema *
jedex_schema_link() {
  // TODO named linked types not implemented.
  return NULL;
}


jedex_schema *
jedex_schema_array( jedex_schema *items ) {
  struct jedex_array_schema *array = ( struct jedex_array_schema * ) jedex_new( struct jedex_array_schema );
  if ( !array ) {
    log_err( "Can not allocate new array schema" );
    return NULL;
  }

  array->items = items;
  jedex_schema_init( &array->obj, JEDEX_ARRAY );
  
  return &array->obj;
}


jedex_schema *
jedex_schema_array_items( const jedex_schema *array ) {
  return jedex_schema_to_array( array )->items;
}


jedex_schema *
jedex_schema_map( jedex_schema *values ) {
  struct jedex_map_schema *map = ( struct jedex_map_schema * ) jedex_new( struct jedex_map_schema );
  if ( !map ) {
    log_err( "Can not allocate new map schema" );
    return NULL;
  }
  map->values = values;
  jedex_schema_init( &map->obj, JEDEX_MAP );

  return &map->obj;
}


jedex_schema *
jedex_schema_map_values( const jedex_schema *map ) {
  return jedex_schema_to_map( map )->values;
}


size_t
jedex_schema_union_size( const jedex_schema *union_schema ) {
	check_param( EINVAL, is_jedex_schema( union_schema ), "union schema" );
	check_param( EINVAL, is_jedex_union( union_schema ), "union schema" );
	struct jedex_union_schema *unionp = jedex_schema_to_union( union_schema );

	return unionp->branches->num_entries;
}


jedex_schema *
jedex_schema_union_branch( jedex_schema *unionp, int branch_index ) {
	union {
		st_data_t data;
		jedex_schema *schema;
	} val;

	if ( st_lookup( jedex_schema_to_union( unionp )->branches, branch_index, &val.data ) ) {
		return val.schema;
	}
  else {
		log_err( "No union branch for discriminant %d", branch_index );
		return NULL;
	}
}


jedex_schema *
jedex_schema_union( void ) {
  struct jedex_union_schema *schema = ( struct jedex_union_schema * ) jedex_new( struct jedex_union_schema );
  if ( !schema ) {
    log_err( "Can not allocate new union schema" );
    return NULL;
  }

  schema->branches = st_init_numtable_with_size( DEFAULT_TABLE_SIZE );
  if ( !schema->branches ) {
    log_err( "Can not allocate new union schema (branches)" );
    jedex_free( schema );
    return NULL;
  }

  schema->branches_byname = st_init_strtable_with_size( DEFAULT_TABLE_SIZE );
  if ( !schema->branches_byname ) {
    log_err( "Can not allocate new union schema (branches_byname)" );
    st_free_table( schema->branches );
    jedex_free( schema );
    return NULL;
  }
  jedex_schema_init( &schema->obj, JEDEX_UNION );
  
  return &schema->obj;
}


int
jedex_schema_union_append( const jedex_schema *union_schema, const jedex_schema *schema ) {
  check_param( EINVAL, is_jedex_schema( union_schema ), "union schema" );
  check_param( EINVAL, is_jedex_union( union_schema ), "union schema" );
  check_param( EINVAL, is_jedex_schema( schema ), "schema" );

  struct jedex_union_schema *unionp = jedex_schema_to_union( union_schema );

  int new_index = unionp->branches->num_entries;
  st_insert( unionp->branches, new_index, ( st_data_t ) schema );

  const char *name = jedex_schema_type_name( schema );
  // ?? new_index not schema
  st_insert( unionp->branches_byname, ( st_data_t ) name, ( st_data_t ) new_index ); 

  return 0;
}

static int
save_named_schemas( const char *name, jedex_schema *schema, st_table *st ) {
  return st_insert( st, ( st_data_t ) name, ( st_data_t ) schema );
}


static int
jedex_schema_record_field_append( const jedex_schema *record_schema, 
                                  const char *field_name, 
                                  jedex_schema *field_schema ) {
  check_param( EINVAL, is_jedex_schema( record_schema ), "record schema" );
  check_param( EINVAL, is_jedex_record( record_schema ), "record_schema" );
  check_param( EINVAL, field_name, "field_name" );
  check_param( EINVAL, is_jedex_schema( field_schema ), "field_schema" );

  if ( !is_jedex_id( field_name ) ) {
    log_err( "Invalid jedex identifier" );
    return EINVAL;
  }

  if ( record_schema == field_schema ) {
    log_err( "Can not create a circular schema" );
    return EINVAL;
  }
  
  struct jedex_record_schema *record = jedex_schema_to_record( record_schema );
  struct jedex_record_field *new_field = ( struct jedex_record_field * ) jedex_new( struct jedex_record_field );
  if ( !new_field ) {
    log_err( "Can not allocate new field" );
    return ENOMEM;
  }

  new_field->index = record->fields->num_entries;
  new_field->name = strdup( field_name );
  new_field->type = field_schema;
  st_insert( record->fields, record->fields->num_entries, ( st_data_t ) new_field );
  st_insert( record->fields_byname, ( st_data_t ) new_field->name, ( st_data_t ) new_field );

  return 0;
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
        if ( !json_field_type ) {
          log_err( "Record schema field must have a type" );
          return EINVAL;
        }

        jedex_schema *json_field_type_schema = NULL;
        field_rval = jedex_schema_from_json_t( json_field_type,
                                               &json_field_type_schema,
                                               named_schemas );
        if ( field_rval ) {
          return field_rval;
        }

        field_rval = jedex_schema_record_field_append( *schema,
                                                       json_string_value( json_field_name ),
                                                       json_field_type_schema );
        if ( field_rval != 0 ) {
          return field_rval;
        }
      }
    }
    break;
    case JEDEX_ARRAY: {
      json_t *json_items = json_object_get( json, "items" );
      if ( !json_items ) {
        log_err( "Array schema must have items" );
        return EINVAL;
      }

      jedex_schema *items_schema = NULL;
      int items_rval;

      items_rval = jedex_schema_from_json_t( json_items,
                                               &items_schema,
                                               named_schemas );
      if ( items_rval ) {
        return items_rval;
      }
      *schema = jedex_schema_array( items_schema );
    }
    break;
    case JEDEX_MAP: {
      json_t *json_values = json_object_get( json, "values" );
      if ( !json_values ) {
        log_err( "Must type must have values" );
        return EINVAL;
      }

      jedex_schema *values_schema = NULL;
      int values_rval;

      values_rval = jedex_schema_from_json_t( json_values,
                                              &values_schema,
                                              named_schemas );
      if ( values_rval ) {
        return values_rval;
      }
      *schema = jedex_schema_map( values_schema );
    }
    break;
    case JEDEX_UNION: {
      uint32_t num_schemas = json_array_size( json );
      if ( num_schemas == 0 ) {
        log_err( "Union type must have at least one branch" );
        return EINVAL;
      }
      *schema = jedex_schema_union();
      for ( i = 0; i < num_schemas; i++ ) {
        json_t *schema_json = json_array_get( json, i );
        if ( !schema_json ) {
          log_err( "Can not retrieve branch JSON" );
          return EINVAL;
        }

        int schema_rval;
        jedex_schema *s = NULL;

        schema_rval = jedex_schema_from_json_t( schema_json,
                                                &s,
                                                named_schemas );
        if ( schema_rval != 0 ) {
          return schema_rval;
        }
        schema_rval = jedex_schema_union_append( *schema, s );
        if ( schema_rval != 0 ) {
          return schema_rval;
        }
      }
    }
    break;
    default: {
      log_err( "Unknown schema type" );
      return EINVAL;
    }
  }

  return 0;
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
