/*
 * Messenger send structures and function prototypes.
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


#ifndef MESSENGER_SEND_H
#define MESSENGER_SEND_H


#include <sys/socket.h>
#include <sys/un.h>
#include "message_buffer.h"


#define MESSENGER_SEND_BUFFER 100000


#define THREADS 2
#define ITEM_SIZE 512
#define MAX_TAKE 32

#define ARRAY_SIZE( x ) ( int32_t ) ( sizeof( x ) / sizeof( x[ 0 ] ) )


#define MEM_BLOCK 4096
#define MAX_MEM_DESC 5
typedef struct memory_region memory_region;


struct memory_region {
  void *tail;
  void *head;
  void *last_accessed;
  uint32_t start;
  uint32_t end;
  uint32_t left;
  uint32_t size;
};


typedef struct memory_desc memory_desc;


struct memory_desc {
  memory_region *mregion;
};


struct job_opt {
  char service_name[ MESSENGER_SERVICE_NAME_LENGTH ];
  int server_socket;
  void *buffer;
  uint32_t buffer_len;
};


typedef struct job_item job_item;


struct job_item {
  struct job_opt opt;
  pthread_t tag_id;
  uint16_t done;
};


#define DONE 0
#define END  DONE + 1
typedef struct thread_ctrl thread_ctrl;


struct thread_ctrl {
  int job_tag[ 2 ];
  int job_value[ 2 ];
};


typedef struct job_ctrl job_ctrl;
/*
 * one job control structure for each service for all job items
 */
struct job_ctrl {
  /*
   * protect shared job item variables.
  */
  pthread_mutex_t mutex;
  
  /*
   * incremented by one after a new job item is added to item.
  */
  int job_end;
  /*
   * incremented by one after a job item has been retrieved from item for 
   * processing.
  */
  int job_start;
  /*
   * incremented by one when a message has been transmitted completed 
   * processing.
  */
  int job_done;
  /*
   * used for lock-free access
  */
  pthread_t tag_id;
  thread_ctrl thread_ctrl[ THREADS ];
  /*
  * an array of job items to handle.
  * The client that is connected to the service uses adds items to this array.
  */
  job_item item[ ITEM_SIZE ];
};


typedef struct send_queue {
  struct timespec reconnect_interval;
  struct sockaddr_un server_addr;
  char service_name[ MESSENGER_SERVICE_NAME_LENGTH ];
  uint64_t overflow_total_length;
  message_buffer *buffer;
  int server_socket;
  int refused_count;
  bool running_timer;
  int socket_buffer_size;
  uint32_t overflow;
} send_queue;


void init_messenger_send( const char *working_directory );
void start_messenger_send( void );
bool ( *delete_message_replied_callback )( const char *service_name, void ( *callback )( uint16_t tag, void *data, size_t len, void *user_data ) );
bool ( *send_message )( const char *service_name, const uint16_t tag, const void *data, size_t len );
bool ( *send_request_message )( const char *to_service_name, const char *from_service_name, const uint16_t tag, const void *data, size_t len, void *user_data );
bool ( *send_reply_message )( const messenger_context_handle *handle, const uint16_t tag, const void *data, size_t len );
void number_of_send_queue( int *connected_count, int *sending_count, int *reconnecting_count, int *closed_count );
void delete_send_service( const char *service_name );
void delete_send_queue( send_queue *sq );
void delete_all_send_queues();


#endif // MESSENGER_SEND_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
