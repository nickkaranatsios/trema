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
#define RESPONDER_BASE_PORT 6200

#define SUBSCRIPTION_MSG "SET_SUBSCRIPTION"
#define ADD_SERVICE_REQUEST_MSG "ADD_SERVICE_REQUEST"
#define ADD_SERVICE_REPLY_MSG "ADD_SERVICE_REPLY"
#define EXIT_MSG "EXIT"

typedef void ( subscriber_callback )( void *args );
typedef void ( request_callback )( void *args );
typedef void ( responder_callback )( void *args );
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


typedef struct resp_callback {
  responder_callback *callback;
  const char *service;
  const char *requester_id;
} resp_callback;


typedef struct emirates_priv {
  zctx_t *ctx;
  void *pub;
  void *sub;
  uint32_t pub_port; // publisher's assigned port
  uint32_t sub_port; // subscriber's assigned port
  zlist_t *sub_callbacks;
  poll_handler *sub_handler;
  void *requester;
  void *replier;
  uint32_t requester_port;
  uint32_t responder_port;
  zlist_t *req_callbacks;
  zlist_t *resp_callbacks;
} emirates_priv;


int publisher_init( emirates_priv *iface );
int subscriber_init( emirates_priv *iface );
int requester_init( emirates_priv *iface );
int responder_init( emirates_priv *iface );
int check_status( void *socket );
void send_ok_status( void *socket );
void send_ng_status( void *socket );


CLOSE_EXTERN
#endif // EMIRATES_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
