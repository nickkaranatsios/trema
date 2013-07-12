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
#include "jedex_iface.h"


typedef struct emirates_iface emirates_iface;


struct emirates_iface {
  void *priv;
  void *( *get_publisher ) ( emirates_iface * );
  void *( *get_requester ) ( emirates_iface * );
};


emirates_iface *emirates_initialize( void );
void emirates_finalize( emirates_iface **iface );
void publish_service_profile( emirates_iface *iface, jedex_parcel *parcel );

// example ONLY
#define quote( name ) #name
#define str( name ) quote( name )

#define subscribe_service_profile( iface, sub_schema_names, user_callback ) \
  subscribe_to_service( str( service_profile ), ( priv( iface ) )->subscriber, sub_schema_names, user_callback )

#define subscribe_user_profile( iface, sub_schema_names, user_callback ) \
  subscribe_to_service( str( user_profile ), ( priv ( iface ) )->subscriber, sub_schema_names, user_callback )

#define set_menu_request( iface, callback ) \
  service_request( str( menu ), priv( iface ), callback )

#define set_menu_reply( iface, callback ) \
  service_reply( str( menu ), priv( iface ), callback )

#define send_menu_request( priv ) \
  send_request( str( menu ), priv )

#define set_profile_request( iface, callback ) \
  service_request( str( profile ), priv( iface ), callback )

#define set_profile_reply( iface, callback ) \
  service_reply( str( profile ), priv( iface ), callback )

#define send_profile_request( priv ) \
  send_request( str( profile ), priv )

void set_ready( emirates_iface *iface );


CLOSE_EXTERN
#endif // EMIRATES_EXT_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
