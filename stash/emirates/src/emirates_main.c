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
  zmsg_destroy( &msg );
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



emirates_iface *
emirates_initialize( void ) {
  emirates_iface *iface = ( emirates_iface * ) zmalloc( sizeof( emirates_iface ) );
  iface->priv = ( emirates_priv * ) zmalloc( sizeof( emirates_priv ) );

  zctx_t *ctx;
  ctx = zctx_new();
  if ( ctx == NULL ) {
    return NULL;
  }
  iface->priv->ctx = ctx;
#ifdef TEST
  if ( publisher_init( iface->priv ) || subscriber_init( iface->priv ) ||
    responder_init( iface->priv ) || requester_init( iface->priv ) ) {
     return NULL;
  }
#endif
  srandom( ( uint32_t ) time( NULL ) );
  if ( responder_init( iface->priv ) ) {
    return NULL;
  }
  if ( requester_init( iface->priv ) ) {
    return NULL;
  }

  return iface;
}


void
emirates_finalize( emirates_iface **iface ) {
  assert( iface );

  if ( *iface ) {
    assert( ( *iface )->priv );
    
    zctx_destroy( &( *iface )->priv->ctx );
    *iface = NULL;
  }
}




static char *
any_value_to_json( jedex_value *val ) {
  char *json;

  jedex_value_to_json( val, 1, &json );

  return json;
}




/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
