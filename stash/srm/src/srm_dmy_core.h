/*
 * System Resource Manager dummy core logic
 *
 * Config file data administrator
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */

#ifndef SRM_DMY_CORE_H
#define SRM_DMY_CORE_H

#include "linked_list.h"

/******************************************************************************
 * Include files
 ******************************************************************************/
uint8_t
compute_n_vms( const char* service_name, uint64_t bandwidth, uint64_t n_subscribers );

uint8_t
compute_vm_cpu( const char* service_name );

uint16_t
compute_vm_memory( const char* service_name );

list_element*
compute_vm_locations( uint8_t n_vms, uint8_t n_cpu, uint16_t total_memory );

uint32_t
compute_vm_relocation( uint32_t vm_ip_address );

void
update_link( uint64_t from_dpid, uint64_t to_dpid, double link_util );

void
update_pm( uint32_t pm_ip_address, uint8_t n_cpu, uint16_t total_memory );

void
update_vm( uint32_t vm_ip_address, uint32_t pm_location_address, uint8_t n_cpu, uint16_t total_memory, const char* service_name );


#endif // SRM_DMY_CORE_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
