/*
 * Service Manager data pool
 *
 * Config file data administrator
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */


/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "arg_assert.h"
#include "log_writer.h"
#include "srm_data_pool.h"
#include "srm_dmy_core.h"

static int top_pm_ip_index;

/******************************************************************************
 * Functions
 ******************************************************************************/
uint8_t
compute_n_vms( const char* service_name, uint64_t bandwidth, uint64_t n_subscribers ) {
  uint8_t       n_vms;
  vm_info_table vm_info;
  int           pos;
  int           service_id;
  
  top_pm_ip_index = 0;
  
  // Internet Access Service
  if (      strcmp( service_name, SERVICE_NAME_INTERNET ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;
    if ( n_subscribers <= CASE1_N_SUBSCRIBERS ) {
      n_vms = VM_COUNT_INET_CASE1;
    }
    else if ( n_subscribers <= CASE2_N_SUBSCRIBERS ) {
      n_vms = VM_COUNT_INET_CASE2;
    }
    else if ( n_subscribers <= CASE3_N_SUBSCRIBERS ) {
      n_vms = VM_COUNT_INET_CASE3;
    }
    else {
      n_vms = VM_COUNT_INET_CASE_MAX;
    }
    return n_vms;
  }
  // Video Service
  else if ( strcmp( service_name, SERVICE_NAME_VIDEO    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;
    if ( n_subscribers <= CASE1_N_SUBSCRIBERS ) {
      n_vms = VM_COUNT_VIDEO_CASE1;
    }
    else if ( n_subscribers <= CASE2_N_SUBSCRIBERS ) {
      n_vms = VM_COUNT_VIDEO_CASE2;
      top_pm_ip_index = 2;  // PM3
    }
    else {
      n_vms = VM_COUNT_VIDEO_CASE3;
    }
    return n_vms;
  }
  // Phone Service
  else if ( strcmp( service_name, SERVICE_NAME_PHONE    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_PHONE;
  }
  // Other Service
  else {
    log_err("Unknown Service :(%s)", service_name );
    return -1;
  }
  
  n_vms = 1;
  pos   = 0;
  while( get_next_vm_info( service_id, &vm_info, &pos ) ) {
    n_vms++;
  }
  
  return n_vms;
}

uint8_t
compute_vm_cpu( const char* service_name ) {
  uint8_t       n_cpu;
  vm_info_table vm_info;
  int           pos;
  int           service_id;
  
  // Internet Access Service
  if (      strcmp( service_name, SERVICE_NAME_INTERNET ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;
  }
  // Video Service
  else if ( strcmp( service_name, SERVICE_NAME_VIDEO    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;
  }
  // Phone Service
  else if ( strcmp( service_name, SERVICE_NAME_PHONE    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_PHONE;
  }
  // Other Service
  else {
    log_err("Unknown Service :(%s)", service_name );
    return -1;
  }
  
  n_cpu = 1;
  pos   = 0;
  while( get_next_vm_info( service_id, &vm_info, &pos ) ) {
    n_cpu += vm_info.cpu_count;
  }
  
  return n_cpu;
}

uint16_t
compute_vm_memory( const char* service_name ) {
  uint16_t      memory;
  vm_info_table vm_info;
  int           pos;
  int           service_id;
  
  // Internet Access Service
  if (      strcmp( service_name, SERVICE_NAME_INTERNET ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;
  }
  // Video Service
  else if ( strcmp( service_name, SERVICE_NAME_VIDEO    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;
  }
  // Phone Service
  else if ( strcmp( service_name, SERVICE_NAME_PHONE    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_PHONE;
  }
  // Other Service
  else {
    log_err("Unknown Service :(%s)", service_name );
    return -1;
  }
  
  memory = 1024;
  pos   = 0;
  while( get_next_vm_info( service_id, &vm_info, &pos ) ) {
    memory += vm_info.memory_size;
  }
  
  return memory;
}

list_element*
compute_vm_locations( uint8_t n_vms, uint8_t n_cpu, uint16_t total_memory ) {
  int ii;
  static uint32_t      dummy_ip_list[ VM_MAX_COUNT ];
  static list_element  ip_list[ VM_MAX_COUNT ];
  
  log_debug("compute_vm_locations(n_vms=%d, n_cpu=%d, total_memory=%d)",
    n_vms, n_cpu, total_memory);

  for ( ii = 0; ii < n_vms; ii++ ) {
    uint32_t pm_ip;
    switch ( (ii + top_pm_ip_index) % 5 ) {
      case 0 : pm_ip = get_ip_address_int(PM1_IP);  break;
      case 1 : pm_ip = get_ip_address_int(PM2_IP);  break;
      case 2 : pm_ip = get_ip_address_int(PM3_IP);  break;
      case 3 : pm_ip = get_ip_address_int(PM4_IP);  break;
      case 4 : pm_ip = get_ip_address_int(PM5_IP);  break;
      default: pm_ip = get_ip_address_int(PM1_IP);  break;
    }
    dummy_ip_list[ ii ] = pm_ip;
    ip_list[ ii ].data = &dummy_ip_list[ ii ];
    ip_list[ ii ].next = &ip_list[ (ii + 1) ];
  }
  ip_list[ (n_vms - 1) ].next = NULL;
  
  return ip_list;
}

uint32_t
compute_vm_relocation( uint32_t vm_ip_address ) {
  uint32_t pm_ip_address;
  pm_ip_address = get_ip_address_int(PM5_IP);
  log_debug("compute_vm_relocation(vm_ip_address=%s) return %s", get_ip_address_string(vm_ip_address), PM5_IP );
  return pm_ip_address;
}

void
update_link( uint64_t from_dpid, uint64_t to_dpid, double link_util ) {
  log_debug("update_link(from_dpid=%llu, to_dpid=%llu, link_util=%lf)",
    from_dpid, to_dpid, link_util );
}


void
update_pm( uint32_t pm_ip_address, uint8_t n_cpu, uint16_t total_memory ) {
  log_debug("update_pm(pm_ip_address=%s, n_cpu=%d, total_memory=%d)",
    get_ip_address_string(pm_ip_address), n_cpu, total_memory );
}

void
update_vm( uint32_t vm_ip_address, uint32_t pm_location_address, uint8_t n_cpu, uint16_t total_memory, const char* service_name ) {
  log_debug("update_pm(vm_ip_address=%s, pm_location_address=%s, n_cpu=%d, total_memory=%d)",
    get_ip_address_string(vm_ip_address), get_ip_address_string(pm_location_address), n_cpu, total_memory );
}



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

