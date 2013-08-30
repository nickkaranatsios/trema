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


#include "sc_resource_manager.h"


void
vm_allocate_handler( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  UNUSED( user_data );
}


void
service_module_reg_handler( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  UNUSED( user_data );
}


void
pm_info_handler( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );

  if ( val && val->iface ) {
    jedex_value field;
    jedex_value_get_by_name( val, "ip_address", &field, NULL );
    int tmp;
    jedex_value_get_int( &field, &tmp );
    uint32_t ip_address = ( uint32_t ) tmp;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
