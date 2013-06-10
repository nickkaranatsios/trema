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


#ifndef GENERIC_H
#define GENERIC_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <stdlib.h>

#include "schema.h"
#include "value.h"


/*
 * This file contains an jedex_value_t implementation that can store
 * values of any Jedex schema.
 */


/**
 * Return a generic jedex_value_iface_t implementation for the given
 * schema, regardless of what type it is.
 */

jedex_value_iface *jedex_generic_class_from_schema( jedex_schema schema );

/**
 * Allocate a new instance of the given generic value class.  @a iface
 * must have been created by @ref jedex_generic_class_from_schema.
 */

int jedex_generic_value_new( jedex_value_iface *iface, jedex_value *dest );


/*
 * These functions return an jedex_value_iface_t implementation for each
 * primitive schema type.  (For enum, fixed, and the compound types, you
 * must use the @ref jedex_generic_class_from_schema function.)
 */

jedex_value_iface *jedex_generic_boolean_class( void );
jedex_value_iface *jedex_generic_bytes_class( void );
jedex_value_iface *jedex_generic_double_class( void );
jedex_value_iface *jedex_generic_float_class( void );
jedex_value_iface *jedex_generic_int_class( void );
jedex_value_iface *jedex_generic_long_class( void );
jedex_value_iface *jedex_generic_null_class(void);
jedex_value_iface *jedex_generic_string_class(void);


/*
 * These functions instantiate a new generic primitive value.
 */

int jedex_generic_boolean_new( jedex_value *value, int val );
int jedex_generic_bytes_new( jedex_value *value, void *buf, size_t size );
int jedex_generic_double_new( jedex_value *value, double val );
int jedex_generic_float_new( jedex_value *value, float val );
int jedex_generic_int_new( jedex_value *value, int32_t val );
int jedex_generic_long_new( jedex_value *value, int64_t val );
int jedex_generic_null_new( jedex_value *value );
int jedex_generic_string_new(jedex_value *value, const char *val );
int jedex_generic_string_new_length( jedex_value *value, const char *val, size_t size );


CLOSE_EXTERN
#endif // GENERIC_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
