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
jedex_generic_class_from_schema_memoized( jedex_schema *schema ) {
  jedex_generic_value_iface *result = NULL;

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
      result = jedex_generic_array_class( schema );
    break;
    case JEDEX_RECORD:
      result = jedex_generic_record_class( schema );
    break;
    case JEDEX_UNION:
      result = jedex_generic_union_class( schema );
    break;
    case JEDEX_MAP:
      result = jedex_generic_map_class( schema );
    break;
    default:
      log_err( "Unknown schema type" );
    break;
  }

  return result;
}


jedex_value_iface *
jedex_generic_class_from_schema( jedex_schema *schema ) {
  jedex_generic_value_iface *result = jedex_generic_class_from_schema_memoized( schema );

  return &result->parent;
}


int
jedex_generic_value_new( jedex_value_iface *iface, jedex_value *dest ) {
  int rval;
  jedex_generic_value_iface *giface = container_of( iface, jedex_generic_value_iface, parent );

  size_t instance_size = jedex_value_instance_size( giface );
  void *self = jedex_malloc( instance_size );
  if ( self == NULL ) {
    log_err( "Failed to allocate a generic value %s", strerror( ENOMEM ) );
    dest->iface = NULL;
    dest->self = NULL;
    return ENOMEM;
  }

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
