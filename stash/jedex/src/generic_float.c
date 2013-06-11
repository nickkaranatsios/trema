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


static pthread_key_t generic_float_key;
static pthread_once_t generic_float_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_float_key( void ) {
  pthread_key_create( &generic_float_key, free );
}


static int
jedex_generic_float_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  float *self = ( float * ) vself;
  *self = 0.0f;

  return 0;
}


static jedex_type
jedex_generic_float_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_FLOAT;
}


static jedex_schema *
jedex_generic_float_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_float();
}


static int
jedex_generic_float_get( const jedex_value_iface *iface, const void *vself, float *out ) {
  UNUSED( iface );

  const float *self = ( const float * ) vself;
  *out = *self;

  return 0;
}


static int
jedex_generic_float_set( const jedex_value_iface *iface, void *vself, float val ) {
  UNUSED( iface );

  float *self = ( float * ) vself;
  *self = val;

  return 0;
}


static size_t
jedex_generic_float_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( float );
}


static int
jedex_generic_float_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  float *self = ( float * ) vself;
  *self = 0.0f;

  return 0;
}


static void
jedex_generic_float_done(const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


jedex_generic_value_iface *
jedex_generic_float_class( void ) {
  pthread_once( &generic_float_key_once, make_generic_float_key );

  jedex_generic_value_iface *generic_float = ( jedex_generic_value_iface * ) pthread_getspecific( generic_float_key ); 
  if ( generic_float == NULL ) {
    generic_float = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_float_key, generic_float );

    memset( &generic_float->parent, 0, sizeof( generic_float->parent ) );
    generic_float->parent.reset = jedex_generic_float_reset;
    generic_float->parent.get_type = jedex_generic_float_get_type;
    generic_float->parent.get_schema = jedex_generic_float_get_schema;
    generic_float->parent.get_float = jedex_generic_float_get;

    generic_float->parent.set_float = jedex_generic_float_set;
    generic_float->instance_size = jedex_generic_float_instance_size;
    generic_float->init = jedex_generic_float_init;
    generic_float->done = jedex_generic_float_done;
  }

  return generic_float;
}


jedex_value_iface *
jedex_value_float_class( void ) {
  return &( jedex_generic_float_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
