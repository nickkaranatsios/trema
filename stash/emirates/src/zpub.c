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
publish_service( const char *service, publisher_info *self, jedex_parcel *parcel ) {
  for ( list_element_safe *e = parcel->values_list->head; e != NULL; e = e->next ) {
    jedex_value *item = e->data;
    char *json;
    jedex_value_to_json( item, true, &json );
    if ( json ) {
      zmsg_t *msg = zmsg_new();
      zmsg_addmem( msg, service, strlen( service ) );
      zmsg_addmem( msg, json, strlen( json ) );
      zmsg_send( &msg, publisher_socket( self ) );
      zmsg_destroy( &msg );
    }
  }

  return;
}


static int
publish_output( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  publisher_info *self = arg;

  zmsg_t *msg = one_or_more_msg( poller->socket );

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  zframe_t *service_data = zmsg_next( msg );
  if ( service_data != NULL ) {
    zmsg_send( &msg, publisher_zmq_socket( self ) );
  }
  else {
    return EINVAL;
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
start_publishing( publisher_info *self ) {
  zloop_t *loop = zloop_new();
  zmq_pollitem_t poller = { publisher_pipe_socket( self ), 0, ZMQ_POLLIN, 0 };
  zloop_poller( loop, &poller, publish_output, self );
  zloop_start( loop );
}


void
publish_service_profile( emirates_iface *iface, jedex_parcel *parcel ) {
  publish_service( "service_profile", ( priv( iface ) )->publisher, parcel );
}


static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  publisher_info *self = args;
  publisher_pipe_socket( self ) = pipe;
  int rc;

  publisher_zmq_socket( self ) = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( publisher_zmq_socket( self ), "tcp://localhost:%zu", publisher_port( self ) );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XSUB %d", rc );
    send_ng_status( publisher_pipe_socket( self ) );
    return;
  }

  send_ok_status( publisher_pipe_socket( self ) );
  start_publishing( self );
}


void *
get_publisher_socket( emirates_priv *priv ) {
  return publisher_socket( priv->publisher );
}


int
publisher_init( emirates_priv *priv ) {
  priv->publisher = ( publisher_info * ) zmalloc( sizeof( publisher_info ) );
  memset( priv->publisher, 0, sizeof( publisher_info ) );
  publisher_port( priv->publisher ) = PUB_BASE_PORT;

  publisher_socket( priv->publisher ) = zthread_fork( priv->ctx, publisher_thread, priv->publisher );
  if ( check_status( publisher_socket( priv->publisher ) ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


void
publisher_finalize( emirates_priv **priv ) {
  emirates_priv *priv_p = *priv;
  if ( priv_p->publisher ) {
    free( priv_p->publisher );
    priv_p->publisher = NULL;
  }
}

 
/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
