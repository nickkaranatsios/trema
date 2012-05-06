/*
 * Messenger receive structures and function prototypes.
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


#ifndef MESSENGER_RECV_H
#define MESSENGER_RECV_H


#define MESSENGER_RECV_BUFFER 100000


typedef struct messenger_socket {
  int fd;
} messenger_socket;


typedef struct receive_queue_callback {
  void  *function;
  uint8_t message_type;
} receive_queue_callback;


typedef struct receive_queue {
  char service_name[ MESSENGER_SERVICE_NAME_LENGTH ];
  dlist_element *message_callbacks;
  int listen_socket;
  struct sockaddr_un listen_addr;
  dlist_element *client_sockets;
  message_buffer *buffer;
} receive_queue;


void init_messenger_recv( const char *working_directory );
bool ( *add_message_received_callback )( const char *service_name, const callback_message_received function );
bool ( *rename_message_received_callback )( const char *old_service_name, const char *new_service_name );
bool ( *delete_message_received_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len ) );
bool ( *add_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) );
bool ( *delete_message_requested_callback )( const char *service_name, void ( *callback )( const messenger_context_handle *handle, uint16_t tag, void *data, size_t len ) );
bool ( *add_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) );
bool ( *delete_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) );
void delete_all_receive_queues();


#endif // MESSENGER_RECV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
