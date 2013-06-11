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


#include "allocation.h"


void *
jedex_allocator( void *ptr, size_t nsize ) {
  if ( nsize == 0 ) {
    free( ptr );
    return NULL;
  }
  else {
    return realloc( ptr, nsize );
  }
}


void *
jedex_calloc( size_t count, size_t size ) {
  void *ptr = jedex_malloc( count * size );
  if ( ptr != NULL ) {
    memset( ptr, 0, count * size );
  }

  return ptr;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
