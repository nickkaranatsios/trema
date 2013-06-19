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


static pthread_key_t generic_int_key;
static pthread_once_t generic_int_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_int_key( void ) {
  pthread_key_create( &generic_int_key, free );
}


static int
jedex_generic_int_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int *self = ( int * ) vself;
  *self = 0;

  return 0;
}


static jedex_type
jedex_generic_int_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_INT32;
}


static jedex_schema *
jedex_generic_int_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_int();
}


static int
jedex_generic_int_get( const jedex_value_iface *iface, const void *vself, int32_t *out ) {
  UNUSED( iface );

  const int32_t *self = ( const int32_t * ) vself;
  *out = *self;

  return 0;
}


static int
jedex_generic_int_set( const jedex_value_iface *iface, void *vself, int32_t val ) {
  UNUSED( iface );

  int32_t *self = ( int32_t * ) vself;
  *self = val;

  return 0;
}


static size_t
jedex_generic_int_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( int32_t );
}


static int
jedex_generic_int_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int32_t *self = ( int32_t * ) vself;
  *self = 0;

  return 0;
}


static void
jedex_generic_int_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


jedex_generic_value_iface *
jedex_generic_int_class( void ) {
  pthread_once( &generic_int_key_once, make_generic_int_key );

  jedex_generic_value_iface *generic_int = ( jedex_generic_value_iface * ) pthread_getspecific( generic_int_key ); 
  if ( generic_int == NULL ) {
    generic_int = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_int_key, generic_int );

    memset( &generic_int->parent, 0, sizeof( generic_int->parent ) );
    generic_int->parent.reset = jedex_generic_int_reset;
    generic_int->parent.get_type = jedex_generic_int_get_type;
    generic_int->parent.get_schema = jedex_generic_int_get_schema;
    generic_int->parent.get_int = jedex_generic_int_get;

    generic_int->parent.set_int = jedex_generic_int_set;
    generic_int->instance_size = jedex_generic_int_instance_size;
    generic_int->init = jedex_generic_int_init;
    generic_int->done = jedex_generic_int_done;
  }

  return generic_int;
}


jedex_value_iface *
jedex_value_int_class( void ) {
  return &( ( jedex_generic_int_class() )->parent );
}


int
jedex_generic_int_new( jedex_value *value, int32_t val ) {
  int  rval;

  check( rval, jedex_generic_value_new( jedex_value_int_class(), value ) );

  return jedex_generic_int_set( value->iface, value->self, val );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
