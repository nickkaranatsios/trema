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


#define return_json( type, exp )						\
	{								\
		json_t  *result = exp;					\
		if ( result == NULL ) {					\
			log_err( "Cannot allocate JSON " type );	\
		}							\
		return result;						\
	}

#define check_return( retval, call ) \
	do { \
		int  __rc; \
		__rc = call; \
		if ( __rc != 0 ) { \
			return retval; \
		} \
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

			return result;
		}

		case JEDEX_UNION:
		{
			int disc;
			jedex_value branch;
			jedex_schema *union_schema;
			jedex_schema *branch_schema;
			const char *branch_name;

			check_return( NULL, jedex_value_get_current_branch( value, &branch ) );

			if ( jedex_value_get_type( &branch ) == JEDEX_NULL ) {
				return_json( "null", json_null() );
			}

			check_return( NULL, jedex_value_get_discriminant( value, &disc ) );
			union_schema = jedex_value_get_schema( value );
			branch_schema = jedex_schema_union_branch( union_schema, disc );
			branch_name = jedex_schema_type_name( branch_schema );

			json_t *result = json_object();
			if ( result == NULL ) {
				log_err( "Cannot allocate JSON union" );
				return NULL;
			}

			json_t *branch_json = jedex_value_to_json_t( &branch );
			if ( branch_json == NULL ) {
				json_decref( result );
				return NULL;
			}

			if ( json_object_set_new( result, branch_name, branch_json ) ) {
				log_err( "Cannot append branch to union" );
				json_decref( result );
				return NULL;
			}

			return result;
		}

		default:
			return NULL;
	}
}


int
jedex_value_to_json( const jedex_value *value, int one_line, char **json_str ) {
	check_param( EINVAL, value, "value" );
	check_param( EINVAL, json_str, "string buffer" );

	json_t  *json = jedex_value_to_json_t( value );
	if ( json == NULL ) {
		return ENOMEM;
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


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
