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


#ifndef SCHEMA_PRIV_H
#define SCHEMA_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "basics.h"
#include "schema.h"
#include "st.h"


struct jedex_record_field {
  int index;
  char *name;
  jedex_schema *type;
};  


struct jedex_record_schema {
  struct jedex_obj obj;
  char *name; 
  char *space;
  st_table *fields;
  st_table *fields_byname;
};


struct jedex_array_schema {
  struct jedex_obj obj;
  jedex_schema *items;
};


struct jedex_map_schema {
  struct jedex_obj obj;
  jedex_schema *values;
};



struct jedex_union_schema {
  struct jedex_obj obj;
  st_table *branches;
  st_table *branches_byname;
};


#define jedex_schema_to_record( schema_ ) ( container_of( schema_, struct jedex_record_schema, obj ) )
#define jedex_schema_to_array( schema_ ) ( container_of( schema_, struct jedex_array_schema, obj ) )
#define jedex_schema_to_map( schema_ ) ( container_of( schema_, struct jedex_map_schema, obj ) )
#define jedex_schema_to_union( schema_ ) ( container_of( schema_, struct jedex_union_schema, obj ) )


CLOSE_EXTERN
#endif // SCHEMA_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
