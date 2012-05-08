/*
 * Declare message buffer used by messenger send/recv modules.
 *
 * Author: Nick Karanatsios
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


#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

typedef struct message_buffer {
  void *buffer;
  void *start;
  void *tail;
  void *end;
  size_t data_length;
  size_t size;
  size_t head_offset;
} message_buffer;


message_buffer *create_message_buffer( size_t size );
void free_message_buffer( message_buffer *buf );
void* get_message_buffer_head( message_buffer *buf );
void* get_message_buffer_tail( message_buffer *buf, uint32_t len );
size_t message_buffer_remain_bytes( message_buffer *buf );
bool write_message_buffer( message_buffer *buf, const void *data, size_t len );
void *write_message_buffer_at_tail( void *tail, const void *data, size_t len );
void truncate_message_buffer( message_buffer *buf, size_t len );


#endif // MESSAGE_BUFFER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
