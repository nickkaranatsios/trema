/*
 * Message buffering before sending to socket
 *
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
 *
 * Copyright (C) 2008-2012 NEC Corporation
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


#include <assert.h>
#include <pthread.h>
#include "wrapper.h"
#include "log.h"
#include "message_buffer.h"


message_buffer *
create_message_buffer( size_t size ) {
  message_buffer *buf = xmalloc( sizeof( message_buffer ) );

  buf->buffer = xmalloc( size );
  buf->size = size;
  buf->start = buf->buffer;
  buf->end = ( char * )buf->start + size;
  buf->tail = buf->start;
  buf->data_length = 0;
  buf->head_offset = 0;

  return buf;
}


void
free_message_buffer( message_buffer *buf ) {
  assert( buf != NULL );

  xfree( buf->buffer );
  xfree( buf );
}


void*
get_message_buffer_head( message_buffer *buf ) {
  return ( char * ) buf->buffer + buf->head_offset;
}


void *
get_message_buffer_tail( message_buffer *buf, uint32_t len ) {
  uint32_t remaining_len = ( uint32_t ) ( ( char * ) buf->end - ( char * ) buf->tail );

  if ( remaining_len < len ) {
    return buf->start;
  }
  return buf->tail;
}


size_t
message_buffer_remain_bytes( message_buffer *buf ) {
  assert( buf != NULL );

  return buf->size - buf->data_length;
}



void
write_message_buffer_at_tail( message_buffer *buf, const void *hdr, size_t hdr_len, const void *body, size_t body_len ) {
  memcpy( buf->tail, hdr, hdr_len );
  buf->tail = ( char * ) buf->tail + hdr_len;
  memcpy( buf->tail, body, body_len );
  buf->tail = ( char * ) buf->tail + body_len;
}


bool
write_message_buffer( message_buffer *buf, const void *data, size_t len ) {
  assert( buf != NULL );

  if ( message_buffer_remain_bytes( buf ) < len ) {
    return false;
  }

  if ( ( buf->head_offset + buf->data_length + len ) <= buf->size ) {
    memcpy( ( char * ) get_message_buffer_head( buf ) + buf->data_length, data, len );
  }
  else {
    memmove( buf->buffer, ( char * ) get_message_buffer_head( buf ), buf->data_length );
    buf->head_offset = 0;
    memcpy( ( char * ) buf->buffer + buf->data_length, data, len );
  }
  buf->data_length += len;

  return true;
}


void
truncate_message_buffer( message_buffer *buf, size_t len ) {
  assert( buf != NULL );

  if ( len == 0 || buf->data_length == 0 ) {
error( "truncate_message_buffer len %d data length %d", len, buf->data_length );
    return;
  }

  if ( len > buf->data_length ) {
    len = buf->data_length;
  }

  if ( ( buf->head_offset + len ) <= buf->size ) {
    buf->head_offset += len;
  }
  else {
    memmove( buf->buffer, ( char * ) buf->buffer + buf->head_offset + len, buf->data_length - len );
    buf->head_offset = 0;
  }
  buf->data_length -= len;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
