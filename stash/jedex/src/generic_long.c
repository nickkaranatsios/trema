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


static pthread_key_t generic_long_key = UINT_MAX;
static pthread_once_t generic_long_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_long_key( void ) {
  pthread_key_create( &generic_long_key, free );
}


static int
jedex_generic_long_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int64_t *self = ( int64_t * ) vself;
  *self = 0;

  return 0;
}


static jedex_type
jedex_generic_long_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_INT64;
}


static jedex_schema *
jedex_generic_long_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself);

  return jedex_schema_long();
}


static int
jedex_generic_long_get( const jedex_value_iface *iface, const void *vself, int64_t *out ) {
  UNUSED( iface );

  const int64_t *self = ( const int64_t * ) vself;
  *out = *self;

  return 0;
}


static int
jedex_generic_long_set( const jedex_value_iface *iface, void *vself, int64_t val ) {
  UNUSED( iface );

  int64_t *self = ( int64_t * ) vself;
  *self = val;

  return 0;
}


static size_t
jedex_generic_long_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( int64_t );
}


static int
jedex_generic_long_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  int64_t *self = ( int64_t * ) vself;
  *self = 0;

  return 0;
}


static void
jedex_generic_long_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );
  UNUSED( vself );
}


void
jedex_generic_long_free( void ) {
  if ( generic_long_key == UINT_MAX ) {
    return;
  }
  jedex_generic_value_iface *generic_long = ( jedex_generic_value_iface * ) pthread_getspecific( generic_long_key ); 
  jedex_free( generic_long );
  generic_long = NULL;
  pthread_setspecific( generic_long_key, generic_long );
}


jedex_generic_value_iface *
jedex_generic_long_class( void ) {
  pthread_once( &generic_long_key_once, make_generic_long_key );

  jedex_generic_value_iface *generic_long = ( jedex_generic_value_iface * ) pthread_getspecific( generic_long_key ); 
  if ( generic_long == NULL ) {
    generic_long = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_long_key, generic_long );

    memset( &generic_long->parent, 0, sizeof( generic_long->parent ) );
    generic_long->parent.incref = jedex_generic_value_incref;
    generic_long->parent.decref = jedex_generic_value_decref;
    generic_long->parent.reset = jedex_generic_long_reset;
    generic_long->parent.get_type = jedex_generic_long_get_type;
    generic_long->parent.get_schema = jedex_generic_long_get_schema;
    generic_long->parent.get_long = jedex_generic_long_get;

    generic_long->parent.set_long = jedex_generic_long_set;
    generic_long->instance_size = jedex_generic_long_instance_size;
    generic_long->init = jedex_generic_long_init;
    generic_long->done = jedex_generic_long_done;
  }

  return generic_long;
}


jedex_value_iface *
jedex_value_long_class( void ) {
  return &( jedex_generic_long_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
