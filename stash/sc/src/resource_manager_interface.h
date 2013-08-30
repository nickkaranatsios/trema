/*
 * Resource mamager interface functions.
 *
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


#ifndef RESOURCE_MANAGER_INTERFACE_H
#define RESOURCE_MANAGER_INTERFACE_H


#include <inttypes.h>
#include "jedex_iface.h"
#include "service_controller.h"
#include "resource_manager_local_db.h"


extern void
physical_machine_information_notification_callback( jedex_value *val, const char *json,
                                                    void *user_data );
extern void
register_service_request_callback( jedex_value *val, const char *json,
                                   const char *client_id, void *user_data );
extern void
remove_service_request_callback( jedex_value *val, const char *json,
                                 const char *client_id, void *user_data );
extern void
virtual_machine_allocate_request_callback( jedex_value *val, const char *json,
                                           const char *client_id, void *user_data );
extern void
delete_service_request_callback( jedex_value *val, const char *json,
                                 const char *client_id, void *user_data );
extern void
virtual_machine_migrate_request_callback( jedex_value *val, const char *json,
                                          const char *client_id, void *user_data );
extern void
statistics_status_request_callback( jedex_value *val, const char *json,
                                    const char *client_id, void *user_data );


#endif // RESOURCE_MANAGER_INTERFACE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
