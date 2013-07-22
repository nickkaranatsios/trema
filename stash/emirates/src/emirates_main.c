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


#include "emirates.h"


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


static nfds_t
count_rfds( const emirates_priv *priv ) {
   nfds_t nr_fds = 0;

  if ( subscriber_notify_in( priv->subscriber ) ) {
    nr_fds++;
  }
  if ( requester_notify_in( priv->requester ) ) {
    nr_fds++;
  }
  if ( responder_notify_in( priv->responder ) ) {
    nr_fds++;
  }

  return nr_fds;
}


static void
set_rfds( const emirates_priv *priv, struct pollfd rfds[] ) {
  if ( responder_notify_in( priv->responder ) ) {
    rfds[ 0 ].fd = responder_notify_in( priv->responder );
    rfds[ 0 ].events = POLLIN;
  }
  if ( requester_notify_in( priv->requester ) ) {
    rfds[ 1 ].fd = requester_notify_in( priv->requester );
    rfds[ 1 ].events = POLLIN;
  }
  if ( subscriber_notify_in( priv->subscriber ) ) {
    rfds[ 2 ].fd = subscriber_notify_in( priv->subscriber );
    rfds[ 2 ].events = POLLIN;
  }
}


static void
rfds_check( const emirates_priv *priv, struct pollfd rfds[] ) {
  if ( responder_notify_in( priv->responder ) ) {
    if ( rfds[ 0 ].revents & POLLIN ) {
    }
  }
  if ( requester_notify_in( priv->requester ) ) {
    if ( rfds[ 1 ].revents & POLLIN ) {
    }
  }
  if ( subscriber_notify_in( priv->subscriber ) ) {
    if ( rfds[ 1 ].revents & POLLIN ) {
    }
  }
}


int
emirates_loop( emirates_iface *iface ) {
  nfds_t nr_fds = count_rfds( iface->priv );
  struct pollfd rfds[ nr_fds ];
  int msec = -1;
  
  set_rfds( iface->priv, rfds );
  int res;
  while ( !zctx_interrupted ) {
    if ( ( res = poll( rfds, nr_fds, msec ) ) == -1 ) {
      res = EIO;
      break;
    }
    if ( res == 0 ) {
      res = ETIMEDOUT;
      break;
    }
    else {
      rfds_check( iface->priv, rfds );
    }
  }

  return EINVAL;
}


emirates_iface *
emirates_initialize( void ) {
  emirates_iface *iface = ( emirates_iface * ) zmalloc( sizeof( emirates_iface ) );
  emirates_priv *priv =  ( emirates_priv * ) zmalloc( sizeof( emirates_priv ) );
  memset( priv, 0, sizeof( emirates_priv ) );
  iface->priv = priv;

  zctx_t *ctx;
  ctx = zctx_new();
  if ( ctx == NULL ) {
    return NULL;
  }
  srandom( ( uint32_t ) time( NULL ) );
  priv->ctx = ctx;
  if ( publisher_init( priv ) ) {
    return NULL;
  }
  iface->get_publisher = publisher;
  if ( subscriber_init( priv ) ) {
    return NULL;
  }
  if ( responder_init( priv ) ) {
    return NULL;
  }
  iface->set_service_request = service_request;
  if ( requester_init( priv ) ) {
    return NULL;
  }
  iface->get_requester = requester;

  return iface;
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
