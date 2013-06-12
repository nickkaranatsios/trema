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


#ifndef GENERIC_INTERNAL_H
#define GENERIC_INTERNAL_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "generic.h"
#include "schema.h"
#include "value.h"
#include "data.h"


/*
 * Each generic value implementation struct defines a couple of extra
 * methods that we use to control the lifecycle of the value objects.
 */

typedef struct jedex_generic_value_iface {
	jedex_value_iface parent;

	/**
	 * Return the size of an instance of this value type.  If this
	 * returns 0, then this value type can't be used with any
	 * function or type (like jedex_value_new) that expects to
	 * allocate space for the value itself.
	 */
	size_t ( *instance_size ) ( const jedex_value_iface *iface );

	/**
	 * Initialize a new value instance.
	 */
	int ( *init ) ( const jedex_value_iface *iface, void *self );

	/**
	 * Finalize a value instance.
	 */
	void ( *done ) ( const jedex_value_iface *iface, void *self );
} jedex_generic_value_iface;


#define jedex_value_instance_size( gcls ) \
    ( ( gcls )->instance_size == NULL? 0: ( gcls )->instance_size( &( gcls )->parent ) )
#define jedex_value_init( gcls, self ) \
    ( ( gcls )->init == NULL? EINVAL: ( gcls )->init( &( gcls )->parent, ( self ) ) )
#define jedex_value_done( gcls, self ) \
    ( ( gcls )->done == NULL? ( void ) 0: ( gcls )->done( &( gcls )->parent, ( self ) ) )



typedef struct jedex_generic_array_value_iface {
	jedex_generic_value_iface parent;
	jedex_schema *schema;
	jedex_generic_value_iface  *child_giface;
} jedex_generic_array_value_iface;



typedef struct jedex_generic_array {
	jedex_raw_array array;
} jedex_generic_array;


typedef struct jedex_generic_union_value_iface {
  jedex_generic_value_iface parent;
  jedex_schema *schema;

  /** The total size of each value struct for this union. */
  size_t instance_size;

  /** The number of branches in this union.  Yes, we could get
   * this from schema, but this is easier. */
  size_t branch_count;

  /** The value implementation for each branch. */
  jedex_generic_value_iface **branch_ifaces;
} jedex_generic_union_value_iface;


typedef struct jedex_generic_union {
} jedex_generic_union;


#define jedex_generic_union_field(iface, rec, index) \
  (((char *) (rec)) + (iface)->branch_ifaces[(index)])
//#define jedex_generic_union_field( iface, rec, index ) \
  ( ( ( char * ) ( rec ) ) + ( iface )->branch_ifaces[ ( index ) ] )

/** Return a pointer to the active branch within a union struct. */
#define jedex_generic_union_branch( _union, index ) \
  ( ( ( char * ) ( _union ) ) + sizeof( jedex_generic_union ) * index )



typedef struct jedex_generic_map_value_iface {
  jedex_generic_value_iface parent;
  jedex_schema *schema;
  jedex_generic_value_iface *child_giface;
} jedex_generic_map_value_iface;


typedef struct jedex_generic_map {
  jedex_raw_map map;
} jedex_generic_map;


/*
 * For generic records, we need to store the value implementation for
 * each field.  We also need to store an offset for each field, since
 * we're going to store the contents of each field directly in the
 * record, rather than via pointers.
 */

typedef struct jedex_generic_record_value_iface {
  jedex_generic_value_iface parent;
  jedex_schema *schema;

  /** The total size of each value struct for this record. */
  size_t instance_size;

  /** The number of fields in this record.  Yes, we could get this
   * from schema, but this is easier. */
  size_t field_count;

  /** The offset of each field within the record struct. */
  size_t *field_offsets;

  /** The value implementation for each field. */
  jedex_generic_value_iface  **field_ifaces;
} jedex_generic_record_value_iface;


typedef struct jedex_generic_record {
  /* The rest of the struct is taken up by the inline storage
   * needed for each field. */
} jedex_generic_record;


/** Return a pointer to the given field within a record struct. */
#define jedex_generic_record_field( iface, rec, index ) \
  ( ( ( char * ) ( rec ) ) + ( iface )->field_offsets[ ( index ) ] )


jedex_generic_value_iface *jedex_generic_class_from_schema_memoized( jedex_schema *schema );
jedex_generic_value_iface *jedex_generic_boolean_class( void );
jedex_generic_value_iface *jedex_generic_bytes_class( void );
jedex_generic_value_iface *jedex_generic_double_class( void );
jedex_generic_value_iface *jedex_generic_float_class( void );
jedex_generic_value_iface *jedex_generic_int_class( void );
jedex_generic_value_iface *jedex_generic_long_class( void );
jedex_generic_value_iface *jedex_generic_string_class( void );
jedex_generic_value_iface *jedex_generic_null_class( void );
jedex_generic_value_iface *jedex_generic_array_class( jedex_schema *schema );
jedex_generic_value_iface *jedex_generic_record_class( jedex_schema *schema );
jedex_generic_value_iface *jedex_generic_map_class( jedex_schema *schema );


CLOSE_EXTERN
#endif // GENERIC_INTERNAL_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
