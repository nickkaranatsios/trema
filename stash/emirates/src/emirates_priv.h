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


#ifndef EMIRATES_PRIV_H
#define EMIRATES_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "emirates.h"


#define STATUS_MSG "STATUS:"
#define STATUS_NG  "NG"
#define STATUS_OK "OK"
#define POLL_TIMEOUT 500
#define PUB_BASE_PORT 6000
#define SUB_BASE_PORT 6001
#define REQUESTER_BASE_PORT 6100
#define REQUESTER_ID_SIZE 32
#define RESPONDER_BASE_PORT 6200
#define RESPONDER_ID_SIZE REQUESTER_ID_SIZE

#define SUBSCRIPTION_MSG "SET_SUBSCRIPTION"
#define ADD_SERVICE_REQUEST "ADD_SERVICE_REQUEST"
#define ADD_SERVICE_REPLY "ADD_SERVICE_REPLY"
#define REQUEST "REQUEST"
#define READY "READY"
#define REPLY "REPLY"
#define REPLY_TIMEOUT "REPLY_TIMEOUT"
#define EXIT "EXIT"

#define SERVICE_MAX 64
#define IDENTITY_MAX SERVICE_MAX
#define REQUEST_HEARTBEAT 5000
#define REPLY_TIMER REQUEST_HEARTBEAT + 1000
#define OUTPUT_CTRL_BIT (2U)
#define use_output( q ) ( ( q ) & OUTPUT_CTRL_BIT )
#define enable_output( q ) ( ( q ) |= OUTPUT_CTRL_BIT )
#define disable_output( q ) ( ( q ) &= ~OUTPUT_CTRL_BIT )

#define create_notify( ptr ) \
  do {  \
    int pipe_fd[ 2 ]; \
    if ( pipe( pipe_fd ) < 0 ) { \
      ( ptr )->notify_in = ( ptr )->notify_out = -1; \
    } \
    else { \
      ( ptr )->notify_in = pipe_fd[ 0 ]; \
      ( ptr )->notify_out = pipe_fd[ 1 ]; \
    } \
  } while ( 0 )


#define signal_notify_out( fd ) \
  do { \
    char dummy = 'y'; \
    write( ( fd ), &dummy, sizeof( dummy ) ); \
  } while ( 0 )



typedef void ( subscriber_callback )( void *args );
typedef void ( request_callback )( void *args );
typedef void ( reply_callback )( void *args );
typedef int ( poll_handler )( zmq_pollitem_t *item, void *arg );


typedef struct sub_callback {
  subscriber_callback *callback;
  const char *service;
  const char **sub_schema_names;
  void *schema;
} sub_callback;


typedef struct req_callback {
  request_callback *callback;
  const char *service;
  const char *responder_id;
} req_callback;


typedef struct rep_callback {
  reply_callback *callback;
  const char *service;
  const char *requester_id;
} rep_callback;


typedef struct requester_info {
  void *pipe; // requester's child thread to main thread i/o socket
  void *output; // requester's zmq output socket
  void *requester; // requester's main thread i/o socket
  char *own_id; // a unique id for this requester
  int64_t request_expiry; // an expiry timer associated with an outgoing request
  int output_ctrl; // a flag that throttles output from requester
  int notify_in; // the read end of a pipe fd to signal data is ready to be read from application
  int notify_out; // the write end of a pipe fd used to signal data availability to parent thread
  uint32_t timeout_id; // a timeout id associated with an outgoing request
  uint32_t outstanding_id; // the last oudstanding timeout id
  uint32_t port; // a base port a requester connects to.
} requester_info;


typedef struct responder_info {
  void *pipe; // responder's child thread to main thread i/o socket
  void *output; // responder's zmq output socket
  void *responder; // responder's main thread i/o socket
  char *own_id; // a unique id for this responder
  char client_id[ IDENTITY_MAX ]; // the client id/requester of the last recv'd request
  zframe_t *service_frame; // the service frame of the last recv'd request
  int64_t expiry; // timeouts replies
  int notify_in; // the read end of a pipe fd to signal data is ready to be read from application
  int notify_out; //the write end of a pipe fd used to signal data availability to parent thread
  uint32_t port; // a base port a responder connects to
} responder_info;


typedef struct publisher_info {
  void *pipe; // publisher's child thread to main thread i/o socket
  void *output; // publisher's zmq output socket
  void *publisher; // publisher's main thread i/o socket
  uint32_t port; // a base port a publisher connects to
} publisher_info;


