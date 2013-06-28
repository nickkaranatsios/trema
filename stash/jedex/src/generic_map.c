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


static pthread_key_t generic_map_key;
static pthread_once_t generic_map_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_map_key( void ) {
  pthread_key_create( &generic_map_key, free );
}


static void
jedex_generic_map_free_elements( const jedex_generic_value_iface *child_giface, jedex_generic_map *self ) {
 size_t i;

  for ( i = 0; i < jedex_raw_map_size( &self->map ); i++ ) {
    void *child_self = jedex_raw_map_get_raw( &self->map, i );
    jedex_value_done( child_giface, child_self );
  }
}


static int
jedex_generic_map_reset( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  jedex_generic_map *self = ( jedex_generic_map * ) vself;
  jedex_generic_map_free_elements( iface->child_giface, self );
  jedex_raw_map_clear( &self->map );

  return 0;
}


static jedex_type
jedex_generic_map_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );
  UNUSED( vself );

  return JEDEX_MAP;
}


static jedex_schema *
jedex_generic_map_get_schema( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( vself );

  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );

  return iface->schema;
}


static int
jedex_generic_map_get_size( const jedex_value_iface *viface, const void *vself, size_t *size ) {
  UNUSED( viface );
  
  const jedex_generic_map *self = ( const jedex_generic_map * ) vself;
  if ( size != NULL ) {
    *size = jedex_raw_map_size( &self->map );
  }

  return 0;
}


static int
jedex_generic_map_get_by_index( const jedex_value_iface *viface,
                                const void *vself,
                                size_t index,
                                jedex_value *child,
                                const char **name ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  const jedex_generic_map *self = ( const jedex_generic_map * ) vself;

  if ( index >= jedex_raw_map_size( &self->map ) ) {
    log_err( "Map index %zu out of range", index );
    return EINVAL;
  }
  child->iface = &iface->child_giface->parent;
  child->self = jedex_raw_map_get_raw( &self->map, index );
  if ( name != NULL ) {
    *name = jedex_raw_map_get_key( &self->map, index );
  }

  return 0;
}


static int
jedex_generic_map_get_by_name( const jedex_value_iface *viface,
                               void *vself,
                               const char *name,
                               jedex_value *child,
                               size_t *index ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  jedex_generic_map *self = ( jedex_generic_map * ) vself;
  child->iface = &iface->child_giface->parent;
  child->self = jedex_raw_map_get( &self->map, name, index );

  return 0;
}


static int
jedex_generic_map_add( const jedex_value_iface *viface,
                       void *vself,
                       const char *key,
                       jedex_value *child,
                       size_t *index,
                       int *is_new ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  int rval;

  jedex_generic_map *self = ( jedex_generic_map * ) vself;
  child->iface = &iface->child_giface->parent;
  rval = jedex_raw_map_get_or_create( &self->map, key, &child->self, index );
  if ( rval < 0 ) {
    return -rval;
  }
  if ( is_new != NULL ) {
    *is_new = rval;
  }
  if ( rval ) {
    check( rval, jedex_value_init( iface->child_giface, child->self ) );
  }

  return 0;
}


static size_t
jedex_generic_map_instance_size( const jedex_value_iface *viface ) {
  UNUSED( viface );

  return sizeof( jedex_generic_map );
}


static int
jedex_generic_map_init( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  jedex_generic_map *self = ( jedex_generic_map * ) vself;

  size_t child_size = jedex_value_instance_size( iface->child_giface );
  jedex_raw_map_init( &self->map, child_size );

  return 0;
}


static void
jedex_generic_map_done( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_map_value_iface *iface = container_of( viface, jedex_generic_map_value_iface, parent );
  jedex_generic_map *self = ( jedex_generic_map * ) vself;
  jedex_generic_map_free_elements( iface->child_giface, self );
  jedex_raw_map_done( &self->map );
}


static jedex_generic_value_iface *
jedex_generic_map_get( void ) {
  pthread_once( &generic_map_key_once, make_generic_map_key );

  jedex_generic_value_iface *generic_map = ( jedex_generic_value_iface * ) pthread_getspecific( generic_map_key ); 
  if ( generic_map == NULL ) {
    generic_map = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_map_key, generic_map );
    memset( &generic_map->parent, 0, sizeof( generic_map->parent ) );
    generic_map->parent.reset = jedex_generic_map_reset;
    generic_map->parent.get_type = jedex_generic_map_get_type;
    generic_map->parent.get_schema = jedex_generic_map_get_schema;

    generic_map->parent.get_size = jedex_generic_map_get_size;
    generic_map->parent.get_by_index = jedex_generic_map_get_by_index;
    generic_map->parent.get_by_name = jedex_generic_map_get_by_name;
    generic_map->parent.add = jedex_generic_map_add;

    generic_map->instance_size = jedex_generic_map_instance_size;
    generic_map->init = jedex_generic_map_init;
    generic_map->done = jedex_generic_map_done;
  }

  return generic_map;
}


jedex_generic_value_iface *
jedex_generic_map_class( jedex_schema *schema, memoize_state *state ) {
  jedex_schema *child_schema = jedex_schema_array_items( schema );
  jedex_generic_value_iface *child_giface = jedex_generic_class_from_schema_memoized( schema, state );
  if ( child_giface == NULL ) {
    return NULL;
  }

  size_t child_size = jedex_value_instance_size( child_giface );
  if ( child_size == 0 ) {
    log_err("Map value class must provide instance_size");
    return NULL;
  }

  jedex_generic_map_value_iface *iface = ( jedex_generic_map_value_iface * ) jedex_new( jedex_generic_map_value_iface );
  if ( iface == NULL ) {
    return NULL;
  }

  /*
   * TODO: Maybe check that schema.items matches
   * child_iface.get_schema?
   */

  memcpy( &iface->parent, jedex_generic_map_get(), sizeof( iface->parent ) );
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
