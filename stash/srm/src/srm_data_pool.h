/*
 * System Resource Manager data pool
 *
 * Config file data administrator
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */

#ifndef SRM_DATA_POOL_H
#define SRM_DATA_POOL_H

/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "nc_srm_message.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define SRM_CONFIG_INI_PATH              "./system_resource_manager.ini"

#define SRM_CONFIG_PM_MAX_COUNT         5

typedef enum {
  SRM_CONFIG_SERVICE_INDEX_INTERNET = 0,
  SRM_CONFIG_SERVICE_INDEX_VIDEO,
  SRM_CONFIG_SERVICE_INDEX_PHONE,
  SRM_CONFIG_MAX_SERVICE_COUNT,
  SRM_CONFIG_SERVICE_INDEX_NOT_USED = 99
} SRM_CONFIG_SERVICE_INDEX;

/*----------------------------------------------------------------------------
 * Configuration
 *----------------------------------------------------------------------------*/
// Service Name
#define SERVICE_NAME_INTERNET           "Internet_Access_Service"
#define SERVICE_NAME_VIDEO              "Video_Service"
#define SERVICE_NAME_PHONE              "Phone_Service"

// Physical machine ip address
#define PM1_IP                          "172.16.4.2"
#define PM2_IP                          "172.16.4.3"
#define PM3_IP                          "172.16.4.4"
#define PM4_IP                          "172.16.4.5"
#define PM5_IP                          "172.16.4.6"

// group address for IGMP
#define IGMP_GROUP_ADDRESS_TOP          1
#define IGMP_GROUP_ADDRESS_END          10
#define IGMP_GROUP_ADDRESS              "224.1.1.%d"
#define IGMP_GROUP_MAX_COUNT            ((IGMP_GROUP_ADDRESS_END - IGMP_GROUP_ADDRESS_TOP) + 1)

// Virtual machine ip address
#define VM_IP_TOP                       100
#define VM_IP_END                       254 
#define VM_IP_ADDRESS                   "172.16.4.%d"
#define VM_MAX_COUNT                    ((VM_IP_END - VM_IP_TOP) + 1)

// Data plane port ip address
#define DATA_PLANE_PORT_IP              "11.0.0.%d"
// Data plane prot mac
#define DATA_PLANE_PORT_MAC             0x1803733e7a00

// for System Test
#define VM_IP_TOP_INET                  100
#define VM_IP_TOP_VIDEO                 200
#define VM_INDEX_TOP_INET               (VM_IP_TOP_INET - VM_IP_TOP)
#define VM_INDEX_TOP_VIDEO              (VM_IP_TOP_VIDEO - VM_IP_TOP)
#define VM_COUNT_INET_CASE1             1
#define VM_COUNT_INET_CASE2             2
#define VM_COUNT_INET_CASE3             35
#define VM_COUNT_INET_CASE_MAX          (5 * 10)
#define VM_COUNT_VIDEO_CASE1            1
#define VM_COUNT_VIDEO_CASE2            1
#define VM_COUNT_VIDEO_CASE3            (5 * 1)
#define CASE1_N_SUBSCRIBERS             50
#define CASE2_N_SUBSCRIBERS             100
#define CASE3_N_SUBSCRIBERS             15000
#define CASE_MAX_N_SUBSCRIBERS          20000     // Not-used

/******************************************************************************
 * Structs
 ******************************************************************************/
typedef struct {
  uint32_t        vm_ip_address;
  uint32_t        pm_ip_address;
  int             service_id;
  uint32_t        port_ip_address;
  uint64_t        port_mac_address;
  uint8_t         cpu_count;
  uint16_t        memory_size;
  uint64_t        user_count;
  uint32_t        group_address;
} vm_info_table;

typedef struct {
  double          band_width_f;
  uint64_t        band_width;
  uint64_t        n_subscribers;
} service_info_table;

typedef enum {
  SRM_STATE_INITIAL = 0,
  SRM_STATE_STARTED,
  SRM_STATE_VERSIONUP
} SRM_LAST_STATE;

/**
 * load initial configuration from file
 */
int
srm_load_configuration();

/**
 * Get IP address value form ip address string.
 */
uint32_t
get_ip_address_int( const char* ip_address_string );

/**
 * Get IP address string form ip address value.
 */
char*
get_ip_address_string( uint32_t ip_address_int );


const char*
get_service_name( int service_id );

int
get_service_id( const char* service_name );

/**
 * Get next VM IP address as a same pm_ip_address
 */
int
get_next_vm_ip_address( uint32_t pm_ip_address, uint32_t *vm_ip_address, int* pos );

/**
 * Get VM info 
 */
vm_info_table*
get_vm_info( uint32_t vm_ip_address );

/**
 * Get VM service name
 */
const char*
get_vm_service_name( uint32_t vm_ip_address );

/**
 * Get next VM Info as a same service
 */
int
get_next_vm_info( int service_id, vm_info_table *vm_info, int* pos );

/**
 * Get next empty VM Info as a same service
 */
vm_info_table*
get_next_empty_vm_info( int* pos );

/**
 * Get next PM IP address
 */
int
get_next_pm_ip_address( uint32_t *pm_ip_address, int* pos );

/**
 * Assign PM IP address to virtual machine
 */
int
assign_pm_ip_address_to_vm( uint32_t vm_ip_address, uint32_t pm_ip_address );

uint32_t
get_pm_machine_id( uint32_t pm_ip_address );

#endif // SRM_DATA_POOL_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

