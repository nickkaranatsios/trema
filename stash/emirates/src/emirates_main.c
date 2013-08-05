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


#include "emirates_priv.h"
#include "callback.h"


static void
send_status( void *socket, const char *status ) {
  zmsg_t *msg = zmsg_new();
  assert( msg );
  zmsg_addstr( msg, STATUS_MSG );
  zmsg_addstr( msg, status );
  zmsg_send( &msg, socket );
}


void
send_ng_status( void *socket ) {
  send_status( socket, STATUS_NG );
}


void
send_ok_status( void *socket ) {
  send_status( socket, STATUS_OK );
}
  

int
check_status( void *socket ) {
  zmsg_t *status_msg = zmsg_recv( socket );
  
  if ( status_msg == NULL ) {
    return EINVAL;
  }
  size_t nr_frames = zmsg_size( status_msg );
  assert( nr_frames == 2 );
  bool status_ok = false;

  zframe_t *status_frame = zmsg_first( status_msg );
  if ( zframe_streq( status_frame, STATUS_MSG ) ) {
    zframe_t *what = zmsg_next( status_msg );
    if ( zframe_streq( what, STATUS_OK ) ) {
      status_ok = true;
    }
  }
  zmsg_destroy( &status_msg );

  return status_ok == false ? EINVAL : 0;
}


static void *
publisher( emirates_iface *iface ) {
  return get_publisher_socket( priv( iface ) );
}


static void *
requester( emirates_iface *iface ) {
  return get_requester_socket( priv( iface ) );
}


void
set_periodic_timer( emirates_iface *iface, int msecs, timer_handler callback, void *user_data ) {
  emirates_priv *priv = priv( iface );

  timer_callback *timer = priv->timer;
  assert( timer );

  timer->msecs = msecs;
  timer->callback = callback;
  timer->user_data = user_data;
}


static nfds_t
count_rfds( const emirates_priv *priv ) {
   nfds_t nr_fds = 0;

  if ( priv->subscriber && subscriber_notify_in( priv->subscriber ) ) {
    nr_fds++;
  }
  if ( priv->requester && requester_notify_in( priv->requester ) ) {
    nr_fds++;
  }
  if ( priv->responder && responder_notify_in( priv->responder ) ) {
    nr_fds++;
  }

  return nr_fds;
}


static void
set_rfds( const emirates_priv *priv, struct pollfd rfds[] ) {
  int idx = 0;

  if ( priv->responder && responder_notify_in( priv->responder ) ) {
    rfds[ idx ].fd = responder_notify_in( priv->responder );
    rfds[ idx++ ].events = POLLIN;
  }
  if ( priv->requester && requester_notify_in( priv->requester ) ) {
    rfds[ idx ].fd = requester_notify_in( priv->requester );
    rfds[ idx++ ].events = POLLIN;
  }
  if ( priv->subscriber && subscriber_notify_in( priv->subscriber ) ) {
    rfds[ idx ].fd = subscriber_notify_in( priv->subscriber );
    rfds[ idx ].events = POLLIN;
  }
}


// TODO insert the read loops for each entity
static void
rfds_check( const emirates_priv *priv, struct pollfd rfds[] ) {
  int idx = 0;

  // the order of checking should be exactly as the order of setting
  if ( priv->responder && responder_notify_in( priv->responder ) ) {
    if ( rfds[ idx ].revents & POLLIN ) {
      responder_invoke( priv );
    }
  }
  if ( priv->requester && requester_notify_in( priv->requester ) ) {
    if ( rfds[ idx++ ].revents & POLLIN ) {
      requester_invoke( priv );
    }
  }
  if ( priv->subscriber && subscriber_notify_in( priv->subscriber ) ) {
    if ( rfds[ idx ].revents & POLLIN ) {
      subscriber_invoke( priv );
    }
  }
}


static emirates_iface *
init_emirates_iface( emirates_iface **iface ) {
  *iface = ( emirates_iface * ) zmalloc( sizeof( emirates_iface ) );
  emirates_priv *priv =  ( emirates_priv * ) zmalloc( sizeof( emirates_priv ) );
  memset( priv, 0, sizeof( emirates_priv ) );
  ( *iface )->priv = priv;

  zctx_t *ctx;
  ctx = zctx_new();
  if ( ctx == NULL ) {
    return NULL;
  }
  srandom( ( uint32_t ) time( NULL ) );
  priv->ctx = ctx;

  timer_callback *callback = ( timer_callback * ) zmalloc( sizeof( timer_callback ) );
  memset( callback, 0, sizeof( timer_callback ) );
  priv->timer = callback;
  ( *iface )->set_periodic_timer = set_periodic_timer;

  return *iface;
}


static emirates_iface *
initialize_entities( emirates_iface *iface, const int flag ) {
  if ( ENTITY_TST( flag, PUBLISHER ) ) {
    if ( publisher_init( iface->priv ) ) {
      return NULL;
    }
    iface->get_publisher = publisher;
    iface->publish = publish;
    iface->publish_value = publish_value;
  }
  if ( ENTITY_TST( flag, SUBSCRIBER ) ) {
    if ( subscriber_init( iface->priv ) ) {
      return NULL;
    }
    iface->set_subscription = subscription;
    iface->set_subscription_new = subscription_new;
  }
  if ( ENTITY_TST( flag, RESPONDER ) ) {
    if ( responder_init( iface->priv ) ) {
      return NULL;
    }
    iface->set_service_request = service_request;
    iface->send_reply_raw = send_reply_raw;
    iface->send_reply = send_reply;
  }
  if ( ENTITY_TST( flag, REQUESTER ) ) {
    if ( requester_init( iface->priv ) ) {
      return NULL;
    }
    iface->set_service_reply = service_reply;
    iface->send_request = send_request;
    iface->get_requester = requester;
  }

  return iface;
}


int
emirates_loop( emirates_iface *iface ) {
  nfds_t nr_fds = count_rfds( iface->priv );
  struct pollfd rfds[ nr_fds ];
  int msec = -1;
  emirates_priv *priv = iface->priv;
  timer_callback *timer = priv->timer;
  if ( timer->msecs ) {
    msec = timer->msecs;
  }
  
  set_rfds( iface->priv, rfds );
  int res = 0;
  while ( !zctx_interrupted ) {
    if ( ( res = poll( rfds, nr_fds, msec ) ) == -1 ) {
      res = EIO;
      break;
    }
    if ( res == 0 ) {
      if ( timer->callback ) {
        timer->callback( timer->user_data );
      }
    }
    else {
      rfds_check( iface->priv, rfds );
    }
  }

  return res;
}


emirates_iface *
emirates_initialize( void ) {
  emirates_iface *iface = NULL; 

  if ( ( iface = init_emirates_iface( &iface ) ) == NULL ) {
    return NULL;
  }
  int flag = ~0;

  // initialize all entities
  return initialize_entities( iface, flag );
}


emirates_iface *
emirates_initialize_only( const int flag ) {
  emirates_iface *iface = NULL;

  if ( ( iface = init_emirates_iface( &iface ) ) == NULL ) {
    return NULL;
  }
  // initialize only those entities set by the flag
  return initialize_entities( iface, flag );
}


void
emirates_finalize( emirates_iface **iface ) {
  assert( iface );

  if ( *iface ) {
    assert( ( *iface )->priv );
    
    zctx_destroy( &( priv( *iface ) )->ctx );
    emirates_priv *priv = priv( *iface );
    requester_finalize( &priv );
    responder_finalize( &priv );
    publisher_finalize( &priv );
    subscriber_finalize( &priv );
    *iface = NULL;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
