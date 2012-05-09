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
#define ITEM_SIZE 256
#define ARRAY_SIZE( x ) ( int32_t ) ( sizeof( x ) / sizeof( x[ 0 ] ) )
#define alloc_nr( x ) ( ( ( x ) + 8 ) * 3 / 2 )
#define ALLOC_GROW( x, nr, alloc ) \
 do { \
    if ( ( nr ) > alloc ) { \
      if ( alloc_nr( alloc ) < ( nr ) ) \
        alloc = ( nr ); \
      else \
        alloc = alloc_nr( alloc ); \
      x = realloc( ( x ), alloc * sizeof( *( x ) ) ); \
    } \
  } while ( 0 )


struct job_opt {
  void *buffer;
  uint32_t buffer_len;
};


struct job_item {
  struct job_opt opt;
  uint16_t done;
};


/*
 * one job control structure for each service for all job items
 */
struct job_ctrl {
  /*
   * protect shared job item variables.
  */
  pthread_mutex_t mutex;
  /* 
   * used to signal when a new job item is added.
  */
  pthread_cond_t cond_add;
  /* 
   * signal when the result of one item is written to socket.
  */
  pthread_cond_t cond_write;
  /* 
   * signal when we finished with all job items.
  */
  pthread_cond_t cond_result;
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
   * incremented by one when a job item has been processed from item.
  */
  int job_done;
  /*
   * the server socket to send this job item.
  */
  int server_socket;
  /*
  * an array of job items to handle.
  * The client that is connected to the service uses adds items to this array.
  */
  struct job_item item[ ITEM_SIZE ];
};
  

typedef struct send_queue {
  char service_name[ MESSENGER_SERVICE_NAME_LENGTH ];
  int server_socket;
  int refused_count;
  struct timespec reconnect_interval;
  struct sockaddr_un server_addr;
  message_buffer *buffer;
  bool running_timer;
  uint32_t overflow;
  uint64_t overflow_total_length;
  int socket_buffer_size;
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
