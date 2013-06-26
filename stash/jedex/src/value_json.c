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


#include "jedex.h"


#define return_json( type, exp )            \
  {                \
    json_t  *result = exp;          \
    if ( result == NULL ) {          \
      log_err( "Cannot allocate JSON " type );  \
    }              \
    return result;            \
  }

#define set_fail( reference, type ) \
  do { \
    log_err( "Cannot set " type ); \
    json_decref( ( reference ) ); \
    *failed = EINVAL; \
    return NULL; \
  } while ( 0 )
  


static int
encode_utf8_bytes( const void *src, size_t src_len, void **dest, size_t *dest_len ) {
  check_param( EINVAL, src, "source" );
  check_param( EINVAL, dest, "dest" );
  check_param( EINVAL, dest_len, "dest_len" );

  // First, determine the size of the resulting UTF-8 buffer.
  // Bytes in the range 0x00..0x7f will take up one byte; bytes in
  // the range 0x80..0xff will take up two.
  const uint8_t *src8 = ( const uint8_t * ) src;

  size_t utf8_len = src_len + 1;  // +1 for NUL terminator
  size_t i;
  for ( i = 0; i < src_len; i++ ) {
    if ( src8[ i ] & 0x80 ) {
      utf8_len++;
    }
  }

  // Allocate a new buffer for the UTF-8 string and fill it in.
  uint8_t *dest8 = ( uint8_t * ) jedex_malloc( utf8_len );
  if ( dest8 == NULL ) {
    log_err( "Cannot allocate JSON bytes buffer" );
    return ENOMEM;
  }

  uint8_t *curr = dest8;
  for ( i = 0; i < src_len; i++ ) {
    if ( src8[ i ] & 0x80 ) {
      *curr++ = ( 0xc0 | ( src8[ i ] >> 6 ) );
      *curr++ = ( 0x80 | ( src8[ i ] & 0x3f ) );
    }
    else {
      *curr++ = src8[i];
    }
  }

  *curr = '\0';

  // And we're good.
  *dest = dest8;
  *dest_len = utf8_len;

  return 0;
}


