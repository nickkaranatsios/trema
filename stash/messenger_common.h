/*
 * Messenger common share definitions.
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



#ifndef MESSENGER_COMMON_H
#define MESSENGER_COMMON_H


#define MESSENGER_SERVICE_NAME_LENGTH 32


typedef void ( *callback_message_received )( uint16_t tag, void *data, size_t len );


enum {
  MESSAGE_TYPE_NOTIFY,
  MESSAGE_TYPE_REQUEST,
  MESSAGE_TYPE_REPLY,
};


typedef struct message_header {
  uint8_t version;         // version = 0 (unused)
  uint8_t message_type;    // MESSAGE_TYPE_
  uint16_t tag;            // user defined
  uint32_t message_length; // message length including header
  uint8_t value[ 0 ];
} message_header;


typedef struct messenger_context_handle {
  uint32_t transaction_id;
  uint16_t service_name_len;
  uint16_t pad;
  char service_name[ 0 ];
} messenger_context_handle;


#endif // MESSENGER_COMMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

