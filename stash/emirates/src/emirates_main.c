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
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  uint32_t *port = args;

  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( pub, "tcp://localhost:%u", port );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XSUB %d\n", rc );
    return;
  }

  if ( pub != NULL ) {
    while ( true ) {
      char *string = zstr_recv( pipe );
      if ( string == NULL ) {
        break;
      }
      // printf( "publish string %s\n", string );
      if ( zstr_send( pub, string ) == -1 ) {
        break; //  Interrupted
      }
      free( string );
    }
  }
}


static void
subscriber_thread( void *args, zctx_t *ctx, void *pipe ) {
  uint32_t *port = args;
  int rc;

  void *sub = zsocket_new( ctx, ZMQ_SUB );
  rc = zsocket_connect( sub, "tcp://localhost:%u", port );
  zsockopt_set_subscribe( sub, "" );


  uint64_t sub_count = 0;
  while ( true ) {
    char *string = zstr_recv( sub );
    if ( string == NULL ) {
      break; //  Interrupted
    }
    rc = zstr_send( pipe, string );
    if ( rc != 0 ) {
      log_err( "Failed to send subscribed topic to parent thread %s", string );
    }
    free( string );
  }
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
  iface->priv->pub_port = PUB_BASE_PORT;
  iface->priv->pub = zthread_fork( iface->priv->ctx, publisher_thread, &iface->priv->pub_port );
  
  iface->priv->sub_port = SUB_BASE_PORT;
  iface->priv->sub = zthread_fork( iface->priv->ctx, subscriber_thread, &iface->priv->sub_port );

  return iface;
}


void
emirates_finalize( emirates_iface **iface ) {
  assert( iface );

  if ( *iface ) {
    assert( ( *iface )->priv );
    
    zsocket_destroy( ( *iface )->priv->ctx, ( *iface )->priv->pub );
    zsocket_destroy( ( *iface )->priv->ctx, ( *iface )->priv->sub );
    zctx_destroy( &( *iface )->priv->ctx );
    free( iface );
    *iface = NULL;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
