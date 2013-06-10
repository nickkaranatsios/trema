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
	jedex_generic_value_iface_t parent;
	jedex_schema schema;
	jedex_generic_value_iface  *child_giface;
} jedex_generic_array_value_iface;



typedef struct jedex_generic_array {
	jedex_raw_array array;
} jedex_generic_array;



CLOSE_EXTERN
#endif // GENERIC_INTERNAL_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
