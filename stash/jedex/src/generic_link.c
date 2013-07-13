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


static pthread_key_t generic_link_key;
static pthread_once_t generic_link_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_link_key( void ) {
  pthread_key_create( &generic_link_key, free );
}


static int
jedex_generic_link_reset( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_reset( self );
}


static jedex_type
jedex_generic_link_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_type( self );
}


static jedex_schema *
jedex_generic_link_get_schema( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_schema( self );
}


static int
jedex_generic_link_get_boolean( const jedex_value_iface *iface, const void *vself, int *out ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_boolean( self, out );
}


static int
jedex_generic_link_get_bytes( const jedex_value_iface *iface, const void *vself, const void **buf, size_t *size ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_bytes( self, buf, size );
}


static int
jedex_generic_link_grab_bytes( const jedex_value_iface *iface, const void *vself, jedex_wrapped_buffer *dest ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_grab_bytes( self, dest );
}


static int
jedex_generic_link_get_double( const jedex_value_iface *iface, const void *vself, double *out ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_double( self, out );
}


static int
jedex_generic_link_get_float( const jedex_value_iface *iface, const void *vself, float *out ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_float( self, out );
}


static int
jedex_generic_link_get_int( const jedex_value_iface *iface, const void *vself, int32_t *out ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_int( self, out );
}


static int
jedex_generic_link_get_long( const jedex_value_iface *iface, const void *vself, int64_t *out ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_long( self, out );
}


static int
jedex_generic_link_get_null( const jedex_value_iface *iface, const void *vself ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_null( self );
}


static int
jedex_generic_link_get_string( const jedex_value_iface *iface, const void *vself, const char **str, size_t *size ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_string( self, str, size );
}


static int
jedex_generic_link_grab_string( const jedex_value_iface *iface, const void *vself, jedex_wrapped_buffer *dest ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_grab_string( self, dest );
}


static int
jedex_generic_link_set_boolean( const jedex_value_iface *iface, void *vself, int val ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_boolean( self, val );
}


static int
jedex_generic_link_set_bytes( const jedex_value_iface *iface, void *vself, void *buf, size_t size ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_bytes( self, buf, size );
}


static int
jedex_generic_link_give_bytes( const jedex_value_iface *iface, void *vself, jedex_wrapped_buffer *buf ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_give_bytes( self, buf );
}


static int
jedex_generic_link_set_double( const jedex_value_iface *iface, void *vself, double val ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_double( self, val );
}


static int
jedex_generic_link_set_float( const jedex_value_iface *iface, void *vself, float val ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_float( self, val );
}


static int
jedex_generic_link_set_int( const jedex_value_iface *iface, void *vself, int32_t val ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_int( self, val );
}


static int
jedex_generic_link_set_long( const jedex_value_iface *iface, void *vself, int64_t val ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_long( self, val );
}


static int
jedex_generic_link_set_null( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_null( self );
}


static int
jedex_generic_link_set_string( const jedex_value_iface *iface, void *vself, const char *str ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_string( self, str );
}


static int
jedex_generic_link_set_string_len( const jedex_value_iface *iface, void *vself, const char *str, size_t size ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_set_string_len( self, str, size );
}


static int
jedex_generic_link_give_string_len( const jedex_value_iface *iface, void *vself, jedex_wrapped_buffer *buf ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_give_string_len( self, buf );
}


static int
jedex_generic_link_get_size( const jedex_value_iface *iface, const void *vself, size_t *size ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;

  return jedex_value_get_size( self, size );
}


static int
jedex_generic_link_get_by_index( const jedex_value_iface *iface,
                                 const void *vself,
                                 size_t index,
                                 jedex_value *child,
                                 const char **name ) {
  UNUSED( iface );

  const jedex_value *self = ( const jedex_value * ) vself;
  return jedex_value_get_by_index(self, index, child, name);
}

static int
jedex_generic_link_get_by_name( const jedex_value_iface *iface,
                                void *vself,
                                const char *name,
                                jedex_value *child,
                                size_t *index ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_get_by_name( self, name, child, index );
}


static int
jedex_generic_link_append( const jedex_value_iface *iface,
                           void *vself,
                           jedex_value *child_out,
                           size_t *new_index ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_append( self, child_out, new_index );
}


static int
jedex_generic_link_add( const jedex_value_iface *iface,
                        void *vself,
                        const char *key,
                        jedex_value *child,
                        size_t *index,
                        int *is_new ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;

  return jedex_value_add( self, key, child, index, is_new );
}