typedef struct subscriber_info {
  zlist_t *callbacks; // a list of user subscription callbacks map to service name
  void *pipe; // subscriber's child thread to main thread i/o socket
  void *output; // subscriber's zmq output socket
  void *subscriber; // subscriber's main thread i/o socket
  int notify_in; // the read end of a pipe fd to to signal subscriber data availalibility
  int notify_out; // the write end of a pipe fd to signal data availability to parent thread
  uint32_t port; // a base port a subscriber connects to
} subscriber_info;


typedef struct emirates_priv {
  zctx_t *ctx;
  poll_handler *sub_handler;
  zlist_t *req_callbacks;
  zlist_t *rep_callbacks;
  requester_info *requester; 
  responder_info *responder;
  publisher_info *publisher;
  subscriber_info *subscriber;
} emirates_priv;


#define requester_socket( self ) ( ( self )->requester )
#define requester_pipe_socket( self ) ( ( self )->pipe )
#define requester_zmq_socket( self ) ( ( self )->output )
#define requester_id( self ) ( ( self )->own_id )
#define requester_output_ctrl( self ) ( ( self )->output_ctrl )
#define requester_port( self ) ( ( self )->port )
#define requester_timeout_id( self ) ( ( self )->timeout_id )
#define requester_outstanding_id( self ) ( ( self )->outstanding_id )
#define requester_inc_timeout_id( self ) ( requester_timeout_id( self )++ )
#define requester_notify_in( self ) ( ( self )->notify_in )
#define requester_notify_out( self ) ( ( self )->notify_out )

#define responder_socket( self ) ( ( self )->responder )
#define responder_pipe_socket( self ) ( ( self )->pipe )
#define responder_zmq_socket( self ) ( ( self )->output )
#define responder_id( self ) ( ( self )->own_id )
#define responder_port( self ) ( ( self )->port )
#define responder_expiry( self ) ( ( self )->expiry )
#define responder_service_frame( self ) ( ( self )->service_frame )
#define responder_notify_in( self ) ( ( self )->notify_in )
#define responder_notify_out( self ) ( ( self )->notify_out )

#define publisher_socket( self ) ( ( self )->publisher )
#define publisher_pipe_socket( self ) ( ( self )->pipe )
#define publisher_zmq_socket( self ) ( ( self )->output )
#define publisher_port( self ) ( ( self )->port )


#define subscriber_socket( self ) ( ( self )->subscriber )
#define subscriber_pipe_socket( self ) ( ( self )->pipe )
#define subscriber_zmq_socket( self ) ( ( self )->output )
#define subscriber_port( self ) ( ( self )->port )
#define subscriber_callbacks( self ) ( ( self )->callbacks )
#define subscriber_notify_in( self ) ( ( self )->notify_in )
#define subscriber_notify_out( self ) ( ( self )->notify_out )

#define notify_in( fd, user_data, info_type ) \
  do { \
    emirates_priv *priv = user_data; \
    char dummy; \
    read( priv->info_type->notify_in, &dummy, sizeof( dummy ) ); \
    assert( dummy == 'y' ); \
    poll_##info_type( priv ); \
  } while ( 0 )

#define notify_in_requester( fd, user_data ) \
  notify_in( ( fd ), ( user_data ), requester )

#define notify_in_responder( fd, user_data ) \
  notify_in( ( fd ), ( user_data ), responder )

#define notify_in_subscriber( fd, user_data ) \
  notify_in( ( fd ), ( user_data ), subscriber )

int publisher_init( emirates_priv *iface );
int subscriber_init( emirates_priv *iface );
int requester_init( emirates_priv *iface );
int responder_init( emirates_priv *iface );
int check_status( void *socket );
void send_ok_status( void *socket );
void send_ng_status( void *socket );

uint32_t send_request( const char *service, emirates_priv *priv );
void service_request( const char *service, emirates_priv *priv, request_callback *callback );
void service_reply( const char *service, emirates_priv *priv, reply_callback *callback );
void subscribe_to_service( const char *service, subscriber_info *self, const char **sub_schema_names, subscriber_callback *user_callback );
zmsg_t *one_or_more_msg( void *socket );
int get_time_left( int64_t expiry );


CLOSE_EXTERN
#endif // EMIRATES_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