static json_t *
jedex_value_to_json_t( const jedex_value *value ) {
  switch ( jedex_value_get_type( value ) ) {
    case JEDEX_BOOLEAN: {
      int val;
      check_return( NULL, jedex_value_get_boolean( value, &val ) );
      return_json( "boolean", val?  json_true(): json_false() );
    }

    case JEDEX_BYTES: {
      const void *val;
      size_t size;
      void *encoded = NULL;
      size_t encoded_size = 0;

      check_return( NULL, jedex_value_get_bytes( value, &val, &size ) );

      if ( encode_utf8_bytes( val, size, &encoded, &encoded_size ) ) {
        return NULL;
      }

      json_t *result = json_string_nocheck( ( const char * ) encoded );
      jedex_free( encoded );
      if ( result == NULL ) {
        log_err( "Cannot allocate JSON bytes" );
      }
      return result;
    }

    case JEDEX_DOUBLE: {
      double val;
      check_return( NULL, jedex_value_get_double( value, &val ) );
      return_json( "double", json_real( val ) );
    }

    case JEDEX_FLOAT: {
      float val;
      check_return( NULL, jedex_value_get_float( value, &val ) );
      return_json( "float", json_real( val ) );
    }

    case JEDEX_INT32: {
      int32_t val;
      check_return( NULL, jedex_value_get_int( value, &val ) );
      return_json( "int", json_integer( val ) );
    }

    case JEDEX_INT64: {
      int64_t val;
      check_return( NULL, jedex_value_get_long( value, &val ) );
      return_json( "long", json_integer( val ) );
    }

    case JEDEX_NULL: {
      check_return( NULL, jedex_value_get_null( value ) );
      return_json( "null", json_null() );
    }

    case JEDEX_STRING: {
      const char *val;
      size_t size;
      check_return( NULL, jedex_value_get_string( value, &val, &size ) );
      return_json( "string", json_string( val ) );
    }

    case JEDEX_ARRAY: {
      int rc;
      size_t element_count, i;
      json_t *result = json_array();
      if ( result == NULL ) {
        log_err( "Cannot allocate JSON array" );
        return NULL;
      }

      rc = jedex_value_get_size( value, &element_count );
      if ( rc != 0 ) {
        json_decref( result );
        return NULL;
      }

      for (i = 0; i < element_count; i++) {
        jedex_value element;
        rc = jedex_value_get_by_index( value, i, &element, NULL );
        if ( rc != 0 ) {
          json_decref( result );
          return NULL;
        }

        json_t *element_json = jedex_value_to_json_t( &element );
        if ( element_json == NULL ) {
          json_decref( result );
          return NULL;
        }

        if ( json_array_append_new( result, element_json ) ) {
          log_err( "Cannot append element to array" );
          json_decref( result );
          return NULL;
        }
      }

      return result;
    }

    case JEDEX_MAP: {
      int rc;
      size_t element_count, i;
      json_t *result = json_object();
      if ( result == NULL ) {
        log_err( "Cannot allocate JSON map" );
        return NULL;
      }

      rc = jedex_value_get_size( value, &element_count );
      if ( rc != 0 ) {
        json_decref( result );
        return NULL;
      }

      for ( i = 0; i < element_count; i++ ) {
        const char *key;
        jedex_value element;

        rc = jedex_value_get_by_index( value, i, &element, &key );
        if ( rc != 0 )  {
          json_decref( result );
          return NULL;
        }

        json_t *element_json = jedex_value_to_json_t( &element );
        if ( element_json == NULL ) {
          json_decref( result );
          return NULL;
        }

        if ( json_object_set_new( result, key, element_json ) ) {
          log_err( "Cannot append element to map" );
          json_decref( result );
          return NULL;
        }
      }

      return result;
    }

    case JEDEX_RECORD: {
      int rc;
      size_t field_count, i;
      json_t *result = json_object();
      if ( result == NULL ) {
        log_err( "Cannot allocate new JSON record" );
        return NULL;
      }

      rc = jedex_value_get_size( value, &field_count );
      if ( rc != 0 ) {
        json_decref( result );
        return NULL;
      }

      for ( i = 0; i < field_count; i++ ) {
        const char *field_name;
        jedex_value field;

        rc = jedex_value_get_by_index( value, i, &field, &field_name );
        if ( rc != 0 ) {
          json_decref( result );
          return NULL;
        }

        json_t *field_json = jedex_value_to_json_t( &field );
        if ( field_json == NULL ) {
          json_decref( result );
          return NULL;
        }

        if ( json_object_set_new( result, field_name, field_json ) ) {
          log_err( "Cannot append field to record" );
          json_decref( result );
          return NULL;
        }
      }
      const char *record_name = jedex_schema_type_name( jedex_value_get_schema( value ) );
      json_t *schema_obj = json_object();
      json_object_set_new( schema_obj, record_name, result );

      return schema_obj;
    }

    case JEDEX_UNION: {
      int rc;
      json_t *result = json_array();
      if ( result == NULL ) {
        log_err( "Cannot allocate new JSON array" );
        return NULL;
      }

      size_t branch_count;
      rc = jedex_value_get_size( value, &branch_count );
      if ( rc != 0 ) {
        json_decref( result );
        return NULL;
      }

      jedex_schema *union_schema;
      union_schema = jedex_value_get_schema( value );

      size_t i = UINT32_MAX;
      jedex_value branch;
      while ( !jedex_generic_next_branch( value, &i, &branch ) ) {
        jedex_schema *branch_schema;
        const char *branch_name;

        branch_schema = jedex_schema_union_branch( union_schema, i );
        json_t *branch_obj = json_object();
        if ( branch_obj == NULL ) {
          log_err( "Can not allocate new JSON object" );
          json_decref( result );
          return NULL;
        }

        json_t *branch_json = jedex_value_to_json_t( &branch );
        if ( branch_json == NULL ) {
          json_decref( result );
          return NULL;
        }

        branch_name = jedex_schema_type_name( branch_schema );
        if ( json_object_set_new( branch_obj, branch_name, branch_json ) ) {
          log_err( "Cannot set branch object" );
          json_decref( result );
          return NULL;
        }
        if ( json_array_append_new( result, branch_obj ) ) {
          log_err( "Cannot append branch to array" );
          json_decref( result );
          return NULL;
        }
      }

      return result;
    }
    default:
      return NULL;
  }
}


static json_t *
jedex_value_primitive( const jedex_value *value, int *failed ) {
  json_t *result = json_object();

  switch ( jedex_value_get_type( value ) ) {
    case JEDEX_BOOLEAN: {
      int val;
      check_return( NULL, jedex_value_get_boolean( value, &val ) );
      if ( json_object_set_new( result, "boolean", val? json_true() : json_false() ) ) {
        set_fail( result, "boolean" );
      }

      return result;
    }
    case JEDEX_BYTES: {
      const void *val;
      size_t size;
      void *encoded = NULL;
      size_t encoded_size = 0;

      check_return( NULL, jedex_value_get_bytes( value, &val, &size ) );

      if ( encode_utf8_bytes( val, size, &encoded, &encoded_size ) ) {
        return NULL;
      }

      json_t *string = json_string_nocheck( ( const char * ) encoded );
      jedex_free( encoded );
      if ( string == NULL ) {
        set_fail( result, "bytes" );
      }
      if ( json_object_set_new( result, "bytes", string ) ) {
        set_fail( result, "bytes" );
      }
  
      return result;
    }
    case JEDEX_DOUBLE: {
      double val;
      check_return( NULL, jedex_value_get_double( value, &val ) );
      if ( json_object_set_new( result, "double", json_real( val ) ) ) {
        set_fail( result, "double" );
      }

      return result;
    }

    case JEDEX_FLOAT: {
      float val;
      check_return( NULL, jedex_value_get_float( value, &val ) );
      if ( json_object_set_new( result, "float", json_real( val ) ) ) {
        set_fail( result, "float" );
      }

      return result;
    }

    case JEDEX_INT32: {
      int32_t val;
      check_return( NULL, jedex_value_get_int( value, &val ) );
      if ( json_object_set_new( result, "int32", json_integer( val ) ) ) {
        set_fail( result, "int32" );
      }

      return result;
    }

    case JEDEX_INT64: {
      int64_t val;
      check_return( NULL, jedex_value_get_long( value, &val ) );
      if ( json_object_set_new( result, "int64", json_integer( val ) ) ) {
        set_fail( result, "int64" );
      }

      return result;
    }

    case JEDEX_NULL: {
      check_return( NULL, jedex_value_get_null( value ) );
      if ( json_object_set_new( result, "null", json_null() ) ) {
        set_fail( result, "null" );
      }

      return result;
    }

    case JEDEX_STRING: {
      const char *val;
      size_t size;
      check_return( NULL, jedex_value_get_string( value, &val, &size ) );
      if ( json_object_set_new( result, "string", json_string( val ) ) ) {
        set_fail( result, "string" );
      }

      return result;
    }
    default: {
      json_decref( result );
  
      return NULL;
    }
  }
}


