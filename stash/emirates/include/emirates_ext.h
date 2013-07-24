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


#include "jedex_iface.h"

#define PUBLISHER   ( 1 << 0 )
#define SUBSCRIBER  ( 1 << 1 )
#define REQUESTER   ( 1 << 2 )
#define RESPONDER   ( 1 << 3 )

#define ENTITY_SET( flag, entity ) ( ( flag ) | ( entity ) )
#define ENTITY_TST( flag, entity ) ( ( flag ) & ( entity ) )
#define ENTITY_CLR( flag, entity ) ( ( flag ) & ~( entity ) )


typedef struct emirates_iface emirates_iface;
typedef void ( *request_handler ) ( void *user_data );
typedef void ( *reply_handler ) ( void *user_data );
typedef void ( *subscription_handler ) ( void *user_data );
typedef void ( *timer_handler ) ( void *user_data );


struct emirates_iface {
  void *priv;
  void *( *get_publisher ) ( emirates_iface *iface );
  void *( *get_requester ) ( emirates_iface *iface );
  void ( *set_service_request ) ( emirates_iface *iface, const char *service, request_handler request_callback );
  void ( *set_service_reply ) ( emirates_iface *iface, const char *service, reply_handler reply_callback );
  // this call sets a callback handler to be called when a subscription received
  void ( *set_subscription ) ( emirates_iface *iface, const char *service, const char **schema_names, subscription_handler subscription_callback );
  void ( *publish ) ( emirates_iface *iface, const char *service, jedex_parcel *parcel );
  uint32_t ( *send_request ) ( emirates_iface *iface, const char *service, jedex_parcel *parcel );
  void ( *set_periodic_timer ) ( emirates_iface *iface, int msecs, timer_handler timer_callback, void *user_data );
};


emirates_iface *emirates_initialize( void );
emirates_iface *emirates_initialize_only( const int flag );
int emirates_loop( emirates_iface *iface );
void emirates_finalize( emirates_iface **iface );
void publish( emirates_iface *iface, const char *service_name, jedex_parcel *parcel );
void service_request( emirates_iface *iface, const char *service, request_handler callback );
void service_reply( emirates_iface *iface, const char *service, reply_handler callback );
void subscription( emirates_iface *iface, const char *service, const char **schema_names, subscription_handler subscription_callback );
uint32_t send_request( emirates_iface *iface, const char *service, jedex_parcel *parcel );
void set_periodic_timer( emirates_iface *iface, int msecs, timer_handler timer_callback, void *user_data );


// example ONLY
#define quote( name ) #name
#define str( name ) quote( name )

#define subscribe_service_profile( iface, sub_schema_names, user_callback ) \
  subscribe_to_service( str( service_profile ), ( priv( iface ) )->subscriber, sub_schema_names, user_callback )

#define subscribe_user_profile( iface, sub_schema_names, user_callback ) \
  subscribe_to_service( str( user_profile ), ( priv ( iface ) )->subscriber, sub_schema_names, user_callback )

#define set_menu_reply( iface, callback ) \
  service_reply( str( menu ), priv( iface ), callback )

#define set_profile_reply( iface, callback ) \
  service_reply( str( profile ), priv( iface ), callback )

#define send_menu_request( priv ) \
  send_request( str( menu ), priv )

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