static size_t
jedex_generic_link_instance_size( const jedex_value_iface *viface ) {
  UNUSED( viface );

  return sizeof( jedex_value );
}


static int
jedex_generic_link_init( const jedex_value_iface *viface, void *vself ) {
  int rval;

  jedex_generic_link_value_iface *iface = container_of( viface, jedex_generic_link_value_iface, parent.parent );

  jedex_value *self = ( jedex_value * ) vself;
  size_t target_instance_size = jedex_value_instance_size( iface->target_giface );
  if ( target_instance_size == 0 ) {
    return EINVAL;
  }

  self->iface = &iface->target_giface->parent;
  self->self = jedex_malloc( target_instance_size );
  if ( self->self == NULL ) {
    return ENOMEM;
  }

  rval = jedex_value_init( iface->target_giface, self->self );
  if ( rval != 0 ) {
    jedex_free( self->self );
  }

  return rval;
}


static void
jedex_generic_link_done( const jedex_value_iface *iface, void *vself ) {
  UNUSED( iface );

  jedex_value *self = ( jedex_value * ) vself;
  jedex_generic_value_iface *target_giface = container_of( self->iface, jedex_generic_value_iface, parent );
  jedex_value_done( target_giface, self->self );
  jedex_free( self->self );
  self->iface = NULL;
  self->self = NULL;
}


static jedex_generic_value_iface *
generic_link_class( void ) {
  pthread_once( &generic_link_key_once, make_generic_link_key );

  jedex_generic_value_iface *link = ( jedex_generic_value_iface * ) pthread_getspecific( generic_link_key ); 
  if ( link == NULL ) {
    link = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_link_key, link );
    memset( &link->parent, 0, sizeof( link->parent ) );
    link->parent.reset = jedex_generic_link_reset;
    link->parent.get_type = jedex_generic_link_get_type;
    link->parent.get_schema = jedex_generic_link_get_schema;
    link->parent.get_boolean = jedex_generic_link_get_boolean;
    link->parent.get_bytes = jedex_generic_link_get_bytes;
    link->parent.grab_bytes = jedex_generic_link_grab_bytes;
    link->parent.get_double = jedex_generic_link_get_double;
    link->parent.get_float = jedex_generic_link_get_float;
    link->parent.get_int = jedex_generic_link_get_int;
    link->parent.get_long = jedex_generic_link_get_long;
    link->parent.get_null = jedex_generic_link_get_null;
    link->parent.get_string = jedex_generic_link_get_string;
    link->parent.grab_string = jedex_generic_link_grab_string;
    link->parent.get_size = jedex_generic_link_get_size;
    link->parent.get_by_index = jedex_generic_link_get_by_index;
    link->parent.get_by_name = jedex_generic_link_get_by_name;

    link->parent.set_boolean = jedex_generic_link_set_boolean;
    link->parent.set_bytes = jedex_generic_link_set_bytes;
    link->parent.give_bytes = jedex_generic_link_give_bytes;
    link->parent.set_double = jedex_generic_link_set_double;
    link->parent.set_float = jedex_generic_link_set_float;
    link->parent.set_int = jedex_generic_link_set_int;
    link->parent.set_long = jedex_generic_link_set_long;
    link->parent.set_null = jedex_generic_link_set_null;
    link->parent.set_string = jedex_generic_link_set_string;
    link->parent.set_string_len = jedex_generic_link_set_string_len;
    link->parent.give_string_len = jedex_generic_link_give_string_len;

    link->parent.append = jedex_generic_link_append;
    link->parent.add = jedex_generic_link_add;

    link->instance_size = jedex_generic_link_instance_size;
    link->init = jedex_generic_link_init;
    link->done = jedex_generic_link_done;
  }

  return link;
}


jedex_generic_link_value_iface *
jedex_generic_link_class( jedex_schema *schema ) {
  if ( !is_jedex_link( schema ) ) {
    log_err( "Expected link schema" );
    return NULL;
  }

  jedex_generic_link_value_iface *iface = ( jedex_generic_link_value_iface * ) jedex_new( jedex_generic_link_value_iface );
  if ( iface == NULL ) {
    return NULL;
  }

  memcpy( &iface->parent, generic_link_class(), sizeof( iface->parent ) );
  iface->schema = schema;
  iface->next = NULL;

  return iface;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
