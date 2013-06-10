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


#include <pthread.h>
#include "priv.h"
#include "allocation.h"
#include "generic_internal.h"


static pthread_key_t generic_boolean_key;
static pthread_once_t generic_boolean_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_boolean_key( void ) {
  pthread_key_create( &generic_boolean_key, free );
}


static int
jedex_generic_boolean_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int *self = ( int * ) vself;
  *self = 0;

  return 0;
}


static jedex_type
jedex_generic_boolean_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_BOOLEAN;
}


static jedex_schema *
jedex_generic_boolean_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_boolean();
}


static int
jedex_generic_boolean_get( const jedex_value_iface *iface, const void *vself, int *out ) {
  UNUSED(iface);

  const int *self = ( const int * ) vself;
  *out = *self;

  return 0;
}


static int
jedex_generic_boolean_set( const jedex_value_iface *iface, void *vself, int val ) {
  UNUSED( iface );

  int *self = ( int * ) vself;
  *self = val;

  return 0;
}


static size_t
jedex_generic_boolean_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( int );
}


static int
jedex_generic_boolean_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int *self = ( int * ) vself;
  *self = 0;

  return 0;
}


static void
jedex_generic_boolean_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


static jedex_generic_value_iface *
generic_boolean_get( void ) {
  pthread_once( &generic_boolean_key_once, make_generic_boolean_key );

  jedex_generic_value_iface *generic_boolean = ( jedex_generic_value_iface * ) pthread_getspecific( generic_boolean_key ); 
  if ( generic_boolean == NULL ) {
    generic_boolean = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_boolean_key, generic_boolean );

    memset( &generic_boolean->parent, 0, sizeof( generic_boolean->parent ) );
    generic_boolean->parent.reset = jedex_generic_boolean_reset;
    generic_boolean->parent.get_type = jedex_generic_boolean_get_type;
    generic_boolean->parent.get_schema = jedex_generic_boolean_get_schema;
    generic_boolean->parent.get_boolean = jedex_generic_boolean_get;

    generic_boolean->parent.set_boolean = jedex_generic_boolean_set;
    generic_boolean->instance_size = jedex_generic_boolean_instance_size;
    generic_boolean->init = jedex_generic_boolean_init;
    generic_boolean->done = jedex_generic_boolean_done;
  }

  return generic_boolean;
}


jedex_value_iface *
jedex_generic_boolean_class( void ) {
  return &( generic_boolean_get() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