int
jedex_value_to_json( const jedex_value *value, int one_line, char **json_str ) {
  check_param( EINVAL, value, "value" );
  check_param( EINVAL, json_str, "string buffer" );

  json_t *json;
  int failed = 0;

  json = jedex_value_primitive( value, &failed );
  if ( failed ) {
    return ENOMEM;
  }
  else if ( json == NULL ) {
    json = jedex_value_to_json_t( value );
    if ( json == NULL ) {
      return ENOMEM;
    }
  }

  /*
   * Jansson will only encode an object or array as the root
   * element.
   */
  *json_str = json_dumps( json,
                          JSON_ENCODE_ANY |
                          JSON_INDENT( one_line? 0: 2 ) |
                          JSON_ENSURE_ASCII |
                          JSON_PRESERVE_ORDER);
  json_decref( json );

  return 0;
}


int
jedex_value_from_json_root( const json_t *json, const jedex_schema *schema, jedex_value *value ) {
  json_type type;

  if ( json_is_array( json ) ) {
    size_t items;
    items = json_array_size( json );
    for ( size_t i = 0; i < items; i++ ) {
      json_t *element = json_array_get( json, i );
      const jedex_schema *items_schema = schema;
      if ( is_jedex_array( schema ) ) {
        items_schema = jedex_schema_array_items( schema );
      }
      int rval;
      rval = jedex_value_from_json_root( element, items_schema, value );
      if ( rval ) {
        return rval;
      }
    }
  } 
  else if ( json_is_object( json ) ) {
    if ( is_jedex_union( schema ) ) {
      size_t branch_count = jedex_schema_union_size( schema );
      for ( size_t i = 0; i < branch_count; i++ ) {
        const jedex_schema *branch_schema = jedex_schema_union_branch( schema, i );
        if ( is_jedex_record( branch_schema ) ) {
          const char *name = ( jedex_schema_to_record( branch_schema ) )->name;
          if ( name != NULL ) {
            int rval;
            rval = jedex_value_from_json_root( json_object_get( json, name ), branch_schema, value );
            if ( rval ) {
              return rval;
            }
          }
        }
      }
    }

    if ( is_jedex_record( schema ) ) {
      size_t field_count = jedex_schema_record_size( schema );
      for ( size_t i = 0; i < field_count; i++ ) {
        const char *field_name = jedex_schema_record_field_name( schema, i );
        if ( field_name != NULL ) {
          jedex_schema *field_schema = jedex_schema_record_field_get_by_index( schema, i );
          int rval;
          rval = jedex_value_from_json_root( json_object_get( json, field_name ), field_schema, value );
          if ( rval ) {
            return rval;
          }
        }
      }
    }
  }
  else if ( json_is_string( json ) ) {
    const char *value = json_string_value( json );
    printf( "value = %s\n", value );
  }
  else if ( json_is_integer( json ) ) {
    json_int_t int_val = json_integer_value( json );
  }
  else if ( json_is_real( json ) ) {
    double real = json_real_value( json );
    printf( "real %f\n", real );
  }
  else if ( json_is_boolean( json ) ) {
    printf( "bool val %d\n", json_is_boolean( json ) );
  }

  return 0;
}


jedex_value *
json_to_jedex_value( void *schema, const char **sub_schema_names, const char *json ) {
  if ( json ) {
    json_error_t json_error;
    json_t *root = json_loads( json, JSON_DECODE_ANY, &json_error );
    if ( root != NULL ) {
      const char *key;
      json_t *value;
      const char *schema_name = jedex_schema_type_name( schema );
      json_object_foreach( root, key, value ) {
        if ( !strcmp( schema_name, key ) ) {
          jedex_value_iface *val_iface = jedex_generic_class_from_schema( schema );
        }
        if ( json_is_object( value ) ) {
        }
      }
    }
  }
#ifdef LATER
  jedex_parcel *parcel = jedex_parcel_create( schema, sub_schema_names );
  for ( ; *sub_schema_names; sub_schema_names++ ) {
    jedex_value *val = jedex_parcel_value( parcel, sub_schema_names );
   
  }
#endif

  return NULL;
}

  

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
