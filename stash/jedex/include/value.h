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


#ifndef VALUE_H
#define VALUE_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "basics.h"
#include "schema.h"
#include "data.h"


typedef struct jedex_value_iface jedex_value_iface;


typedef struct jedex_value {
  jedex_value_iface *iface;
  void *self;
} jedex_value;


struct jedex_value_iface {
  int ( *reset ) ( const jedex_value_iface *iface, void *self );
  jedex_type ( *get_type ) ( const jedex_value_iface *iface, const void *self );
  jedex_schema *( *get_schema ) ( const jedex_value_iface *iface, const void *self );
  bool ( *is_primary ) ( const jedex_value_iface *iface, void *self, const char *name );

  int ( *get_boolean ) ( const jedex_value_iface *iface, const void *self, int *out );
  int ( *get_bytes ) ( const jedex_value_iface *iface, const void *self, const void **buf, size_t *size );
  int ( *grab_bytes ) ( const jedex_value_iface *iface, const void *self, jedex_wrapped_buffer *dest );
  int ( *get_double ) ( const jedex_value_iface *iface, const void *self, double *out );
  int ( *get_float ) ( const jedex_value_iface *iface, const void *self, float *out );
  int ( *get_int ) ( const jedex_value_iface *iface, const void *self, int32_t *out );
  int ( *get_long ) ( const jedex_value_iface *iface, const void *self, int64_t *out );
  int ( *get_null ) ( const jedex_value_iface *iface, const void *self );
  /* The result will be NUL-terminated; the size will INCLUDE the
   * NUL terminator.  str will never be NULL unless there's an
   * error. */
  int ( *get_string ) ( const jedex_value_iface *iface, const void *self, const char **str, size_t *size );
  int ( *grab_string ) ( const jedex_value_iface *iface, const void *self, jedex_wrapped_buffer *dest );

  int ( *set_boolean ) ( const jedex_value_iface *iface, void *self, int val );
  int ( *set_bytes ) ( const jedex_value_iface *iface, void *self, void *buf, size_t size );
  int ( *give_bytes ) ( const jedex_value_iface *iface, void *self, jedex_wrapped_buffer *buf );
  int ( *set_double ) ( const jedex_value_iface *iface, void *self, double val );
  int ( *set_float ) ( const jedex_value_iface *iface, void *self, float val );
  int ( *set_int ) ( const jedex_value_iface *iface, void *self, int32_t val );
  int ( *set_long ) ( const jedex_value_iface *iface, void *self, int64_t val );
  int ( *set_null ) ( const jedex_value_iface *iface, void *self );
  /* The input must be NUL-terminated */
  int ( *set_string ) ( const jedex_value_iface *iface, void *self, const char *str );
  /* and size must INCLUDE the NUL terminator */
  int ( *set_string_len ) ( const jedex_value_iface *iface, void *self, const char *str, size_t size );
  int ( *give_string_len ) ( const jedex_value_iface *iface, void *self, jedex_wrapped_buffer *buf );

  /*-------------------------------------------------------------
   * Compound value getters
   */

  /* Number of elements in array/map/union, or the number of fields in a
   * record. */
  int ( *get_size ) ( const jedex_value_iface *iface, const void *self, size_t *size );

  /*
   * For arrays and maps, returns the element with the given
   * index.  (For maps, the "index" is based on the order that the
   * keys were added to the map.)  For records, returns the field
   * with that index in the schema.
   *
   * For maps and records, the name parameter (if given) will be
   * filled in with the key or field name of the returned value.
   * For arrays, the name parameter will always be ignored.
   */
  int ( *get_by_index ) ( const jedex_value_iface *iface, void *self, size_t index, jedex_value *child, const char **name );

  /*
   * For maps, returns the element with the given key.  For
   * records, returns the element with the given field name.  If
   * index is given, it will be filled in with the numeric index
   * of the returned value.
   */
  int ( *get_by_name ) ( const jedex_value_iface *iface, void *self, const char *name, jedex_value *child, size_t *index );

  /* Current union value */
  int ( *get_branch ) ( const jedex_value_iface *iface, void *self, size_t index, jedex_value *branch );
  /* current selected branch index */
  int ( *get_discriminant ) ( const jedex_value_iface *iface, const void *self, int *out );

  /*-------------------------------------------------------------
   * Compound value setters
   */

  /*
   * For all of these, the value class should know which class to
   * use for its children.
   */

  /* Creates a new array element. */
  int ( *append ) ( const jedex_value_iface *iface, void *self, jedex_value *child_out, size_t *new_index );

