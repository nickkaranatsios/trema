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


#include "service_manager.h"
#include "service_manager_priv.h"


jedex_schema *
_sub_schema_find( const char *name, size_t tbl_size, jedex_schema *sub_schemas[] ) {
  for ( size_t i = 0; i < tbl_size; i++ ) {
    if ( !strcmp( name, jedex_schema_type_name( sub_schemas[ i ] ) ) ) {
      return sub_schemas[ i ];
    } 
  }

  return NULL;
}


jedex_value *
_jedex_value_find( const char *name, size_t tbl_size, jedex_value *rval[] ) {
  for ( size_t i = 0; i < tbl_size; i++ ) {
    if ( rval[ i ] != NULL ) {
      jedex_schema *rschema = jedex_value_get_schema( rval[ i ] );
       const char *sname = jedex_schema_type_name( rschema );
      if ( !strcmp( name, sname ) ) {
        return rval[ i ];
      }
    }
  }

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
