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


#define PUB_BASE_PORT 6000
#define SUB_BASE_PORT 6001

typedef void ( subscriber_callback )( void *args );

typedef struct sub_callback {
  subscriber_callback *callback;
  const char *service;
} sub_callback;


typedef struct emirates_priv {
  zctx_t *ctx;
  void *pub;
  void *pub_raw;
  void *sub;
  void *sub_raw;
  uint32_t pub_port; // publisher's assigned port
  uint32_t sub_port; // subscriber's assigned port
  zlist_t *sub_callbacks;
} emirates_priv;


CLOSE_EXTERN
#endif // EMIRATES_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
