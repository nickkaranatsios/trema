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


#ifndef BASICS_H
#define BASICS_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


enum jedex_type {
  JEDEX_STRING,
  JEDEX_BYTES,
  JEDEX_INT32,
  JEDEX_INT64,
  JEDEX_FLOAT,
  JEDEX_DOUBLE,
  JEDEX_BOOLEAN,
  JEDEX_NULL,
  JEDEX_RECORD,
  JEDEX_MAP,
  JEDEX_ARRAY,
  JEDEX_UNION,
  JEDEX_LINK,
  JEDEX_INVALID
};

typedef enum jedex_type jedex_type;

enum jedex_class {
  JEDEX_SCHEMA,
  JEDEX_DATUM
};

typedef enum jedex_class jedex_class;

struct jedex_obj {
  jedex_type type;
  jedex_class class_type;
};


#define jedex_classof( obj )     ( ( obj )->class_type )
#define is_jedex_schema( obj )   ( obj && jedex_classof( obj ) == JEDEX_SCHEMA )
#define is_jedex_datum( obj )    ( obj && jedex_classof( obj ) == JEDEX_DATUM )

#define jedex_typeof( obj )      ( ( obj )->type )
#define is_jedex_string( obj )   ( obj && jedex_typeof( obj ) == JEDEX_STRING )
#define is_jedex_bytes( obj )    ( obj && jedex_typeof( obj ) == JEDEX_BYTES )
#define is_jedex_int32( obj )    ( obj && jedex_typeof( obj ) == JEDEX_INT32 )
#define is_jedex_int64( obj )    ( obj && jedex_typeof( obj ) == JEDEX_INT64 )
#define is_jedex_float( obj )    ( obj && jedex_typeof( obj ) == JEDEX_FLOAT )
#define is_jedex_double( obj )   (obj && jedex_typeof( obj ) == JEDEX_DOUBLE )
#define is_jedex_boolean( obj )  ( obj && jedex_typeof( obj ) == JEDEX_BOOLEAN )
#define is_jedex_null( obj )     ( obj && jedex_typeof( obj ) == JEDEX_NULL )
#define is_jedex_primitive( obj )( is_jedex_string( obj ) \
                             || is_jedex_bytes( obj ) \
                             || is_jedex_int32( obj ) \
                             || is_jedex_int64( obj ) \
                             || is_jedex_float( obj ) \
                             || is_jedex_double( obj ) \
                             || is_jedex_boolean( obj ) \
                             || is_jedex_null( obj ) )
#define is_jedex_record( obj )   ( obj && jedex_typeof( obj ) == JEDEX_RECORD )
#define is_jedex_named_type( obj ) ( is_jedex_record( obj ) )
#define is_jedex_map( obj ) ( obj && jedex_typeof( obj ) == JEDEX_MAP )
#define is_jedex_array( obj ) ( obj && jedex_typeof( obj ) == JEDEX_ARRAY )
#define is_jedex_union( obj ) ( obj && jedex_typeof( obj ) == JEDEX_UNION )
#define is_jedex_link( obj ) ( obj && jedex_typeof( obj ) == JEDEX_LINK )
#define is_jedex_complex_type( obj ) ( !( is_jedex_primitive( obj ) )


CLOSE_EXTERN
#endif // BASICS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
