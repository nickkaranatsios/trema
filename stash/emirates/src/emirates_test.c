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
service_profile_callback( void *args ) {
}


static void
user_profile_callback( void *args ) {
}


int
main( int argc, char **argv ) {
  emirates_iface *iface = emirates_initialize();
  if ( iface != NULL ) {
    printf( "GOOD iface ptr\n" );

    subscribe_service_profile( iface, service_profile_callback );
    int i = 0;
    while( !zctx_interrupted ) {
      if ( !i ) {
        subscribe_user_profile( iface, user_profile_callback );
      }
      if ( i++ == 5 ) {
        publish_service_profile( iface );
      }
      zclock_sleep( 1 * 1000 );
    }

    emirates_finalize( &iface );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
