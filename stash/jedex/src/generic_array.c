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


static pthread_key_t generic_array_key;
static pthread_once_t generic_array_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_array_key( void ) {
  pthread_key_create( &generic_array_key, free );
}


static void
jedex_generic_array_free_elements( const jedex_generic_value_iface *child_giface, jedex_generic_array *self ) {
  size_t i;

  for ( i = 0; i < jedex_raw_array_size( &self->array ); i++ ) {
    void *child_self = jedex_raw_array_get_raw( &self->array, i );
    jedex_value_done( child_giface, child_self );
  }
}


static int
jedex_generic_array_reset( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );
  jedex_generic_array *self = ( jedex_generic_array * ) vself;
  jedex_generic_array_free_elements( iface->child_giface, self );
  jedex_raw_array_clear( &self->array );

  return 0;
}


static jedex_type
jedex_generic_array_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );
  UNUSED( vself );

  return JEDEX_ARRAY;
}


static jedex_schema *
jedex_generic_array_get_schema( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( vself );

  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );

  return iface->schema;
}


static int
jedex_generic_array_get_size( const jedex_value_iface *viface, const void *vself, size_t *size ) {
  UNUSED( viface );

  const jedex_generic_array *self = ( const jedex_generic_array * ) vself;
  if ( size != NULL ) {
    *size = jedex_raw_array_size( &self->array );
  }

  return 0;
}


static int
jedex_generic_array_get_by_index( const jedex_value_iface *viface,
                                  const void *vself,
                                  size_t index,
                                  jedex_value *child,
                                  const char **name ) {
  UNUSED( name );

  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );
  const jedex_generic_array *self = ( jedex_generic_array * ) vself;

  if ( index >= jedex_raw_array_size( &self->array ) ) {
    log_err( "Array index %zu out of range", index );
    return EINVAL;
  }
  child->iface = &iface->child_giface->parent;
  child->self = jedex_raw_array_get_raw( &self->array, index );

  return 0;
}


static int
jedex_generic_array_append( const jedex_value_iface *viface,
                            void *vself,
                            jedex_value *child,
                            size_t *new_index ) {
  int rval;
  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );

  jedex_generic_array *self = ( jedex_generic_array * ) vself;
  child->iface = &iface->child_giface->parent;
  child->self = jedex_raw_array_append( &self->array );
  if (child->self == NULL) {
    log_err( "Couldn't expand array" );
    return ENOMEM;
  }
  check( rval, jedex_value_init( iface->child_giface, child->self ) );
  if ( new_index != NULL ) {
    *new_index = jedex_raw_array_size( &self->array ) - 1;
  }

  return 0;
}


static size_t
jedex_generic_array_instance_size( const jedex_value_iface *viface ) {
  UNUSED( viface );

  return sizeof( jedex_generic_array );
}


static int
jedex_generic_array_init( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );
  jedex_generic_array *self = ( jedex_generic_array * ) vself;

  size_t child_size = jedex_value_instance_size( iface->child_giface );
  jedex_raw_array_init( &self->array, child_size );

  return 0;
}


static void
jedex_generic_array_done( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_array_value_iface *iface = container_of( viface, jedex_generic_array_value_iface, parent );
  jedex_generic_array *self = ( jedex_generic_array * ) vself;
  jedex_generic_array_free_elements( iface->child_giface, self );
  jedex_raw_array_done( &self->array );
}


jedex_generic_value_iface *
generic_array_class( void ) {
  pthread_once( &generic_array_key_once, make_generic_array_key );

  jedex_generic_value_iface *generic_array = ( jedex_generic_value_iface * ) pthread_getspecific( generic_array_key ); 
  if ( generic_array == NULL ) {
    generic_array = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_array_key, generic_array );

    memset( &generic_array->parent, 0, sizeof( generic_array->parent ) );
    generic_array->parent.reset = jedex_generic_array_reset;
    generic_array->parent.get_type = jedex_generic_array_get_type;
    generic_array->parent.get_schema = jedex_generic_array_get_schema;
    generic_array->parent.get_size = jedex_generic_array_get_size;
    generic_array->parent.get_by_index = jedex_generic_array_get_by_index;
    generic_array->parent.append = jedex_generic_array_append;

    generic_array->instance_size = jedex_generic_array_instance_size;
    generic_array->init = jedex_generic_array_init;
    generic_array->done = jedex_generic_array_done;
  }

  return generic_array;
}


jedex_generic_value_iface *
jedex_generic_array_class( jedex_schema *schema, memoize_state *state ) {
  jedex_schema *child_schema = jedex_schema_array_items( schema );
  jedex_generic_value_iface *child_giface = jedex_generic_class_from_schema_memoized( child_schema, state );
  if ( child_giface == NULL ) {
    return NULL;
  }

  size_t child_size = jedex_value_instance_size( child_giface );
  if ( child_size == 0 ) {
    log_err( "Array item class must provide instance_size" );
    return NULL;
  }

  jedex_generic_array_value_iface *iface = ( jedex_generic_array_value_iface * ) jedex_new( jedex_generic_array_value_iface );
  if ( iface == NULL ) {
    return NULL;
  }

  /*
   * TODO: Maybe check that schema.items matches
   * child_iface.get_schema?
   */
  memcpy( &iface->parent, generic_array_class(), sizeof( iface->parent ) );
  iface->schema = schema;
  iface->child_giface = child_giface;

  return &iface->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
