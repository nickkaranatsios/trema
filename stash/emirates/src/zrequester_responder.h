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


#ifndef ZREQUESTER_RESPONDER_H
#define ZREQUESTER_RESPONDER_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif




typedef struct service_route {
  char service[ SERVICE_MAX ];
  char identity[ IDENTITY_MAX ];
  int64_t join_at;
  bool active;
} service_route;


typedef struct proxy {
  zctx_t *ctx;
  void *frontend;
  void *backend;
  zloop_t *loop;
  zlist_t *requests;
  zlist_t *replies;
  zlist_t *clients;
  zlist_t *workers;
} proxy;


CLOSE_EXTERN
#endif // ZREQUESTER_RESPONDER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
