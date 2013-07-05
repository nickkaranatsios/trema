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


static int
requester_output( void *pipe, void *requester, int *output ) {
  zmsg_t *msg = one_or_more_msg( pipe );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester_output %zu\n", nr_frames );

    disable_output( *output );
    zmsg_send( &msg, requester );
    return 0;
  }

  return EINVAL;
}


static int
requester_input( void *requester, void *pipe, int *output ) {
  zmsg_t *msg = one_or_more_msg( requester );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester_input %zu\n", nr_frames );

    enable_output( *output );
    zmsg_send( &msg, pipe );
    return 0;
  }

  return EINVAL;
}


static void
start_requester( void *pipe, void *requester ) {
  int output = 0;
  int64_t request_expiry = 0;
  long time_left;

  enable_output( output );
  zmq_pollitem_t items[ 2 ];
  while ( 1 ) {
    int poll_size;

    items[ 0 ].events = items[ 1 ].events = ZMQ_POLLIN;
    items[ 0 ].fd = items[ 1 ].fd = 0;
    items[ 0 ].revents = items[ 1 ].revents = 0;
    if ( use_output( output ) ) {
      items[ 0 ].socket = pipe;
      items[ 1 ].socket = requester;
      poll_size = 2;
    }
    else {
      items[ 0 ].socket = requester;
      poll_size = 1;
    }

    if ( !request_expiry ) {
      time_left = -1;
    }
    else {
      time_left = ( long ) ( request_expiry - zclock_time() );
      if ( time_left < 0 ) {
        time_left = 0;
      }
      time_left *= ZMQ_POLL_MSEC;
    }
    int rc = zmq_poll( items, poll_size, time_left );
    if ( rc == -1 ) {
      break;
    }
    if ( use_output( output ) ) {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        request_expiry = zclock_time() + REQUEST_HEARTBEAT;
        rc = requester_output( pipe, requester, &output );
      }
      if ( items[ 1 ].revents & ZMQ_POLLIN ) {
        request_expiry = 0;
        rc = requester_input( requester, pipe, &output );
      }
    }
    else {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        rc = requester_input( requester, pipe, &output );
      }
    }
    if ( rc == -1 ) {
      break;
    }
    if ( request_expiry ) { 
      if ( zclock_time() >= request_expiry ) {
        printf( "request timeout do something about\n" );
      }
      request_expiry = 0;
    }
  }
  printf( "out of requester\n" );
}


static void
requester_thread( void *args, zctx_t *ctx, void *pipe ) {
  emirates_priv *priv = args;
  uint32_t port = priv->requester_port;

  void *requester = zsocket_new( ctx, ZMQ_REQ );

  priv->requester_id = ( char * ) zmalloc( sizeof( char ) * REQUESTER_ID_SIZE );
  snprintf( priv->requester_id, REQUESTER_ID_SIZE, "%lld", zclock_time() );
#ifdef TEST
  snprintf( priv->requester_id, REQUESTER_ID_SIZE, "%s", "client1" );
#endif
  zsocket_set_identity( requester, priv->requester_id );

  int rc = zsocket_connect( requester, "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( pipe );
    return;
  }

  send_ok_status( pipe );
  start_requester( pipe, requester );
}


int
requester_init( emirates_priv *priv ) {
  priv->requester_port = REQUESTER_BASE_PORT;
  priv->requester = zthread_fork( priv->ctx, requester_thread, priv );
  if ( check_status( priv->requester ) ) {
    zctx_destroy( &priv->ctx );
    return EINVAL;
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
