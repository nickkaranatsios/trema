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


static pthread_key_t generic_string_key;
static pthread_once_t generic_string_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_string_key( void ) {
  pthread_key_create( &generic_string_key, free );
}


static int
jedex_generic_string_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_clear( self );

  return 0;
}


static jedex_type
jedex_generic_string_get_type( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return JEDEX_STRING;
}


static jedex_schema *
jedex_generic_string_get_schema( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );
  UNUSED( vself );

  return jedex_schema_string();
}


static int
jedex_generic_string_get( const jedex_value_iface *iface, const void *vself, const char **str, size_t *size ) {
  UNUSED( iface );

  const jedex_raw_string *self = ( const jedex_raw_string * ) vself;
  const char *contents = ( const char * ) jedex_raw_string_get( self );

  if ( str != NULL ) {
    /*
     * We can't return a NULL string, we have to return an
     * *empty* string
     */

    *str = ( contents == NULL ) ? "" : contents;
  }
  if ( size != NULL ) {
    /* raw_string's length includes the NUL terminator,
     * unless it's empty */
    *size = ( contents == NULL ) ? 1 : jedex_raw_string_length( self );
  }

  return 0;
}


static int
jedex_generic_string_grab( const jedex_value_iface *iface, const void *vself, jedex_wrapped_buffer *dest ) {
  UNUSED( iface );

  const jedex_raw_string *self = ( const jedex_raw_string * ) vself;
  const char *contents = ( const char * ) jedex_raw_string_get( self );

  if ( contents == NULL ) {
    return jedex_wrapped_buffer_new( dest, "", 1 );
  }
  else {
    return jedex_raw_string_grab( self, dest );
  }
}


static int
jedex_generic_string_set( const jedex_value_iface *iface, void *vself, const char *val ) {
  UNUSED( iface );

  check_param( EINVAL, val != NULL, "string contents" );

  /*
   * This raw_string method ensures that we copy the NUL
   * terminator from val, and will include the NUL terminator in
   * the raw_string's length, which is what we want.
   */
  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_set( self, val );

  return 0;
}


static int
jedex_generic_string_set_length( const jedex_value_iface *iface, void *vself, const char *val, size_t size ) {
  UNUSED( iface );

  check_param( EINVAL, val != NULL, "string contents" );
  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_set_length( self, val, size );

  return 0;
}


static int
jedex_generic_string_give_length( const jedex_value_iface *iface, void *vself, jedex_wrapped_buffer *buf ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_give( self, buf );

  return 0;
}


static size_t
jedex_generic_string_instance_size( const jedex_value_iface *iface ) {
  UNUSED( iface );

  return sizeof( jedex_raw_string );
}


static int
jedex_generic_string_init( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_init( self );

  return 0;
}


static void
jedex_generic_string_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_raw_string *self = ( jedex_raw_string * ) vself;
  jedex_raw_string_done( self );
}


jedex_generic_value_iface *
jedex_generic_string_class( void ) {
  pthread_once( &generic_string_key_once, make_generic_string_key );

  jedex_generic_value_iface *generic_string = ( jedex_generic_value_iface * ) pthread_getspecific( generic_string_key ); 
  if ( generic_string == NULL ) {
    generic_string = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_string_key, generic_string );

    memset( &generic_string->parent, 0, sizeof( generic_string->parent ) );
    generic_string->parent.reset = jedex_generic_string_reset;
    generic_string->parent.get_type = jedex_generic_string_get_type;
    generic_string->parent.get_schema = jedex_generic_string_get_schema;
    generic_string->parent.get_string = jedex_generic_string_get;
    generic_string->parent.grab_string = jedex_generic_string_grab;

    generic_string->parent.set_string = jedex_generic_string_set;
    generic_string->parent.set_string_len = jedex_generic_string_set_length;
    generic_string->parent.give_string_len = jedex_generic_string_give_length;
    generic_string->instance_size = jedex_generic_string_instance_size;
    generic_string->init = jedex_generic_string_init;
    generic_string->done = jedex_generic_string_done;
  }

  return generic_string;
}


jedex_value_iface *
jedex_value_string_class( void ) {
  return  &( jedex_generic_string_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
