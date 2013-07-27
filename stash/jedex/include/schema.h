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


#ifndef SCHEMA_H
#define SCHEMA_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "basics.h"


typedef struct jedex_obj jedex_schema;


const char *jedex_schema_type_name( jedex_schema *schema );

jedex_schema *jedex_schema_string( void );
jedex_schema *jedex_schema_bytes( void );
jedex_schema *jedex_schema_int( void );
jedex_schema *jedex_schema_long( void );
jedex_schema *jedex_schema_float( void );
jedex_schema *jedex_schema_double( void );
jedex_schema *jedex_schema_boolean( void );
jedex_schema *jedex_schema_null( void );


jedex_schema *jedex_schema_record( const char *name, const char *space );
jedex_schema *jedex_schema_record_field_get( jedex_schema *schema, const char *field_name );
const char *jedex_schema_record_field_name( jedex_schema *schema, int index );
int jedex_schema_record_field_get_index( jedex_schema *schema, const char *field_name );
bool jedex_schema_record_field_is_primary( jedex_schema *schema, const char *field_name );
jedex_schema *jedex_schema_record_field_get_by_index( jedex_schema *schema, int index );
size_t jedex_schema_record_size( jedex_schema *schema );


jedex_schema *jedex_schema_map( jedex_schema *values );
jedex_schema *jedex_schema_map_values( jedex_schema *map );


jedex_schema *jedex_schema_array( jedex_schema *items );
jedex_schema *jedex_schema_array_items( jedex_schema *array );

jedex_schema *jedex_schema_union( void );
size_t jedex_schema_union_size( jedex_schema *union_schema );
int jedex_schema_union_append( jedex_schema *union_schema, jedex_schema *schema );
int jedex_schema_union_branch_get_index( jedex_schema *schema, const char *branch_name );
jedex_schema *jedex_schema_union_branch( jedex_schema *union_schema, int branch_index );
jedex_schema *jedex_schema_union_branch_by_name( jedex_schema *union_schema, int *branch_index, const char *name );

jedex_schema *jedex_schema_link_target( jedex_schema *schema );

int jedex_schema_from_json( const char *jsontext, jedex_schema **schema );
int jedex_schema_from_json_length( const char *jsontext, size_t length, jedex_schema **schema );
jedex_schema *jedex_schema_get_subschema( jedex_schema *schema, const char *name );

#define jedex_schema_from_json_literal( json, schema ) \
  jedex_schema_from_json_length( ( json ), sizeof( ( json ) ) - 1, ( schema ) )


CLOSE_EXTERN
#endif // SCHEMA_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
