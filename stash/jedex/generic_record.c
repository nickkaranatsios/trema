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


#include <pthread.h>
#include "priv.h"


static pthread_key_t generic_record_key;
static phtread_once_t generic_record_key_once = PTHREAD_ONCE_INIT;


static int
make_generic_record_key( void ) {
  pthread_key_create( &generic_record_key, free );
}


static int
jedex_generic_record_reset( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  int rval;
  jedex_generic_record *self = ( jedex_generic_record * ) vself;
  size_t i;

  for ( i = 0; i < iface->field_count; i++ ) {
    jedex_value  value = {
      &iface->field_ifaces[ i ]->parent,
      jedex_generic_record_field( iface, self, i )
    };
    check( rval, jedex_value_reset( &value ) );
  }

  return 0;
}


static jedex_type
jedex_generic_record_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );
  UNUSED( vself );

  return JEDEX_RECORD;
}


static int
jedex_generic_record_get_size( const jedex_value_iface *viface,
                               const void *vself,
                               size_t *size ) {
  UNUSED( vself );
  const jedex_generic_record_value_iface  *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  if ( size != NULL ) {
    *size = iface->field_count;
  }

  return 0;
}


static int
jedex_generic_record_get_by_index( const jedex_value_iface *viface,
                                   const void *vself,
                                   size_t index,
                                   jedex_value *child,
                                   const char **name ) {
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  const jedex_generic_record *self = ( const jedex_generic_record * ) vself;
  if ( index >= iface->field_count ) {
    log_err( "Field index %zu out of range", index );
    return EINVAL;
  }
  child->iface = &iface->field_ifaces[ index ]->parent;
  child->self = jedex_generic_record_field( iface, self, index );

  /*
   * Grab the field name from the schema if asked for.
   */
  if ( name != NULL ) {
    jedex_schema schema = iface->schema;
    *name = jedex_schema_record_field_name( schema, index );
  }

  return 0;
}


static int
jedex_generic_record_get_by_name( const jedex_value_iface *viface,
                                  const void *vself,
                                  const char *name,
                                  jedex_value *child,
                                  size_t *index_out ) {
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  const jedex_generic_record *self = ( const jedex_generic_record * ) vself;

  jedex_schema_t schema = iface->schema;
  int index = jedex_schema_record_field_get_index( schema, name );
  if ( index < 0 ) {
    log_err( "Unknown record field %s", name );
    return EINVAL;
  }

  child->iface = &iface->field_ifaces[ index ]->parent;
  child->self = jedex_generic_record_field( iface, self, index);
  if ( index_out != NULL ) {
    *index_out = index;
  }

  return 0;
}


static jedex_schema
jedex_generic_record_get_schema( const jedex_value_iface_t *viface, const void *vself ) {
  UNUSED( vself );
  const jedex_generic_record_value_iface  *iface = container_of( viface, generic_record_value_iface, parent );

  return iface->schema;
}


static size_t
jedex_generic_record_instance_size( const jedex_value_iface *viface ) {
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );

  return iface->instance_size;
}


static int
jedex_generic_record_init( const jedex_value_iface *viface, void *vself ) {
  int rval;
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  jedex_generic_record_t *self = ( jedex_generic_record * ) vself;

  /* Initialize each field */
  size_t i;
  for ( i = 0; i < iface->field_count; i++ ) {
    check( rval, jedex_value_init( iface->field_ifaces[i], jedex_generic_record_field( iface, self, i ) ) );
  }

  return 0;
}


static void
jedex_generic_record_done( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_record_value_iface *iface = container_of( viface, jedex_generic_record_value_iface, parent );
  jedex_generic_record *self = ( jedex_generic_record * ) vself;
  size_t i;
  for ( i = 0; i < iface->field_count; i++ ) {
    jedex_value_done( iface->field_ifaces[ i ], jedex_generic_record_field( iface, self, i ) );
  }
}


static struct jedex_generic_record *
jedex_generic_record_get( void ) {
  pthread_once( &generic_record_key_once, make_generic_record_key );

  struct generic_record *record = ( struct generic_record * ) pthread_getspecific( generic_record_key ); 
  if ( record == NULL ) {
    record = ( struct generic_record * ) jedex_new( struct generic_record );
    pthread_setspecific( generic_record_key, record );
    memset( record->parent, 0, sizeof( record->parent ) );
    record->parent->reset = jedex_generic_record_reset;
    record->parent->get_type = jedex_generic_record_get_type;
    record->parent->get_schema = jedex_generic_record_get_schema;
    record->parent->get_size = jedex_generic_record_get_size;
    record->parent->get_by_index = jedex_generic_record_get_by_index;
    record->parent->get_by_name = jedex_generic_record_get_by_name;
    record->parent->instance_size = jedex_generic_record_instance_size;
    record->parent->init = jedex_generic_record_init;
    record->parent->done = jedex_generic_record_done;
  }

  return record;
}


static jedex_generic_value_iface *
jedex_generic_record_class( jedex_schema schema, memoize_state_t *state ) {
  jedex_generic_record_value_iface *iface = ( jedex_generic_record_value_iface * ) jedex_new( jedex_generic_record_value_iface );
  if ( iface == NULL ) {
    return NULL;
  }

  memset( iface, 0, sizeof( jedex_generic_record_value_iface ) );
  iface->parent = jedex_generic_record_get();
  iface->refcount = 1;
  iface->schema = schema;

  iface->field_count = jedex_schema_record_size( schema );
  size_t field_offsets_size = sizeof( size_t ) * iface->field_count;
  size_t field_ifaces_size = sizeof( jedex_generic_value_iface * ) * iface->field_count;

  iface->field_offsets = ( size_t * ) jedex_malloc( field_offsets_size );
  if ( iface->field_offsets == NULL ) {
    goto error;
  }

  iface->field_ifaces = ( jedex_generic_value_iface ** ) jedex_malloc( field_ifaces_size );
  if ( iface->field_ifaces == NULL ) {
    goto error;
  }

  size_t next_offset = sizeof( jedex_generic_record_t );
  size_t i;
  for ( i = 0; i < iface->field_count; i++ ) {
    jedex_schema field_schema = jedex_schema_record_field_get_by_index( schema, i );

    iface->field_offsets[ i ] = next_offset;

    iface->field_ifaces[ i ] = jedex_generic_class_from_schema_memoized( field_schema, state );
    if ( iface->field_ifaces[ i ] == NULL ) {
      goto error;
    }

    size_t field_size = jedex_value_instance_size( iface->field_ifaces[ i ] );
    if ( field_size == 0 ) {
      log_err( "Record field class must provide instance_size" );
      goto error;
    }
    next_offset += field_size;
  }

  iface->instance_size = next_offset;

  return &iface->parent;

error:
  if ( iface->field_offsets != NULL ) {
    jedex_free( iface->field_offsets, field_offsets_size );
  }
  if ( iface->field_ifaces != NULL ) {
    jedex_free( iface->field_ifaces );
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
