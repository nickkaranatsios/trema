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


void
jedex_raw_array_init( jedex_raw_array *array, size_t element_size ) {
  memset( array, 0, sizeof( jedex_raw_array ) );
  array->element_size = element_size;
}


void
jedex_raw_array_done( jedex_raw_array *array ) {
  if ( array->data ) {
    jedex_free( array->data );
  }
  memset( array, 0, sizeof( jedex_raw_array ) );
}


void
jedex_raw_array_clear( jedex_raw_array *array ) {
  array->element_count = 0;
}


int
jedex_raw_array_ensure_size( jedex_raw_array *array, size_t desired_count ) {
  size_t required_size = array->element_size * desired_count;
  if ( array->allocated_size >= required_size ) {
    return 0;
  }

  /*
   * Double the old size when reallocating.
   */

  size_t new_size;
  if ( array->allocated_size == 0 ) {
    /*
     * Start with an arbitrary 10 items.
     */

    new_size = 10 * array->element_size;
  }
  else {
    new_size = array->allocated_size * 2;
  }

  if ( required_size > new_size ) {
    new_size = required_size;
  }

  array->data = jedex_realloc( array->data, new_size );
  if ( array->data == NULL ) {
    log_err( "Cannot allocate space in array for %zu elements", desired_count );
    return ENOMEM;
  }
  array->allocated_size = new_size;

  return 0;
}


void *
jedex_raw_array_append( jedex_raw_array *array ) {
  int rval;

  rval = jedex_raw_array_ensure_size( array, array->element_count + 1 );
  if ( rval ) {
    return NULL;
  }

  size_t offset = array->element_size * array->element_count;
  array->element_count++;

  return ( char * ) array->data + offset;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
