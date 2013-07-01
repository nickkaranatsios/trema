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
publish_service( const char *service, emirates_iface *iface, jedex_parcel *parcel ) {
  emirates_priv *priv = iface->priv;

  for ( list_element *e = parcel->values_list->head; e != NULL; e = e->next ) {
    jedex_value *item = e->data;
    char *json;
    jedex_value_to_json( item, 1, &json );
    if ( json ) {
      zmsg_t *msg = zmsg_new();
      zmsg_addmem( msg, service, strlen( service ) );
      zmsg_addmem( msg, json, strlen( json ) );
      zmsg_send( &msg, priv->pub );
      zmsg_destroy( &msg );
    }
  }

  return;
}


static int
publish_output( zloop_t *loop, zmq_pollitem_t *poller, void *arg ) {
  void *pub = arg;

  zmsg_t *msg = zmsg_recv( poller->socket );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  assert( nr_frames == 2 );

  zframe_t *service_frame = zmsg_first( msg );
  zframe_t *service_data = zmsg_next( msg );
  if ( service_data != NULL ) {
    zmsg_send( &msg, pub );
  }
  else {
    return EINVAL;
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
start_publishing( void *pipe, void *pub ) {
  zloop_t *loop = zloop_new();
  zmq_pollitem_t poller = { pipe, 0, ZMQ_POLLIN, 0 };
  zloop_poller( loop, &poller, publish_output, pub );
  zloop_start( loop );
}


void
publish_service_profile( emirates_iface *iface, jedex_parcel *parcel ) {
  publish_service( "service_profile", iface, parcel );
}


static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  uint32_t *port = args;

  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( pub, "tcp://localhost:%u", *port );
  if ( rc < 0 ) {
    log_err( "Failed to connect to XSUB %d", rc );
    send_ng_status( pipe );
    return;
  }

  send_ok_status( pipe );
  start_publishing( pipe, pub );
}


int
publisher_init( emirates_priv *priv ) {
  priv->pub = zthread_fork( priv->ctx, publisher_thread, &priv->pub_port );
  if ( check_status( priv->pub ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


#ifdef OLD
static void 
publisher_thread( void *args, zctx_t *ctx, void *pipe ) {
  int rc;
  void *pub = zsocket_new( ctx, ZMQ_PUB );
  rc = zsocket_connect( pub, "tcp://localhost:6000" );
  if ( rc < 0 ) {
    printf( "Failed to connect to XSUB %d\n", rc );
    return;
  }

  uint64_t pub_count = 0;
  if ( pub != NULL ) {
    while ( true ) {
      char *string = zstr_recv( pipe );
      if ( string == NULL ) {
        break;
      }
      // printf( "publish string %s\n", string );
      if ( zstr_send( pub, string ) == -1 ) {
        break;              //  Interrupted
      }
      pub_count++;
      free( string );
    }
  }
  printf( "publish( %" PRIu64 ")\n" ,pub_count );
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new(); 

  void *pub = zthread_fork( ctx, publisher_thread, NULL );

  uint64_t end = 2 * 60 * 1000 + zclock_time();
  while ( !zctx_interrupted ) {
    if ( zclock_time() >= end ) {
      break;
    }
    char string[ 10 ];

    sprintf( string, "%c-%05d", randof( 10 ) + 'A', randof( 100000 ) );
    zstr_send( pub, string );
    // zclock_sleep( 1000 );
  }
  
  zctx_destroy( &ctx );

  return 0; 
}

#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
