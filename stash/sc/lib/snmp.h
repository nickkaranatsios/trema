/*
 * Physical machine control snmp functions.
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


#ifndef SNMP_H
#define SNMP_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include "service_controller.h"


void initialize_snmp( void );
void finalize_snmp( void );
int get_port_statuses( uint32_t ip_address, list_element *ports );
int get_cpu_statuses( uint32_t ip_address, list_element *cpus );
int get_memory_status( uint32_t ip_address, memory_status *memory );
int get_service_module_statuses( uint32_t ip_address, list_element *service_module_statuses );


CLOSE_EXTERN
#endif // SNMP_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
