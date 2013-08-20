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


void
service_module_registration_handler( const uint32_t tx_id,
                                     jedex_value *val,
                                     const char *json,
                                     void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
service_module_remove_handler( const uint32_t tx_id,
                               jedex_value *val,
                               const char *json,
                               void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
add_service_profile_handler( const uint32_t tx_id,
                             jedex_value *val,
                             const char *json,
                             void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
del_service_profile_handler( const uint32_t tx_id,
                             jedex_value *val,
                             const char *json,
                             void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
start_service_specific_nc_handler( const uint32_t tx_id,
                                   jedex_value *val,
                                   const char *json,
                                   void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( json );
  UNUSED( user_data );
}


void
oss_bss_add_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  UNUSED( user_data );
}


void
oss_bss_del_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( val );
  UNUSED( json );
  UNUSED( client_id );
  UNUSED( user_data );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
