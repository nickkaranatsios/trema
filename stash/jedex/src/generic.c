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


jedex_generic_value_iface *
jedex_generic_class_from_schema_memoized( jedex_schema *schema, memoize_state *state ) {
  jedex_generic_value_iface *result = NULL;

  if ( jedex_memoize_get( &state->mem, schema, NULL, ( void ** ) &result ) ) {
    return result;
  }
  switch ( schema->type ) {
    case JEDEX_BOOLEAN:
      result = jedex_generic_boolean_class();
    break;
    case JEDEX_BYTES:
      result = jedex_generic_bytes_class();
    break;
    case JEDEX_DOUBLE:
      result = jedex_generic_double_class();
    break;
    case JEDEX_FLOAT:
     result = jedex_generic_float_class();
    break;
    case JEDEX_INT32:
      result = jedex_generic_int_class();
    break;
    case JEDEX_INT64:
     result = jedex_generic_long_class();
    break;
    case JEDEX_NULL:
      result = jedex_generic_null_class();
    break;
    case JEDEX_STRING:
      result = jedex_generic_string_class();
    break;
    case JEDEX_ARRAY:
      result = jedex_generic_array_class( schema, state );
    break;
    case JEDEX_RECORD:
      result = jedex_generic_record_class( schema, state );
    break;
    case JEDEX_UNION:
      result = jedex_generic_union_class( schema, state );
    break;
    case JEDEX_MAP:
      result = jedex_generic_map_class( schema, state );
    break;
    case JEDEX_LINK: {
      jedex_generic_link_value_iface *lresult = jedex_generic_link_class( schema );
      lresult->next = state->links;
      state->links = lresult;
      result = &lresult->parent;
    }
    break;
    default:
      log_err( "Unknown schema type" );
    break;
  }
  jedex_memoize_set( &state->mem, schema, NULL, result );

  return result;
}


jedex_value_iface *
jedex_generic_class_from_schema( jedex_schema *schema ) {
  memoize_state state;

  jedex_memoize_init( &state.mem );
  state.links = NULL;


  jedex_generic_value_iface *result = jedex_generic_class_from_schema_memoized( schema, &state );
  if (result == NULL) {
    jedex_memoize_done( &state.mem );
    return NULL;
  }

  while ( state.links != NULL ) {
    jedex_generic_link_value_iface *link_iface = state.links;
    jedex_schema *target_schema = jedex_schema_link_target( link_iface->schema );

    jedex_generic_value_iface *target_iface = NULL;
    if ( !jedex_memoize_get( &state.mem, target_schema, NULL, ( void ** ) &target_iface ) ) {
      log_err( "Never created a value implementation for %s", jedex_schema_type_name( target_schema ) );
      return NULL;
    }

    link_iface->target_giface = target_iface;
    state.links = link_iface->next;
    link_iface->next = NULL;
  }
  jedex_memoize_done( &state.mem );

  return &result->parent;
}


static void
jedex_refcount_inc( volatile int *refcount ) {
  if ( *refcount != ( int ) -1 ) {
    *refcount += 1;
  }
}


static int
jedex_refcount_dec( volatile int *refcount ) {
  if ( *refcount != ( int ) -1 ) {
    *refcount -= 1;
    return ( *refcount == 0 );
  }

  return 0;
}


static void
jedex_generic_value_free( const jedex_value_iface *iface, void *self ) {
  if ( self != NULL ) {
    const jedex_generic_value_iface *giface = container_of( iface, jedex_generic_value_iface, parent );
    jedex_value_done( giface, self );
    self = ( char * ) self - sizeof( volatile int );
    jedex_free( self );
  }
}


void
jedex_generic_value_incref( jedex_value *value ) {
  volatile int *refcount = ( volatile int * ) ( ( char * ) value->self - sizeof( volatile int ) );

  jedex_refcount_inc( refcount );
}


void
jedex_generic_value_decref( jedex_value *value ) {
  volatile int *refcount = ( volatile int * ) ( ( char * ) value->self - sizeof( volatile int ) );

  if ( jedex_refcount_dec( refcount ) ) {
    jedex_generic_value_free( value->iface, value->self );
  }
}


int
jedex_generic_value_new( jedex_value_iface *iface, jedex_value *dest ) {
  int rval;
  jedex_generic_value_iface *giface = container_non_const_of( iface, jedex_generic_value_iface, parent );

  size_t instance_size = jedex_value_instance_size( giface );
  void *self = jedex_malloc( instance_size + sizeof( volatile int ) );
  if ( self == NULL ) {
    log_err( "Failed to allocate a generic value %s", strerror( ENOMEM ) );
    dest->iface = NULL;
    dest->self = NULL;
    return ENOMEM;
  }

  volatile int *refcount = ( volatile int * ) self;
  self = ( char * ) self + sizeof( volatile int );
  *refcount = 1;
  rval = jedex_value_init( giface, self );
  if ( rval != 0 ) {
    jedex_free( self );
    dest->iface = NULL;
    dest->self = NULL;
    return rval;
  }

  dest->iface = &giface->parent;
  dest->self = self;

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
