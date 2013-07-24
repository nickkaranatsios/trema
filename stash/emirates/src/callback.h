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


#ifndef CALLBACK_H
#define CALLBACK_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "emirates_ext.h"


typedef struct callback_key {
  const char *service;
} callback_key;


typedef struct request_callback {
  callback_key key;
  const char *responder_id;
  request_handler callback;
} request_callback;


typedef struct reply_callback {
  callback_key key;
  const char *requester_id;
  reply_handler callback;
} reply_callback;


typedef struct subscriber_callback {
  callback_key key;
  void **schemas;
  subscription_handler callback;
} subscriber_callback;


typedef struct timer_callback {
  int msecs;
  timer_handler callback;
  void *user_data;
} timer_callback;


void *lookup_callback( zlist_t *callbacks, const char *service );


CLOSE_EXTERN
#endif // CALLBACK_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
