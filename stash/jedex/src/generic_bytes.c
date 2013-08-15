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


static pthread_key_t generic_bytes_key = UINT_MAX;
static pthread_once_t generic_bytes_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_bytes_key( void ) {
  pthread_key_create( &generic_bytes_key, free );
}


static int
jedex_generic_bytes_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_clear( self );

  return 0;
}


static jedex_type
jedex_generic_bytes_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_BYTES;
}


static jedex_schema *
jedex_generic_bytes_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_bytes();
}


static int
jedex_generic_bytes_get( const jedex_value_iface *iface, const void *vself, const void **buf, size_t *size ) {
  UNUSED( iface );

  const jedex_raw_string *self = ( const jedex_raw_string * ) vself;
  if ( buf != NULL ) {
    *buf = jedex_raw_string_get( self );
  }
  if (size != NULL) {
    *size = jedex_raw_string_length( self );
  }

  return 0;
}


static int
jedex_generic_bytes_grab( const jedex_value_iface *iface, const void *vself, jedex_wrapped_buffer *dest ) {
  UNUSED( iface );

  const jedex_raw_string *self = ( const jedex_raw_string * ) vself;

  return jedex_raw_string_grab( self, dest );
}


static int
jedex_generic_bytes_set( const jedex_value_iface *iface, void *vself, void *buf, size_t size ) {
  UNUSED( iface );

  check_param( EINVAL, buf != NULL, "bytes contents" );
  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_set_length( self, buf, size );

  return 0;
}


static int
jedex_generic_bytes_give( const jedex_value_iface *iface, void *vself, jedex_wrapped_buffer *buf ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_give( self, buf );

  return 0;
}


static size_t
jedex_generic_bytes_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( jedex_raw_string );
}


static int
jedex_generic_bytes_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_init( self );

  return 0;
}


static void
jedex_generic_bytes_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_done( self );
}


void
jedex_generic_bytes_free( void ) {
  if ( generic_bytes_key == UINT_MAX ) {
    return;
  }
  jedex_generic_value_iface *generic_bytes = ( jedex_generic_value_iface * ) pthread_getspecific( generic_bytes_key ); 
  jedex_free( generic_bytes );
  generic_bytes = NULL;
  pthread_setspecific( generic_bytes_key, generic_bytes );
}

  
jedex_generic_value_iface *
jedex_generic_bytes_class( void ) {
  pthread_once( &generic_bytes_key_once, make_generic_bytes_key );

  jedex_generic_value_iface *generic_bytes = ( jedex_generic_value_iface * ) pthread_getspecific( generic_bytes_key ); 
  if ( generic_bytes == NULL ) {
    generic_bytes = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_bytes_key, generic_bytes );

    memset( &generic_bytes->parent, 0, sizeof( generic_bytes->parent ) );
    generic_bytes->parent.incref = jedex_generic_value_incref;
    generic_bytes->parent.decref = jedex_generic_value_decref;
    generic_bytes->parent.reset = jedex_generic_bytes_reset;
    generic_bytes->parent.get_type = jedex_generic_bytes_get_type;
    generic_bytes->parent.get_schema = jedex_generic_bytes_get_schema;
    generic_bytes->parent.get_bytes = jedex_generic_bytes_get;
    generic_bytes->parent.grab_bytes = jedex_generic_bytes_grab;

    generic_bytes->parent.set_bytes = jedex_generic_bytes_set;
    generic_bytes->parent.give_bytes = jedex_generic_bytes_give;
    generic_bytes->instance_size = jedex_generic_bytes_instance_size;
    generic_bytes->init = jedex_generic_bytes_init;
    generic_bytes->done = jedex_generic_bytes_done;
  }

  return generic_bytes;
}


jedex_value_iface *
jedex_value_bytes_class( void ) {
  return &( jedex_generic_bytes_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
