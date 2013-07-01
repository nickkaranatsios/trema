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


int
responder_init( emirates_priv *priv ) {
  return 0;
}


#ifdef OLD

static void
responder_thread( void *args, zctx_t *ctx, void *pipe ) {
  void *responder = zsocket_new( ctx, ZMQ_REP );
  int rc = zsocket_connect( responder, "tcp://localhost:5560" );
  if ( rc ) {
    printf( "Failed to connect responder errno %d:%s\n", errno, zmq_strerror( errno ) );
  }
  while ( true ) {
    char *string = zstr_recv( responder );
    if ( string ) {
      printf( "Received request [ %s ]\n", string );
      zstr_send( responder, string );
      free( string );
    }
    // to think how to pass the callback this thread from main thread.
  }
}


void *
responder_init( zctx_t *ctx ) {
  return zthread_fork( ctx, responder_thread, NULL );
}


int
main( int argc, char **argv ) {
  zctx_t *ctx;
  ctx = zctx_new();
  void *responder = responder_init( ctx );
  if ( responder != NULL ) {
    while ( !zctx_interrupted ) {
      zclock_sleep( 1000 );
    }
  }

  return 0;
}
#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
