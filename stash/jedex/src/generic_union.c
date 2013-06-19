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


static pthread_key_t generic_union_key;
static pthread_once_t generic_union_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_union_key( void ) {
  pthread_key_create( &generic_union_key, free );
}


static int
jedex_generic_union_reset( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  jedex_generic_union *self = ( jedex_generic_union * ) vself; 
   

  int rval;
  for ( size_t i = 0; i < iface->branch_count; i++ ) {
    jedex_value value = {
      &iface->branch_ifaces[ i ]->parent,
      jedex_generic_union_branch( iface, self, i )
    };
    check( rval, jedex_value_reset( &value ) );
  }

  return 0;
}


static jedex_schema *
jedex_generic_union_get_schema( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( vself );

  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  return iface->schema;
}


static jedex_type
jedex_generic_union_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );
  UNUSED( vself );

  return JEDEX_UNION;
}


static int
jedex_generic_union_get_size( const jedex_value_iface *viface, const void *vself, size_t *size ) {
  UNUSED( vself );

  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  if ( size != NULL ) {
    *size = iface->branch_count;
  }

  return 0;
}


static int
jedex_generic_union_get_discriminant( const jedex_value_iface *viface,
                                      const void *vself,
                                      int *out ) {
  UNUSED( viface );

  const jedex_generic_union *self = ( const jedex_generic_union * ) vself;

  *out = self->discriminant;

  return 0;
}


static int
jedex_generic_union_get_branch( const jedex_value_iface *viface, 
                                void *vself,
                                size_t index,
                                jedex_value *branch ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  jedex_generic_union *self = ( jedex_generic_union * ) vself;

  if ( index >= iface->branch_count ) {
    return EINVAL;
  }

  self->discriminant = index;
  self->selected_branches[ index ] = index;
  branch->iface = &iface->branch_ifaces[ index ]->parent;
  branch->self = jedex_generic_union_branch( iface, self, index );

  return 0;
}


static size_t
jedex_generic_union_instance_size( const jedex_value_iface *viface ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  return iface->instance_size;
}


static int
jedex_generic_union_init( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  jedex_generic_union *self = ( jedex_generic_union * ) vself;

  size_t selected_branches_size = sizeof( int ) * iface->branch_count;
  self->selected_branches = ( int * ) jedex_malloc( selected_branches_size );
  size_t i;
  int rval;
  for ( i = 0; i < iface->branch_count; i++ ) {
    check( rval, jedex_value_init( iface->branch_ifaces[ i ], jedex_generic_union_branch( iface, self, i ) ) );
    self->selected_branches[ i ] = -1;
  }
  self->discriminant = -1;

  return 0;
}


static void
jedex_generic_union_done( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  jedex_generic_union *self = ( jedex_generic_union * ) vself;

  size_t i;
  for ( i = 0; i < iface->branch_count; i++ ) {
    jedex_value_done( iface->branch_ifaces[ i ], jedex_generic_union_branch( iface, self, i ) );
    self->selected_branches[ i ] = -1;
  }
  self->discriminant = -1;
}


static jedex_generic_value_iface *
generic_union_class( void ) {
  pthread_once( &generic_union_key_once, make_generic_union_key );

  jedex_generic_value_iface *generic_union = ( jedex_generic_value_iface * ) pthread_getspecific( generic_union_key ); 
  if ( generic_union == NULL ) {
    generic_union = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_union_key, generic_union );

    memset( &generic_union->parent, 0, sizeof( generic_union->parent ) );
    generic_union->parent.reset = jedex_generic_union_reset;
    generic_union->parent.get_type = jedex_generic_union_get_type;
    generic_union->parent.get_size = jedex_generic_union_get_size;
    generic_union->parent.get_schema = jedex_generic_union_get_schema;
    generic_union->parent.get_branch = jedex_generic_union_get_branch;
    generic_union->parent.get_discriminant = jedex_generic_union_get_discriminant;

    generic_union->instance_size = jedex_generic_union_instance_size;
    generic_union->init = jedex_generic_union_init;
    generic_union->done = jedex_generic_union_done;
  }

  return generic_union;
}


int
jedex_generic_union_get_next_branch( const jedex_value_iface *viface,
                                     const void *vself,
                                     size_t *index,
                                     jedex_value *branch ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  const jedex_generic_union *self = ( jedex_generic_union * ) vself;

  size_t start;

  if ( *index == UINT32_MAX ) {
    start = 0;
  }
  else {
    start = *index;
    start++;
  }
  
  for ( size_t i = start; i < iface->branch_count; i++ ) {
    if ( i == self->selected_branches[ i ] ) {
       branch->iface = &iface->branch_ifaces[ i ]->parent;
       branch->self = jedex_generic_union_branch( iface, self, i );
       *index = i;

       return 0;
    }
  }

  return EINVAL;
}


jedex_generic_value_iface *
jedex_generic_union_class( jedex_schema *schema, memoize_state *state )
{
  jedex_generic_union_value_iface *iface = ( jedex_generic_union_value_iface * ) jedex_new( jedex_generic_union_value_iface );
  if ( iface == NULL ) {
    return NULL;
  }

  memset( iface, 0, sizeof( jedex_generic_union_value_iface ) );
  memcpy( &iface->parent, generic_union_class(), sizeof( iface->parent ) );
  iface->schema = schema;

  iface->branch_count = jedex_schema_union_size( schema );
  size_t branch_offsets_size = sizeof( size_t ) * iface->branch_count;
  size_t branch_ifaces_size = sizeof( jedex_generic_value_iface * ) * iface->branch_count;

  iface->branch_ifaces = ( jedex_generic_value_iface ** ) jedex_malloc( branch_ifaces_size );
  if ( iface->branch_ifaces == NULL ) {
    goto error;
  }

  iface->branch_offsets = ( size_t * ) jedex_malloc( branch_offsets_size );
  if ( iface->branch_offsets == NULL ) {
    goto error;
  }

  size_t next_offset = sizeof( jedex_generic_union );
  size_t i;
  for ( i = 0; i < iface->branch_count; i++ ) {
    jedex_schema *branch_schema = jedex_schema_union_branch( schema, i );

    iface->branch_ifaces[ i ] = jedex_generic_class_from_schema_memoized( branch_schema, state );
    if ( iface->branch_ifaces[ i ] == NULL) {
      goto error;
    }

    iface->branch_offsets[ i ] = next_offset;
    size_t branch_size = jedex_value_instance_size( iface->branch_ifaces[ i ] );
    if ( branch_size == 0 ) {
      log_err( "Union branch class must provide instance_size" );
      goto error;
    }

    next_offset += branch_size;
  }

  iface->instance_size = next_offset;

  return &iface->parent;

error:
  if ( iface->branch_ifaces != NULL ) {
    jedex_free( iface->branch_ifaces );
  }
  jedex_free( iface );

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
