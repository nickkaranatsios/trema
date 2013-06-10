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
  int ( *give_string_len ) (const jedex_value_iface *iface, void *self, jedex_wrapped_buffer *buf );

  /*-------------------------------------------------------------
   * Compound value getters
   */

  /* Number of elements in array/map, or the number of fields in a
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
  int ( *get_by_index ) ( const jedex_value_iface *iface, const void *self, size_t index, jedex_value *child, const char **name );

  /*
   * For maps, returns the element with the given key.  For
   * records, returns the element with the given field name.  If
   * index is given, it will be filled in with the numeric index
   * of the returned value.
   */
  int ( *get_by_name ) ( const jedex_value_iface *iface, const void *self, const char *name, jedex_value *child, size_t *index );

  /* Discriminant of current union value */
  int ( *get_discriminant ) ( const jedex_value_iface *iface, const void *self, int *out );
  /* Current union value */
  int ( *get_current_branch ) ( const jedex_value_iface *iface, const void *self, jedex_value *branch );

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

  /* Select a union branch. */
  int ( *set_branch ) ( const jedex_value_iface *iface, void *self, int discriminant, jedex_value *branch );
};


CLOSE_EXTERN
#endif // VALUE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
