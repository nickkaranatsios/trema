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


static void
jedex_wrapped_copy_free( jedex_wrapped_buffer *self ) {
  struct jedex_wrapped_copy *copy = ( struct jedex_wrapped_copy * ) self->user_data;
  jedex_free( copy );
}


static int
jedex_wrapped_copy_copy( jedex_wrapped_buffer *dest,
                         const jedex_wrapped_buffer *src,
                         size_t offset,
                         size_t length ) {
  struct jedex_wrapped_copy *copy = ( struct jedex_wrapped_copy * ) src->user_data;
  dest->buf = ( char * ) src->buf + offset;
  dest->size = length;
  dest->user_data = copy;
  dest->free = jedex_wrapped_copy_free;
  dest->copy = jedex_wrapped_copy_copy;
  dest->slice = NULL;

  return 0;
}


int
jedex_wrapped_buffer_new( jedex_wrapped_buffer *dest, void *buf, size_t length ) {
  dest->buf = buf;
  dest->size = length;
  dest->user_data = NULL;
  dest->free = NULL;
  dest->copy = NULL;
  dest->slice = NULL;

  return 0;
}


void
jedex_wrapped_buffer_move( jedex_wrapped_buffer *dest, jedex_wrapped_buffer *src ) {
  memcpy( dest, src, sizeof( jedex_wrapped_buffer ) );
  memset( src, 0, sizeof( jedex_wrapped_buffer ) );
}




int
jedex_wrapped_buffer_new_copy( jedex_wrapped_buffer *dest, const void *buf, size_t length ) {
  size_t allocated_size = sizeof( struct jedex_wrapped_copy ) + length;
  struct jedex_wrapped_copy *copy = ( struct jedex_wrapped_copy * ) jedex_malloc( allocated_size );
  if ( copy == NULL ) {
    return ENOMEM;
  }

  dest->buf = ( ( char * ) copy ) + sizeof( struct jedex_wrapped_copy );
  dest->size = length;
  dest->user_data = copy;
  dest->free = jedex_wrapped_copy_free;
  dest->copy = jedex_wrapped_copy_copy;
  dest->slice = NULL;

  copy->allocated_size = allocated_size;
  memcpy( ( void *)  dest->buf, buf, length );

  return 0;
}


int
jedex_wrapped_buffer_copy( jedex_wrapped_buffer *dest,
                           const jedex_wrapped_buffer *src,
                           size_t offset,
                           size_t length ) {
  if (offset > src->size) {
    log_err( "Invalid offset when slicing buffer" );
    return EINVAL;
  }

  if ( ( offset + length ) > src->size ) {
    log_err( "Invalid length when slicing buffer" );
    return EINVAL;
  }

  if ( src->copy == NULL ) {
    return jedex_wrapped_buffer_new_copy( dest, ( char * ) src->buf + offset, length );
  }
   else {
    return src->copy( dest, src, offset, length );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