  /* Creates a new map element, or returns an existing one. */
  int ( *add ) ( const jedex_value_iface *iface, void *self, const char *key, jedex_value *child, size_t *index, int *is_new );
};


int jedex_value_to_json( const jedex_value *value, bool one_line, char **json_str );
jedex_value *jedex_value_from_iface( jedex_value_iface *val_iface );
jedex_value *json_value_to_json( const void *schema, const char **sub_schema_names, const char *json );
jedex_value *json_to_jedex_value( void *schema, const char *json );


#define jedex_value_call0( value, method, dflt ) \
    ( ( value )->iface->method == NULL ? ( dflt ) : \
     ( value )->iface->method( ( value )->iface, ( value )->self ) )

#define jedex_value_call( value, method, dflt, ... ) \
    ( ( value )->iface->method == NULL ? ( dflt ) : \
     ( value )->iface->method( ( value )->iface, ( value )->self, __VA_ARGS__ ) )

#define jedex_value_reset( value ) \
  jedex_value_call0( value, reset, EINVAL )
#define jedex_value_get_type( value ) \
  jedex_value_call0( value, get_type, ( jedex_type ) -1 )
#define jedex_value_get_schema( value ) \
  jedex_value_call0( value, get_schema, NULL )


#define jedex_value_get_boolean( value, out ) \
  jedex_value_call( value, get_boolean, EINVAL, out )
#define jedex_value_get_bytes( value, buf, size ) \
  jedex_value_call( value, get_bytes, EINVAL, buf, size )
#define jedex_value_grab_bytes( value, dest ) \
  jedex_value_call( value, grab_bytes, EINVAL, dest )
#define jedex_value_get_double( value, out ) \
  jedex_value_call( value, get_double, EINVAL, out )
#define jedex_value_get_float( value, out ) \
  jedex_value_call( value, get_float, EINVAL, out )
#define jedex_value_get_int( value, out ) \
  jedex_value_call( value, get_int, EINVAL, out )
#define jedex_value_get_long( value, out ) \
  jedex_value_call(value, get_long, EINVAL, out)
#define jedex_value_get_null( value ) \
  jedex_value_call0( value, get_null, EINVAL )
#define jedex_value_get_string( value, str, size ) \
  jedex_value_call( value, get_string, EINVAL, str, size )
#define jedex_value_grab_string( value, dest ) \
  jedex_value_call( value, grab_string, EINVAL, dest )


#define jedex_value_get_size( value, size ) \
  jedex_value_call( value, get_size, EINVAL, size )
#define jedex_value_get_by_index( value, idx, child, name ) \
  jedex_value_call( value, get_by_index, EINVAL, idx, child, name )
#define jedex_value_get_by_name( value, name, child, index ) \
  jedex_value_call( value, get_by_name, EINVAL, name, child, index )

#define jedex_value_is_primary( value, name ) \
  jedex_value_call( value, is_primary, false, name )

#define jedex_value_get_discriminant( value, out) \
  jedex_value_call( value, get_discriminant, EINVAL, out )
#define jedex_value_get_branch( value, idx, branch ) \
  jedex_value_call( value, get_branch, EINVAL, idx, branch )

#define jedex_value_set_boolean( value, val ) \
  jedex_value_call( value, set_boolean, EINVAL, val )
#define jedex_value_set_bytes( value, buf, size ) \
  jedex_value_call( value, set_bytes, EINVAL, buf, size )
#define jedex_value_give_bytes( value, buf ) \
  jedex_value_call( value, give_bytes, EINVAL, buf )
#define jedex_value_set_double( value, val ) \
  jedex_value_call( value, set_double, EINVAL, val )
#define jedex_value_set_float( value, val ) \
  jedex_value_call( value, set_float, EINVAL, val )
#define jedex_value_set_int( value, val ) \
  jedex_value_call( value, set_int, EINVAL, val )
#define jedex_value_set_long( value, val ) \
  jedex_value_call( value, set_long, EINVAL, val )
#define jedex_value_set_null( value ) \
  jedex_value_call0( value, set_null, EINVAL )
#define jedex_value_set_string( value, str ) \
  jedex_value_call( value, set_string, EINVAL, str )
#define jedex_value_set_string_len( value, str, size ) \
  jedex_value_call( value, set_string_len, EINVAL, str, size )
#define jedex_value_give_string_len( value, buf ) \
  jedex_value_call( value, give_string_len, EINVAL, buf )


#define jedex_value_append( value, child, new_index ) \
  jedex_value_call( value, append, EINVAL, child, new_index )
#define jedex_value_add( value, key, child, index, is_new ) \
  jedex_value_call( value, add, EINVAL, key, child, index, is_new )


CLOSE_EXTERN
#endif // VALUE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
