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


#define is_resizable( buf ) \
  (( buf ).free == jedex_wrapped_resizable_free )


static void
jedex_wrapped_resizable_free( jedex_wrapped_buffer *self ) {
  struct jedex_wrapped_resizable *resizable = ( struct jedex_wrapped_resizable * ) self->user_data;
  jedex_free( resizable );
}


static int
jedex_wrapped_resizable_new( jedex_wrapped_buffer *dest, size_t buf_size ) {
  size_t allocated_size = jedex_wrapped_resizable_size( buf_size );
  struct jedex_wrapped_resizable *resizable = ( struct jedex_wrapped_resizable * ) jedex_malloc( allocated_size );
  if ( resizable == NULL ) {
    return ENOMEM;
  }

  resizable->buf_size = buf_size;

  dest->buf = ( ( char * ) resizable ) + sizeof( struct jedex_wrapped_resizable );
  dest->size = buf_size;
  dest->user_data = resizable;
  dest->free = jedex_wrapped_resizable_free;
  dest->copy = NULL;
  dest->slice = NULL;

  return 0;
}


static int
jedex_wrapped_resizable_resize( jedex_wrapped_buffer *self, size_t desired ) {
  struct jedex_wrapped_resizable *resizable = ( struct jedex_wrapped_resizable * ) self->user_data;

  /*
   * If we've already allocated enough memory for the desired
   * size, there's nothing to do.
   */

  if ( resizable->buf_size >= desired ) {
    return 0;
  }

  size_t new_buf_size = resizable->buf_size * 2;
  if ( desired > new_buf_size ) {
    new_buf_size = desired;
  }

  struct jedex_wrapped_resizable *new_resizable = ( struct jedex_wrapped_resizable * ) jedex_realloc( resizable, jedex_wrapped_resizable_size( new_buf_size ) );
  if ( new_resizable == NULL ) {
    return ENOMEM;
  }

  new_resizable->buf_size = new_buf_size;

  char *old_buf = ( char * ) resizable;
  char *new_buf = ( char * ) new_resizable;

  ptrdiff_t offset = ( char * ) self->buf - old_buf;
  self->buf = new_buf + offset;
  self->user_data = new_resizable;

  return 0;
}


void
jedex_raw_string_init( jedex_raw_string *str ) {
  memset( str, 0, sizeof( jedex_raw_string ) );
}


void
jedex_raw_string_clear( jedex_raw_string *str ) {
  /*
   * If the string's buffer is one that we control, then we don't
   * free it; that lets us reuse the storage on the next call to
   * avro_raw_string_set[_length].
   */

  if ( is_resizable( str->wrapped ) ) {
    str->wrapped.size = 0;
  }
  else {
    jedex_wrapped_buffer_free( &str->wrapped );
    jedex_raw_string_init( str );
  }
}


void
jedex_raw_string_done( jedex_raw_string *str ) {
  jedex_wrapped_buffer_free( &str->wrapped );
  jedex_raw_string_init( str );
}


static int
jedex_raw_string_ensure_buf( jedex_raw_string *str, size_t length ) {
  int rval;

  if ( is_resizable( str->wrapped ) ) {
    /*
     * If we've already got a resizable buffer, just have it
     * resize itself.
     */

    return jedex_wrapped_resizable_resize( &str->wrapped, length );
  }
  else {
    /*
     * Stash a copy of the old wrapped buffer, and then
     * create a new resizable buffer to store our content
     * in.
     */

    jedex_wrapped_buffer orig = str->wrapped;
    check( rval, jedex_wrapped_resizable_new( &str->wrapped, length ) );

    /*
     * If there was any content in the old wrapped buffer,
     * copy it into the new resizable one.
     */

    if ( orig.size > 0 ) {
      size_t to_copy = ( orig.size < length ) ? orig.size : length;
      memcpy( ( void * ) str->wrapped.buf, orig.buf, to_copy );
    }
    jedex_wrapped_buffer_free( &orig );

    return 0;
  }
}


void
jedex_raw_string_set_length( jedex_raw_string *str, const void *src, size_t length ) {
  jedex_raw_string_ensure_buf( str, length + 1 );
  memcpy( ( void * ) str->wrapped.buf, src, length );
  ( ( char * ) str->wrapped.buf )[ length ] = '\0';
  str->wrapped.size = length;
}


void
jedex_raw_string_append_length( jedex_raw_string *str, const void *src, size_t length ) {
  if ( jedex_raw_string_length( str ) == 0 ) {
    return jedex_raw_string_set_length( str, src, length );
  }

  jedex_raw_string_ensure_buf( str, str->wrapped.size + length );
  memcpy( ( char * ) str->wrapped.buf + str->wrapped.size, src, length );
  str->wrapped.size += length;
}


void
jedex_raw_string_give( jedex_raw_string *str, jedex_wrapped_buffer *src ) {
  jedex_wrapped_buffer_free( &str->wrapped );
  jedex_wrapped_buffer_move( &str->wrapped, src );
}


int
jedex_raw_string_grab( const jedex_raw_string *str, jedex_wrapped_buffer *dest ) {
  return jedex_wrapped_buffer_copy(dest, &str->wrapped, 0, str->wrapped.size );
}


int
jedex_raw_string_equals( const jedex_raw_string *str1, const jedex_raw_string *str2 ) {
  if ( str1 == str2 ) {
    return 1;
  }

  if ( !str1 || !str2 ) {
    return 0;
  }

  if ( str1->wrapped.size != str2->wrapped.size ) {
    return 0;
  }

  return ( memcmp( str1->wrapped.buf, str2->wrapped.buf, str1->wrapped.size ) == 0 );
}


void
jedex_raw_string_set( jedex_raw_string *str, const char *src ) {
  size_t length = strlen( src );
  jedex_raw_string_ensure_buf( str, length + 1 );
  memcpy( ( void * ) str->wrapped.buf, src, length + 1 );
  str->wrapped.size = length + 1;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
