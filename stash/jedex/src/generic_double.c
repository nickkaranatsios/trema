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


static pthread_key_t generic_double_key;
static pthread_once_t generic_double_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_double_key( void ) {
  pthread_key_create( &generic_double_key, free );
}


static int
jedex_generic_double_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  double *self = ( double * ) vself;
  *self = 0.0;

  return 0;
}


static int
jedex_generic_double_free( jedex_value_iface *iface, void *vself ) {
  UNUSED( vself );

  jedex_free( iface );
  return 0;
}


static jedex_type
jedex_generic_double_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_DOUBLE;
}


static jedex_schema *
jedex_generic_double_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_double();
}


static int
jedex_generic_double_get( const jedex_value_iface *iface, const void *vself, double *out ) {
  UNUSED( iface );

  const double *self = ( const double * ) vself;
  *out = *self;

  return 0;
}


static int
jedex_generic_double_set( const jedex_value_iface *iface, void *vself, double val ) {
  UNUSED( iface );

  double *self = ( double * ) vself;
  *self = val;

  return 0;
}


static size_t
jedex_generic_double_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( double );
}


static int
jedex_generic_double_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  double *self = ( double * ) vself;
  *self = 0.0;

  return 0;
}


static void
jedex_generic_double_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


jedex_generic_value_iface *
jedex_generic_double_class( void ) {
  pthread_once( &generic_double_key_once, make_generic_double_key );

  jedex_generic_value_iface *generic_double = ( jedex_generic_value_iface * ) pthread_getspecific( generic_double_key ); 
  if ( generic_double == NULL ) {
    generic_double = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_double_key, generic_double );

    memset( &generic_double->parent, 0, sizeof( generic_double->parent ) );
    generic_double->parent.reset = jedex_generic_double_reset;
    generic_double->parent.free = jedex_generic_double_free;
    generic_double->parent.get_type = jedex_generic_double_get_type;
    generic_double->parent.get_schema = jedex_generic_double_get_schema;
    generic_double->parent.get_double = jedex_generic_double_get;

    generic_double->parent.set_double = jedex_generic_double_set;
    generic_double->instance_size = jedex_generic_double_instance_size;
    generic_double->init = jedex_generic_double_init;
    generic_double->done = jedex_generic_double_done;
  }

  return generic_double;
}


jedex_value_iface *
jedex_value_generic_double_class( void ) {
  return &( jedex_generic_double_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
