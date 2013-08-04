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


#ifndef PARCEL_H
#define PARCEL_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "linked_list.h"
#include "value.h"


typedef struct jedex_parcel {
  const jedex_schema *schema;
  list_element *values_list;
} jedex_parcel;


jedex_parcel *jedex_parcel_create( jedex_schema *schema, const char *sub_schema_names[] );
jedex_value *jedex_parcel_value( const jedex_parcel *parcel, const char *schema_name );
void jedex_parcel_destroy( jedex_parcel **parcel );


CLOSE_EXTERN
#endif // PARCEL_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
