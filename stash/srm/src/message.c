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


#include "system_resource_manager.h"
#include "system_resource_manager_priv.h"


void
dispatch_service_to_sc( int operation, jedex_value *val, system_resource_manager *self ) {
  if ( val & val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *service;
    size_t size;
    jedex_value_get_string( &field, &service, &size );

    double bandwidth;
    jedex_value_get_by_name( val, "band_width", &field, &index ); 
    jedex_value_get_double( &field, &bandwidth );

    uint64_t subscribers;
    jedex_value_get_by_name( val, "subscribers", &field, &index );
    jedex_value_get_long( &field, &subscribers );
  }
}


void
oss_bss_add_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( json );
  UNUSED( client_id );
  system_resource_manager *self = user_data;
  dispatch_service_to_sc( ADD, val, self );
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
