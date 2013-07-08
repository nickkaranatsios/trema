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
requester_output( requester_info *self ) {
  zmsg_t *msg = one_or_more_msg( requester_pipe_socket( self ) );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester_output %zu\n", nr_frames );

    disable_output( requester_output_ctrl( self ) );
    zmsg_send( &msg, requester_zmq_socket( self ) );
    return 0;
  }

  return EINVAL;
}


static int
requester_input( requester_info *self ) {
  zmsg_t *msg = one_or_more_msg( requester_zmq_socket( self ) );

  if ( msg ) {
    size_t nr_frames = zmsg_size( msg );
    printf( "requester_input %zu\n", nr_frames );

    enable_output( requester_output_ctrl( self ) );
    zmsg_send( &msg, requester_pipe_socket( self ) );
    return 0;
  }

  return EINVAL;
}


static void
start_requester( requester_info *self ) {
  requester_output_ctrl( self ) = 0;
  int64_t request_expiry = 0;

  enable_output( self->output_ctrl );
  zmq_pollitem_t items[ 2 ];
  while ( 1 ) {
    int poll_size;

    items[ 0 ].events = items[ 1 ].events = ZMQ_POLLIN;
    items[ 0 ].fd = items[ 1 ].fd = 0;
    items[ 0 ].revents = items[ 1 ].revents = 0;
    if ( use_output( requester_output_ctrl( self ) ) ) {
      items[ 0 ].socket = requester_pipe_socket( self );
      items[ 1 ].socket = requester_zmq_socket( self );
      poll_size = 2;
    }
    else {
      items[ 0 ].socket = requester_zmq_socket( self );
      poll_size = 1;
    }

    int rc = zmq_poll( items, poll_size, get_time_left( request_expiry ) );
    if ( rc == -1 ) {
      break;
    }
    if ( use_output( requester_output_ctrl( self ) ) ) {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        request_expiry = zclock_time() + REQUEST_HEARTBEAT;
        request_expiry = 0;
        rc = requester_output( self );
      }
      if ( items[ 1 ].revents & ZMQ_POLLIN ) {
        request_expiry = 0;
        rc = requester_input( self );
      }
    }
    else {
      if ( items[ 0 ].revents & ZMQ_POLLIN ) {
        rc = requester_input( self );
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
  requester_info *self = args;
  requester_pipe_socket( self ) = pipe;

  uint32_t port = requester_port( self );

  requester_zmq_socket( self ) = zsocket_new( ctx, ZMQ_REQ );

  requester_id( self ) = ( char * ) zmalloc( sizeof( char ) * REQUESTER_ID_SIZE );
  snprintf( requester_id( self ), REQUESTER_ID_SIZE, "%lld", zclock_time() );
#ifdef TEST
  snprintf( requester_id( self ), REQUESTER_ID_SIZE, "%s", "client1" );
#endif
  zsocket_set_identity( requester_zmq_socket( self ), requester_id( self ) );

  int rc = zsocket_connect( requester_zmq_socket( self ), "tcp://localhost:%u", port );
  if ( rc ) {
    log_err( "Failed to connect requester using port %u", port );
    send_ng_status( requester_pipe_socket( self ) );
    return;
  }

  send_ok_status( requester_pipe_socket( self ) );
  start_requester( self );
}


int
requester_init( emirates_priv *priv ) {
  priv->requester = ( requester_info * ) zmalloc( sizeof( requester_info ) );
  requester_port( priv->requester ) = REQUESTER_BASE_PORT;
  requester_socket( priv->requester ) = zthread_fork( priv->ctx, requester_thread, priv->requester );
  if ( check_status( requester_socket( priv->requester ) ) ) {
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
