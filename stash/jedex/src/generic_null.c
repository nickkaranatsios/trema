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


#include "jedex.h"


static pthread_key_t generic_null_key;
static pthread_once_t generic_null_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_null_key( void ) {
  pthread_key_create( &generic_null_key, free );
}


static int
jedex_generic_null_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int  *self = ( int * ) vself;
  *self = 0;

  return 0;
}


static int
jedex_generic_null_free( jedex_value_iface *iface, void *vself ) {
  UNUSED( vself );

  jedex_free( iface );

  return 0;
}


static jedex_type
jedex_generic_null_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_NULL;
}


static jedex_schema *
jedex_generic_null_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_null();
}


static int
jedex_generic_null_get( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return 0;
}


static int
jedex_generic_null_set( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return 0;
}


static size_t
jedex_generic_null_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( int );
}


static int
jedex_generic_null_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int *self = ( int * ) vself;
  *self = 0;

  return 0;
}


static void
jedex_generic_null_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


jedex_generic_value_iface *
jedex_generic_null_class( void ) {
  pthread_once( &generic_null_key_once, make_generic_null_key );

  jedex_generic_value_iface *generic_null = ( jedex_generic_value_iface * ) pthread_getspecific( generic_null_key ); 
  if ( generic_null == NULL ) {
    generic_null = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_null_key, generic_null );

    memset( &generic_null->parent, 0, sizeof( generic_null->parent ) );
    generic_null->parent.reset = jedex_generic_null_reset;
    generic_null->parent.free = jedex_generic_null_free;
    generic_null->parent.get_type = jedex_generic_null_get_type;
    generic_null->parent.get_schema = jedex_generic_null_get_schema;
    generic_null->parent.get_null = jedex_generic_null_get;

    generic_null->parent.set_null = jedex_generic_null_set;
    generic_null->instance_size = jedex_generic_null_instance_size;
    generic_null->init = jedex_generic_null_init;
    generic_null->done = jedex_generic_null_done;
  }

  return generic_null;
}


jedex_value_iface *
jedex_value_null_class( void ) {
  return &( jedex_generic_null_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
