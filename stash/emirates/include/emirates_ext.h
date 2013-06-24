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


#ifndef EMIRATES_EXT_H
#define EMIRATES_EXT_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "emirates_priv.h"


typedef struct emirates_iface {
  emirates_priv *priv;
  zctx_t ( *get_ctx ) ( void ); 
  void ( *get_publisher ) ( void );
} emirates_iface;


emirates_iface *emirates_initialize( void );
void emirates_finalize( emirates_iface **iface );
void subscribe_service_profile( emirates_iface *iface, subscriber_callback *user_callback );
void subscribe_user_profile( emirates_iface *iface, subscriber_callback *user_callback );
void publish_service_profile( emirates_iface *iface );


CLOSE_EXTERN
#endif // EMIRATES_EXT_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
