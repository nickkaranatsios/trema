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
#define IDENTITY_MAX 64
#define SERVICE_MAX 64 + IDENTITY_MAX

#define ENTITY_SET( flag, entity ) ( ( flag ) | ( entity ) )
#define ENTITY_TST( flag, entity ) ( ( flag ) & ( entity ) )
#define ENTITY_CLR( flag, entity ) ( ( flag ) & ~( entity ) )


typedef struct emirates_iface emirates_iface;
/*
 * This is what a responder(worker) would receive to handle a request.
 * The perculiar parameter client_id is the requester_id. Most processes would
 * ignore this parameter but it is needed by db_mapper to effectively process
 * certain kind of messages.
 */
typedef void ( *request_handler ) ( jedex_value *val,
                                    const char *json,
                                    const char *client_id,
                                    void *user_data );
/*
 * This is what a requester(client) would receive after sending a request.
 */
typedef void ( *reply_handler ) ( const uint32_t tx_id,
                                  jedex_value *val,
                                  const char *json,
                                  void *user_data );
/*
 * This is what a subscriber would receive if subscribed to a topic.
 */
typedef void ( *subscription_handler ) ( jedex_value *val,
                                         const char *json,
                                         void *user_data );
typedef void ( *timer_handler ) ( void *user_data );


struct emirates_iface {
  void *priv;
  void *( *get_publisher ) ( emirates_iface *iface );
  void *( *get_requester ) ( emirates_iface *iface );
  /*
   * A responder(worker) indicates to emirates library that it can service
   * a request. The schema parameter is a pointer to a schema object that
   * would be used to parse the json data in order to create a jedex_value
   * object.
   */
  void ( *set_service_request ) ( emirates_iface *iface, 
                                  const char *service,
                                  jedex_schema *schema,
                                  void *user_data,
                                  request_handler request_callback );

  /*
   * use this call to set for example the request timeout timer for the service
   */
  void ( *set_request_expiry ) ( emirates_iface *iface,
                                 const char *service,
                                 int32_t msecs );
                                 
  void ( *set_service_reply ) ( emirates_iface *iface,
                                const char *service,
                                void *user_data,
                                reply_handler reply_callback );
  // this call sets a callback handler to be called when a subscription received
  void ( *set_subscription ) ( emirates_iface *iface,
                               const char *service,
                               jedex_schema **schemas,
                               void *user_data,
                               subscription_handler subscription_callback );
  /*
   * The parcel parameter is used to be able to set multiple objects to be
   * published at once. But the jedex_value object can hold for example an
   * array of same kind of objects so we don't expect applications to have
   * a need for it. Anyway at the moment is supported.
   */
  void ( *publish ) ( emirates_iface *iface,
                      const char *service,
                      jedex_parcel *parcel );

  void ( *publish_value ) ( emirates_iface *iface,
                            const char *service,
                            jedex_value *value );
  /*
   * This sends a request for a service that contains a jedex_value. The
   * reply_schema parameter is the schema that would be used to parse 
   * the expected reply with this request.
   */
  uint32_t ( *send_request ) ( emirates_iface *iface,
                               const char *service,
                               jedex_value *value,
                               jedex_schema *reply_schema );
  /*
   * The following call mainly used by db_mapper to transmit replies.
   * Most other applications should use the send_reply call.
   */
  void ( *send_reply_raw ) ( emirates_iface *iface,
                             const char *service,
                             const char *json );

  void ( *send_reply ) ( emirates_iface *iface,
                         const char *service,
                         jedex_value *value );

  void ( *set_periodic_timer ) ( emirates_iface *iface,
                                 int msecs,
                                 timer_handler timer_callback,
                                 void *user_data );
};


emirates_iface *emirates_initialize( void );

emirates_iface *emirates_initialize_only( const int flag );

int emirates_loop( emirates_iface *iface );

void emirates_finalize( emirates_iface **iface );

void publish( emirates_iface *iface,
              const char *service_name,
              jedex_parcel *parcel );

void publish_value( emirates_iface *iface,
                    const char *service_name,
                    jedex_value *value );

void service_request( emirates_iface *iface,
                      const char *service,
                      jedex_schema *schema,
                      void *user_data,
                      request_handler callback );

void request_expiry( emirates_iface *iface,
                     const char *service,
                     int32_t msecs );

void service_reply( emirates_iface *iface,
                    const char *service,
                    void *user_data,
                    reply_handler callback );

void subscription( emirates_iface *iface,
                   const char *service,
                   jedex_schema **schemas,
                   void *user_data,
                   subscription_handler subscription_callback );

uint32_t send_request( emirates_iface *iface,
                       const char *service,
                       jedex_value *value,
                       jedex_schema *reply_schema );

void send_reply_raw( emirates_iface *iface,
                     const char *service,
                     const char *json );

void send_reply( emirates_iface *iface,
                 const char *service,
                 jedex_value *value );

void set_periodic_timer( emirates_iface *iface,
                         int msecs,
                         timer_handler timer_callback,
                         void *user_data );

void set_ready( emirates_iface *iface );


CLOSE_EXTERN
#endif // EMIRATES_EXT_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
