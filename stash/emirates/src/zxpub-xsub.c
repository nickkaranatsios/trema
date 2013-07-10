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


#define check_return( retval, call ) \
  do { \
    int  __rc; \
    __rc = call; \
    if ( __rc != 0 ) { \
      return retval; \
    } \
  } while ( 0 )


int
main( void ) {
  int rc;
  zctx_t *context = zctx_new();

  // this is the public endpoint for publishers
  void *frontend = zsocket_new( context, ZMQ_XSUB );
  zsocket_bind( frontend, "tcp://*:%zu", PUB_BASE_PORT );

  // this is the public endpoint for subscribers
  void *backend = zsocket_new( context, ZMQ_XPUB );
  zsocket_bind( backend, "tcp://*:%zu", SUB_BASE_PORT );

  // Run the proxy until the user interrupts us
  zmq_proxy( frontend, backend, NULL );

  zctx_destroy( &context );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
