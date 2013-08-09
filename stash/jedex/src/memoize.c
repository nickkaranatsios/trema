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


typedef struct jedex_memoize_key {
  void *key1;
  void *key2;
} jedex_memoize_key;


static int
jedex_memoize_key_cmp( jedex_memoize_key *a, jedex_memoize_key *b ) {
  /*
   * This isn't a proper cmp operation, since it always returns 1
   * if the keys are different.  But that's okay for the hash
   * table implementation we're using.
   */
  return ( a->key1 != b->key1 ) || ( a->key2 != b->key2 );
}


static int
jedex_memoize_key_hash( jedex_memoize_key *a ) {
  return ( int ) ( ( ( uintptr_t ) a->key1 ) ^ ( ( uintptr_t ) a->key2 ) );
}


static struct st_hash_type jedex_memoize_hash_type = {
  HASH_FUNCTION_CAST jedex_memoize_key_cmp,
  HASH_FUNCTION_CAST jedex_memoize_key_hash
};


void
jedex_memoize_init( jedex_memoize *mem ) {
  memset( mem, 0, sizeof(jedex_memoize ) );
  mem->cache = st_init_table( &jedex_memoize_hash_type );
}


static int
jedex_memoize_free_key( jedex_memoize_key *key, void *result, void *dummy ) {
  UNUSED( result );
  UNUSED( dummy );

  jedex_free( key );

  return ST_CONTINUE;
}


void
jedex_memoize_done(jedex_memoize *mem ) {
  st_foreach( ( st_table * ) mem->cache, HASH_FUNCTION_CAST jedex_memoize_free_key, 0 );
  st_free_table( ( st_table * ) mem->cache );
  memset( mem, 0, sizeof(jedex_memoize )) ;
}


int
jedex_memoize_get( jedex_memoize *mem, void *key1, void *key2, void **result ) {
  jedex_memoize_key key;
  key.key1 = key1;
  key.key2 = key2;

  union {
    st_data_t data;
    void *value;
  } val;

  if ( st_lookup( ( st_table * ) mem->cache, ( st_data_t ) &key, &val.data ) ) {
    if ( result ) {
      *result = val.value;
    }
    return 1;
  }
  else {
    return 0;
  }
}


void
jedex_memoize_set( jedex_memoize *mem, void *key1, void *key2, void *result ) {
  /*
   * First see if there's already a cached value for this key.  If
   * so, we don't want to allocate a new jedex_memoize_key
   * instance.
   */

  jedex_memoize_key key;
  key.key1 = key1;
  key.key2 = key2;

  union {
    st_data_t data;
    void *value;
  } val;

  if ( st_lookup( ( st_table * ) mem->cache, ( st_data_t ) &key, &val.data ) ) {
    st_insert( ( st_table * ) mem->cache, ( st_data_t ) &key, ( st_data_t ) result );
    return;
  }

  /*
   * If it's a new key pair, then we do need to allocate.
   */

  jedex_memoize_key *real_key = ( jedex_memoize_key * ) jedex_new( jedex_memoize_key );
  real_key->key1 = key1;
  real_key->key2 = key2;

  st_insert( ( st_table * ) mem->cache, ( st_data_t ) real_key, ( st_data_t ) result );
}


void
jedex_memoize_delete( jedex_memoize *mem, void *key1, void *key2 ) {
  jedex_memoize_key key;
  key.key1 = key1;
  key.key2 = key2;

  union {
    st_data_t data;
    jedex_memoize_key *key;
  } real_key;

  real_key.key = &key;
  if ( st_delete( ( st_table * ) mem->cache, &real_key.data, NULL ) ) {
    jedex_free( real_key.key );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
