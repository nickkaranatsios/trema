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
#include "nc_srm_message.h"
#include "srm_data_pool.h"

/******************************************************************************
 * Global
 ******************************************************************************/
/*----------------------------------------------------------------------------
 * Configuration
 *----------------------------------------------------------------------------*/
// Physical machine IP address list
uint32_t                        pm_ip_address_list[SRM_CONFIG_PM_MAX_COUNT];
uint32_t                        pm_machine_id_list[SRM_CONFIG_PM_MAX_COUNT];

// Virtual machine info
vm_info_table                   vm_info_list[ VM_MAX_COUNT ];

// group address for IGMP
uint32_t                        group_address_list[ IGMP_GROUP_MAX_COUNT ];

/******************************************************************************
 * Functions
 ******************************************************************************/
/**
 * Get IP address value form ip address string.
 */
uint32_t
ip_address_to_i( const char *ip_address_s ) {
  struct in_addr in_addr;
  uint32_t ip_address;
  
  if ( inet_aton( ip_address_s, &in_addr ) != 0 ) {
    ip_address = ntohl( in_addr.s_addr );
  }
  else {
    ip_address = 0;
  }
  
  return ip_address;
}


/**
 * Get IP address string form ip address value.
 */
char*
ip_address_to_s( uint32_t ip_address_i ) {
  struct in_addr in_addr;
  
  in_addr.s_addr = htonl( ip_address_i );
  
  return inet_ntoa( in_addr );
}


const char*
get_service_name( int service_id ) {
  switch ( service_id ) {
    case SRM_CONFIG_SERVICE_INDEX_INTERNET :
      return SERVICE_NAME_INTERNET;
    case SRM_CONFIG_SERVICE_INDEX_VIDEO :
      return SERVICE_NAME_VIDEO;
    case SRM_CONFIG_SERVICE_INDEX_PHONE :
      return SERVICE_NAME_PHONE;
    case SRM_CONFIG_SERVICE_INDEX_NOT_USED :
      return "Not-Used";
    default :
      return "";
  }
}

int
get_service_id( const char* service_name ) {
  arg_assert( service_name );
  
  if      ( strcmp( service_name, SERVICE_NAME_INTERNET ) == 0 ) {
    return SRM_CONFIG_SERVICE_INDEX_INTERNET;
  }
  else if ( strcmp( service_name, SERVICE_NAME_VIDEO ) == 0 ) {
    return SRM_CONFIG_SERVICE_INDEX_VIDEO;
  }
  else if ( strcmp( service_name, SERVICE_NAME_PHONE ) == 0 ) {
    return SRM_CONFIG_SERVICE_INDEX_PHONE;
  }
  else {
    arg_assert( 0 );
  }
}

/**
 * load initial configuration from file
 */
int
srm_load_configuration() {
  int ii;
  char ip_str[32];
  char group_adr[32];
  char port_ip[32];
  
  log_debug("srm_load_configuration: start");
  
  //
  // TODO: load config file
  //
  
  pm_ip_address_list[0] = get_ip_address_int( PM1_IP );
  pm_ip_address_list[1] = get_ip_address_int( PM2_IP );
  pm_ip_address_list[2] = get_ip_address_int( PM3_IP );
  pm_ip_address_list[3] = get_ip_address_int( PM4_IP );
  pm_ip_address_list[4] = get_ip_address_int( PM5_IP );

  pm_machine_id_list[0] = 0xcd0;
  pm_machine_id_list[1] = 0xcd1;
  pm_machine_id_list[2] = 0xcd2;
  pm_machine_id_list[3] = 0xcd3;
  pm_machine_id_list[4] = 0xcd4;
  
  // IGP Group address
  for ( ii = 0; ii < IGMP_GROUP_MAX_COUNT; ii++ ) {
    sprintf( group_adr, IGMP_GROUP_ADDRESS, ii + IGMP_GROUP_ADDRESS_TOP );
    group_address_list[ ii ] = get_ip_address_int( group_adr );
  }
  
  for ( ii = 0; ii < VM_MAX_COUNT; ii++ ) {
    // VM IP address
    sprintf( ip_str, VM_IP_ADDRESS, ii + VM_IP_TOP );
    vm_info_list[ ii ].vm_ip_address = get_ip_address_int( ip_str );
    
    // PM IP address
    //vm_info_list[ ii ].pm_ip_address = pm_ip_address_list[ ii % SRM_CONFIG_PM_MAX_COUNT ];
    vm_info_list[ ii ].pm_ip_address = 0;
    
    // Service Name
    /*
    switch ( ii % 2 ) {
      case 0  : vm_info_list[ ii ].service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;  break;
      case 1  : vm_info_list[ ii ].service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;     break;
      default : break;
    }
    */
    vm_info_list[ ii ].service_id = SRM_CONFIG_SERVICE_INDEX_NOT_USED;
    
    // IGP Group address
    vm_info_list[ ii ].group_address = group_address_list[ ii % IGMP_GROUP_MAX_COUNT ];
    
    // Other Parameters
    sprintf( port_ip, DATA_PLANE_PORT_IP, ii + VM_IP_TOP );
    vm_info_list[ ii ].port_ip_address  = get_ip_address_int( port_ip );
    vm_info_list[ ii ].port_mac_address = DATA_PLANE_PORT_MAC | (uint8_t)(ii & 0xFF);
    vm_info_list[ ii ].cpu_count        = 1;
    vm_info_list[ ii ].memory_size      = 1024;
    vm_info_list[ ii ].user_count       = 100;
    
    log_debug("Add Service             :%s", get_service_name( vm_info_list[ ii ].service_id ) );
    log_debug("Add VM-IP-address       :%s(%u)", get_ip_address_string( vm_info_list[ ii ].vm_ip_address ), vm_info_list[ ii ].vm_ip_address );
    log_debug("Add PM-IP-address       :%s(%u)", get_ip_address_string( vm_info_list[ ii ].pm_ip_address ), vm_info_list[ ii ].pm_ip_address );
    log_debug("Add Group-address       :%s(%u)", get_ip_address_string( vm_info_list[ ii ].group_address ), vm_info_list[ ii ].group_address );
    log_debug("Add Data plane port ip  :%s(%u)", get_ip_address_string( vm_info_list[ ii ].port_ip_address ), vm_info_list[ ii ].port_ip_address );
    log_debug("Add Data plane port mac :0x%llx", vm_info_list[ ii ].port_mac_address );
  }
  
  log_debug("srm_load_configuration: end");
  return 0;
}

