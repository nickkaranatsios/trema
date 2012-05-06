/*
 * Trema messenger library.
 *
 * Author: Toshio Koide
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


#ifndef MESSENGER_H
#define MESSENGER_H


#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "checks.h"
#include "bool.h"
#include "doubly_linked_list.h"
#include "messenger_common.h"
#include "message_buffer.h"
#include "messenger_context.h"
#include "messenger_send.h"
#include "messenger_recv.h"
#include "messenger_dump.h"






extern bool ( *rename_message_received_callback )( const char *old_service_name, const char *new_service_name );
extern bool ( *delete_message_received_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len ) );
extern bool ( *add_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) );
extern bool ( *delete_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) );
extern bool ( *delete_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) );
extern bool ( *send_message )( const char *service_name, const uint16_t tag, const void *data, size_t len );
extern bool ( *send_request_message )( const char *to_service_name, const char *from_service_name, const uint16_t tag, const void *data, size_t len, void *user_data );
extern bool ( *send_reply_message )( const messenger_context_handle *handle, const uint16_t tag, const void *data, size_t len );
extern bool ( *clear_send_queue ) ( const char *service_name );

bool init_messenger( const char *working_directory );
bool finalize_messenger( void );

bool start_messenger( void );
int flush_messenger( void );
bool stop_messenger( void );


#endif // MESSENGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
