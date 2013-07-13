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


#define raw_entry_size( element_size ) \
  ( sizeof( jedex_raw_map_entry ) + element_size )


void
jedex_raw_map_init( jedex_raw_map *map, size_t element_size ) {
  memset( map, 0, sizeof( jedex_raw_map ) );
  jedex_raw_array_init( &map->elements, raw_entry_size( element_size ) );
  map->indices_by_key = st_init_strtable();
}


static void
jedex_raw_map_free_keys( jedex_raw_map *map ) {
  uint32_t i;

  for ( i = 0; i < jedex_raw_map_size( map ); i++ ) {
    void *ventry = ( ( char * ) map->elements.data + map->elements.element_size * i );
    jedex_raw_map_entry *entry = ( jedex_raw_map_entry * ) ventry;
    jedex_free( ( char * ) entry->key );
  }
}


void
jedex_raw_map_done( jedex_raw_map *map ) {
  jedex_raw_map_free_keys( map );
  jedex_raw_array_done( &map->elements );
  st_free_table( ( st_table * ) map->indices_by_key );
  memset( map, 0, sizeof( jedex_raw_map ) );
}


void
jedex_raw_map_clear( jedex_raw_map *map ) {
  jedex_raw_map_free_keys( map );
  jedex_raw_array_clear( &map->elements );
  st_free_table( ( st_table * ) map->indices_by_key );
  map->indices_by_key = st_init_strtable();
}


int
jedex_raw_map_ensure_size( jedex_raw_map *map, size_t desired_count ) {
  return jedex_raw_array_ensure_size( &map->elements, desired_count );
}


void *
jedex_raw_map_get( const jedex_raw_map *map, const char *key, size_t *index ) {
  st_data_t data;

  if ( st_lookup( ( st_table * ) map->indices_by_key, ( st_data_t ) key, &data ) ) {
    uint32_t i = ( uint32_t ) data;
    if ( index ) {
      *index = i;
    }
    void *raw_entry = ( ( char * ) map->elements.data + map->elements.element_size * i );
    return ( char * ) raw_entry + sizeof( jedex_raw_map_entry );
  }
  else {
    return NULL;
  }
}


int
jedex_raw_map_get_or_create( jedex_raw_map *map, const char *key, void **element, size_t *index ) {
  st_data_t data;
  void *el;
  size_t i;
  int is_new;

  if ( st_lookup( ( st_table * ) map->indices_by_key, ( st_data_t) key, &data ) ) {
    i = ( uint32_t ) data;
    void *raw_entry = ( ( char * ) map->elements.data + map->elements.element_size * i );
    el = ( char * ) raw_entry + sizeof( jedex_raw_map_entry );
    is_new = 0;
  }
  else {
    i = map->elements.element_count;
    jedex_raw_map_entry *raw_entry = ( jedex_raw_map_entry * ) jedex_raw_array_append( &map->elements );
    raw_entry->key = strdup( key );
    st_insert( ( st_table * ) map->indices_by_key, ( st_data_t ) raw_entry->key, ( st_data_t ) i );
    if ( !raw_entry ) {
      jedex_free( ( char * ) raw_entry->key );
      return -ENOMEM;
    }
    el = ( ( char * ) raw_entry ) + sizeof( jedex_raw_map_entry );
    is_new = 1;
  }

  if ( element ) {
    *element = el;
  }
  if ( index ) {
    *index = i;
  }

  return is_new;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