/**
 * Get next VM IP address as a same pm_ip_address
 */
int
get_next_vm_ip_address( uint32_t pm_ip_address, uint32_t *vm_ip_address, int* pos ) {
  int ii;
  
  arg_assert( vm_ip_address );
  arg_assert( pos );
  arg_assert( *pos >= 0 );
  
  *vm_ip_address = 0;
  
  if ( *pos >= VM_MAX_COUNT ) {
    return 0;
  }
  for( ii = *pos; ii < VM_MAX_COUNT; ii++, (*pos)++ ) {
    if ( vm_info_list[ ii ].pm_ip_address == pm_ip_address ) {
      *vm_ip_address = vm_info_list[ ii ].vm_ip_address;
      (*pos)++;
      return 1;
    }
  }
  return 0;
}

/**
 * Get VM info 
 */
vm_info_table*
get_vm_info( uint32_t vm_ip_address ) {
  int ii;
  for( ii = 0; ii < VM_MAX_COUNT; ii++ ) {
    if ( vm_info_list[ ii ].vm_ip_address == vm_ip_address ) {
      return &vm_info_list[ ii ];
    }
  }
  
  log_err("Not-found virtual-machine. ip_address='%s'", get_ip_address_string( vm_ip_address ) );
  return NULL;
}

/**
 * Get VM service name
 */
const char*
get_vm_service_name( uint32_t vm_ip_address ) {
  int ii;
  for( ii = 0; ii < VM_MAX_COUNT; ii++ ) {
    if ( vm_info_list[ ii ].vm_ip_address == vm_ip_address ) {
      return get_service_name( vm_info_list[ ii ].service_id );
    }
  }
  
  log_err("Not-found virtual-machine. ip_address='%s'", get_ip_address_string( vm_ip_address ) );
  return "";
}

/**
 * Get next VM Info as a same service
 */
int
get_next_vm_info( int service_id, vm_info_table *vm_info, int* pos ) {
  int ii;
  
  arg_assert( vm_info );
  arg_assert( pos );
  arg_assert( *pos >= 0 );
  
  memset( vm_info, 0, sizeof(vm_info_table) );
  
  if ( *pos >= VM_MAX_COUNT ) {
    return 0;
  }
  for( ii = *pos; ii < VM_MAX_COUNT; ii++, (*pos)++ ) {
    if ( vm_info_list[ ii ].service_id == service_id ) {
      *vm_info = vm_info_list[ ii ];
      (*pos)++;
      return 1;
    }
  }
  return 0;
}


/**
 * Get next empty VM Info as a same service
 */
vm_info_table*
get_next_empty_vm_info( int* pos ) {
  int ii;
  arg_assert( pos );
  if ( *pos >= VM_MAX_COUNT ) {
    return NULL;
  }
  for( ii = *pos; ii < VM_MAX_COUNT; ii++, (*pos)++ ) {
    if ( vm_info_list[ ii ].service_id == SRM_CONFIG_SERVICE_INDEX_NOT_USED ) {
      (*pos)++;
      return &vm_info_list[ ii ];
    }
  }
  return NULL;
}


/**
 * Get next PM IP address
 */
int
get_next_pm_ip_address( uint32_t *pm_ip_address, int* pos ) {
  
  arg_assert( pm_ip_address );
  arg_assert( pos );
  arg_assert( *pos >= 0 );
  
  if ( *pos >= SRM_CONFIG_PM_MAX_COUNT ) {
    *pm_ip_address = 0;
    return 0;
  }
  
  *pm_ip_address = pm_ip_address_list[ *pos ];
  (*pos)++;
  return 1;
}

/**
 * Assign PM IP address to virtual machine
 */
int
assign_pm_ip_address_to_vm( uint32_t vm_ip_address, uint32_t pm_ip_address ) {
  int ii;
  for( ii = 0; ii < VM_MAX_COUNT; ii++ ) {
    if ( vm_info_list[ ii ].vm_ip_address == vm_ip_address ) {
      vm_info_list[ ii ].pm_ip_address = pm_ip_address;
      return 0;
    }
  }
  
  log_err("Not-found virtual-machine. ip_address='%s'", get_ip_address_string( vm_ip_address ) );
  return -1;
}

uint32_t
get_pm_machine_id( uint32_t pm_ip_address ) {
  int ii;
  for ( ii = 0; ii < SRM_CONFIG_PM_MAX_COUNT; ii++ ) {
    if ( pm_ip_address_list[ ii ] == pm_ip_address ) {
        return pm_machine_id_list[ ii ];
    }
  }
  log_err("Not-found pm_ip_address(%lu)", pm_ip_address );
  return 0x0;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

