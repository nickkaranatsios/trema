/*
 * Service Controller IF
 *
 * Handle Service Controller transmission
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zmq.h>
#include <errno.h>
#include <assert.h>
#include "log_writer.h"
#include "module_ports.h"
#include "ipc.h"
#include "srm_data_pool.h"
#include "system_resource_manager_interface.h"
#include "srm_dmy_core.h"                   // TODO:
#include "sc_transmission.h"
#include "create_json_file.h"
#include "ob_transmission.h"

static int SC_requester_id = -1;
static int SC_publisher_id = -1;
static pthread_t  polling_thread;
static pthread_mutex_t requester_mutex;

/**
 * lock requester
 */
static void
lock()
{
  pthread_mutex_lock(&requester_mutex);
}

/**
 * lock requester
 */
static void
unlock()
{
  pthread_mutex_unlock(&requester_mutex);
}

/**
 * check repily type
 */
static int
check_repily_type( int req_type, int rep_type ) {
  int return_value = -1;
  switch ( req_type ) {
    case SRM_VIRTUAL_MACHINE_ALLOCATE_REQUEST :
      if ( rep_type == SRM_VIRTUAL_MACHINE_ALLOCATE_REPLY ) {
        return_value = 0; // OK
      }
      break;
    case SRM_DELETE_SERVICE_REQUEST :
      if ( rep_type == SRM_DELETE_SERVICE_REPLY ) {
        return_value = 0; // OK
      }
      break;
    case SRM_VIRTUAL_MACHINE_MIGRATE_REQUEST :
      if ( rep_type == SRM_VIRTUAL_MACHINE_MIGRATE_REPLY ) {
        return_value = 0; // OK
      }
      break;
    case SRM_STATISTICS_STATUS_REQUEST :
      if ( rep_type == SRM_STATISTICS_STATUS_REPLY ) {
        return_value = 0; // OK
      }
      break;
    default :
      // NG
      log_err("Unknown request type. (%d)", req_type );
      break;
  }
  return return_value;
}


/**
 * received statistics_status processing.
 */
static int
loc_received_statistics( srm_statistics_status_reply* info ) {
  int pm_index, vm_index, result;
  physical_machine_stats *pm_info;
  virtual_machine_stats  *vm_info;
  uint32_t pm_ip_address;
  uint32_t vm_ip_address;
  uint32_t cpu_count;
  uint32_t total_memory;
  const char* vm_service;
  vm_info_table *my_vm_info;
  
  
  arg_assert( info );
  
  if ( info->result != 0 ) {
    log_err("service controller srm_statistics_status_reply error. (%d)", info->result );
    return -1;
  }
  
  
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  for ( pm_index = 0; pm_index < info->physical_machine_count; pm_index++ ) {
    pm_info = &info->physical_machine_stats[ pm_index ];
    pm_ip_address = pm_info->ip_address;
    cpu_count     = pm_info->cpu_count;
    total_memory  = pm_info->total_memory;
    update_pm( pm_ip_address, cpu_count, total_memory );
    log_debug("pm_ip_address=(%s) cpu_count=%d total_memory=%d vms=%d", 
      get_ip_address_string( pm_ip_address ), cpu_count, total_memory, pm_info->virtual_machine_count );
    
    for ( vm_index = 0; vm_index < pm_info->virtual_machine_count; vm_index++ ) {
      vm_info = &pm_info->virtual_machine_stats[ vm_index ];
      vm_ip_address = vm_info->ip_address;
      cpu_count     = vm_info->cpu_count;
      total_memory  = vm_info->total_memory;
      vm_service    = get_vm_service_name( vm_ip_address );
      update_vm(  vm_ip_address, pm_ip_address, cpu_count, total_memory, vm_service );
      log_debug("    vm_ip_address=(%s) cpu_count=%d total_memory=%d service=%s", 
        get_ip_address_string( vm_ip_address ), cpu_count, total_memory, vm_service );
      
      // Makeshift
      if ( pm_ip_address == 0 ) {
        if( ( my_vm_info = get_vm_info( vm_ip_address ) ) != NULL ) {
          pm_info->ip_address = my_vm_info->pm_ip_address;
          strcpy( vm_info->service_stats[0].service_name, vm_service );
        }
      }
    }
  }
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  
  
  if ( srm_last_state == SRM_STATE_VERSIONUP ) {
    result = parse_physical_machine_stats_for_versionup_server( 
            srm_last_versionup_server,
            (const char*)info );
  }
  else {
    result = parse_physical_machine_stats_for_add_service( 
            get_service_name( srm_last_add_service ),
            srm_service_info[ srm_last_add_service ].band_width_f,
            srm_service_info[ srm_last_add_service ].n_subscribers,
            (const char*)info );
  }
  
  if ( result != 0 ) {
    log_err("json file creation failed. (sc_statistics_status)");
    return -1;
  }
  
  return 0;
}


/**
 * send request
 *  statistics_status
 */
static int
loc_send_polling_request( header* req_msg ) {
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  int   req_type;
  int   rep_type;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( req_msg );
  req_type = req_msg->type;
  
  lock();
  result = request_message( SC_requester_id, (const char*)req_msg, req_msg->length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Send request error. result=(%d)", result );
    unlock();
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd  = (common_reply*)rep_msg;
  rep_type = rep_cmd->hdr.type;
  if( check_repily_type( req_type, rep_type ) == 0 ) {
    log_info("Received reply. type=(%d) length=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length );
    
    result = -1;
    switch ( rep_type ) {
      case SRM_STATISTICS_STATUS_REPLY :
        result = loc_received_statistics( (srm_statistics_status_reply*)rep_msg );
        break;
      default :
        log_crit("Unknown reply.");
        break;
    }
    if ( result != 0 ) {
      log_err("received reply processing failure.");
    }
  }
  else {
    log_err("Unknown reply. type=(%d) length=(%d) result=(%d)",
              rep_type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  free( rep_msg );
  unlock();
  return return_value;
}


/**
 * polling thread main routine.
 */
static void*
sc_pooling_thread_routine( void* thread_argument ) {
  srm_statistics_status_request req_msg;
  int result;
  
  req_msg.hdr.length = sizeof(req_msg);
  
  while ( 1 ) {
    
/* #if 0 */
    req_msg.hdr.type   = SRM_STATISTICS_STATUS_REQUEST;
    result = loc_send_polling_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("nc_statistics_request polling error. (%d)", result);
    }
/* endif */
    
    sleep( SRM_SC_POLLING_CYCLE );
  }
  
  pthread_exit(NULL);
}

/**
 * Create a polling thread.
 */
static int
create_polling_thread() {
  int result;
  
  // pthread attribute setting.
  pthread_attr_t attr;
  result = pthread_attr_init( &attr );
  if ( result != 0 ) {
    log_err("pthread_attr_init() failed. (%s)", strerror(errno) );
    return -1;
  }
  // Create the thread.
  result = pthread_create( &polling_thread, &attr, sc_pooling_thread_routine, NULL );
  if ( result != 0 ){
    log_err("pthread_create() failed. (%s)", strerror(errno) );
    return -1;
  }
  
  return 0;
}


/**
 * Send physical_machine_info_notice.
 */
static int
loc_send_physical_machine_info_notice() {
  int ret;
  int pos;
  uint32_t pm_ip_address = 0;
  sm_physical_machine_info_notice pub_msg;
  
  pub_msg.hdr.type    = SM_PHYSICAL_MACHINE_INFOMATION_NOTICE;
  pub_msg.hdr.length  = sizeof(pub_msg);
  pos = 0;
  while ( get_next_pm_ip_address( &pm_ip_address, &pos ) ) {
    pub_msg.ip_address = pm_ip_address;
    log_info("Send physical_machine_info_notice. ip-address='%s'", get_ip_address_string( pm_ip_address ) );
    ret = publish_message( SC_publisher_id, (const char*)&pub_msg, pub_msg.hdr.length );
    if( ret != 0 ) {
      log_err("Send physical_machine_info_notice error. (%d)", ret );
      // continue
    }
  }
  return 0;
}

/**
 * Initialize api information
 */
int
srm_init_sc_transmission() {
  int ret;

  log_debug("\n_/_/_/_/_/_/_/ SC INITIALIZATION START _/_/_/_/_/_/_/_/_/_/_/");
  // mutex initialize
  pthread_mutex_init( &requester_mutex, NULL );
  
  ret = create_publisher_server( SRM_SC2 );
  if ( ret < 0 ) {
    log_crit("unable to add requester [Service Controller]");
    return -1;
  }
  SC_publisher_id = ret;
  
  // wait
  sleep(1);
  
  ret = loc_send_physical_machine_info_notice();
  if ( ret != 0 ) {
    log_crit("send error. physical_machine_info_notice [Service Controller]");
    return -1;
  }
  
  ret = create_requester_ex( LOCAL_IP_ADDRESS, SRM_SC, 600000 );
  if ( ret < 0 ) {
    log_crit("unable to add requester [Service Controller]");
    return -1;
  }
  SC_requester_id = ret;
  
  // wait
  sleep(1);
  
  // create polling thread;
  ret = create_polling_thread();
  if ( ret < 0 ) {
    log_crit("unable to create polling thread [Service Controller]");
    return -1;
  }
  
  log_debug("_/_/_/_/_/_/_/ SC INITIALIZATION COMPLETE _/_/_/_/_/_/_/_/_/_/_/\n");
  return 0;
}


/**
 * Destroy allocated memories
 */
void
srm_close_sc_transmission() {
  log_debug("\n_/_/_/_/_/_/_/ SC CLOSE START    _/_/_/_/_/_/_/_/_/_/");
  if ( SC_requester_id != -1 ) {
    destroy_requester( SC_requester_id );
  }
  if ( SC_publisher_id != -1 ) {
    destroy_publisher( SC_publisher_id );
  }
  if ( pthread_cancel( polling_thread ) != 0 ) {
    log_err("pthread_cancel() failed. (%s)", strerror(errno) );
  }
  if ( pthread_join( polling_thread, NULL ) != 0 ) {
    log_err("pthread_join() failed. (%s)", strerror(errno) );
  }
  log_debug("_/_/_/_/_/_/_/ SC CLOSE COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
}


static int
loc_send_request( header* req_msg ) {
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  int   req_type;
  int   rep_type;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( req_msg );
  
  lock();
  result = request_message( SC_requester_id, (const char*)req_msg, req_msg->length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Send request error. result=(%d)", result );
    unlock();
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  rep_type = rep_cmd->hdr.type;
  req_type = req_msg->type;
  if( check_repily_type( req_type, rep_type ) == 0 ) {
    log_info("Received reply. type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    // OK
    if ( rep_cmd->result == 0 ) {
      return_value = 0;
    }
    // NG
    else {
      return_value = -1;
    }
  }
  else {
    log_info("Unknown reply. type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  free( rep_msg );
  unlock();
  return return_value;
}

/**
 * Virtual machine allocate request to Service Controller
 */
int
sc_request_vm_allocate( const char *service, uint64_t band_width, uint64_t n_subscribers ) {
  srm_virtual_machine_allocate_request req_msg;
  int           service_id;
  int           result        = 0;
  vm_info_table* vm_info;
  uint8_t       n_vms;
  uint8_t       n_cpu;
  uint16_t      memory;
  list_element *ip_list = NULL, *pm_ip = NULL;
  int           ii, pos = 0;
  
  arg_assert( service );
  
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  n_vms   = compute_n_vms( service, band_width, n_subscribers );
  n_cpu   = compute_vm_cpu( service );
  memory  = compute_vm_memory( service );
  ip_list = compute_vm_locations( n_vms, n_cpu, memory );
  log_info( "n_vms=%d n_cpu=%d memory=%d ip_list=(%u)", n_vms, n_cpu, memory, (unsigned int)((void*)ip_list) );
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  
  req_msg.hdr.type   = SRM_VIRTUAL_MACHINE_ALLOCATE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  strcpy( req_msg.service_name, service );
  
  // Internet Access Service
  if (      strcmp( service, SERVICE_NAME_INTERNET ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;
    pos = VM_INDEX_TOP_INET;   // for System test
  }
  // Video Service
  else if ( strcmp( service, SERVICE_NAME_VIDEO    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;
    pos = VM_INDEX_TOP_VIDEO;  // for System test
  }
  // Phone Service
  else if ( strcmp( service, SERVICE_NAME_PHONE    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_PHONE;
  }
  // Other Service
  else {
    log_err("Unknown Service :(%s)", service );
    return -1;
  }
  
  pm_ip = ip_list;
  for ( ii = 0; ii < n_vms; ii++, pm_ip = pm_ip->next ) {
    if ( pm_ip == NULL ) {
      log_err("Virtual machine numbers are mismatch. vms=%d, list-count=%d", n_vms, ii );
      break;
    }
    vm_info = get_next_empty_vm_info( &pos );
    if ( vm_info == NULL ) {
      log_err("Too many virtual machine numbers. set-vms=%d, remain-vms=%d", ii, n_vms-ii );
      break;
    }
    
    vm_info->service_id = service_id;
    vm_info->pm_ip_address = *((uint32_t*)(pm_ip->data));
    vm_info->cpu_count = n_cpu;
    vm_info->memory_size = memory;
    
    req_msg.physical_machine_ip_address       = vm_info->pm_ip_address;
    req_msg.virtual_machine.ip_address        = vm_info->vm_ip_address;
    req_msg.virtual_machine.port.ip_address   = vm_info->port_ip_address;
    req_msg.virtual_machine.port.mac_address  = vm_info->port_mac_address;
    req_msg.virtual_machine.cpu_count         = vm_info->cpu_count;
    req_msg.virtual_machine.memory_size       = vm_info->memory_size;
    
    log_info("Send virtual machine allocate request to Service Controller. service=(%s)", req_msg.service_name );
    log_info("Send virtual machine allocate request to Service Controller. pm_ip=(%s)", get_ip_address_string(req_msg.physical_machine_ip_address) );
    log_info("Send virtual machine allocate request to Service Controller. vm_ip=(%s)", get_ip_address_string(req_msg.virtual_machine.ip_address) );
    log_info("Send virtual machine allocate request to Service Controller. port_ip=(%s)", get_ip_address_string(req_msg.virtual_machine.port.ip_address) );
    log_info("Send virtual machine allocate request to Service Controller. mac=(%llx) n_cpu=(%d) memory=(%d)",
              req_msg.virtual_machine.port.mac_address,
              req_msg.virtual_machine.cpu_count,
              req_msg.virtual_machine.memory_size );
    
    result = loc_send_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("Send virtual machine allocate request error." );
      return -1;
    }
  }
  return 0;
}

/**
 * delete service from Service Controller
 */
int
sc_request_delete_service( const char *service ) {
  srm_service_delete_request req_msg;
  int result = 0;
  vm_info_table vm_info;
  int pos;
  
  arg_assert( service );
  
  req_msg.hdr.type   = SRM_DELETE_SERVICE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  strcpy( req_msg.service_name, service );
  
  // Internet Access Service
  if (      strcmp( service, SERVICE_NAME_INTERNET ) == 0 ) {
    ; // OK
  }
  // Video Service
  else if ( strcmp( service, SERVICE_NAME_VIDEO    ) == 0 ) {
    ; // OK
  }
  // Phone Service
  else if ( strcmp( service, SERVICE_NAME_PHONE    ) == 0 ) {
    ; // OK
  }
  // Other Service
  else {
    log_err("Unknown Service :(%s)", service );
    return -1;
  }
  
  pos = 0;
  while ( get_next_vm_info( get_service_id( service ), &vm_info, &pos ) ) {
    vm_info.service_id = SRM_CONFIG_SERVICE_INDEX_NOT_USED;
    vm_info.pm_ip_address = 0;
  }
  
  log_info("Send Service delete request to Service Controller. service=(%s)",
            req_msg.service_name );
  
  
  result = loc_send_request( (header*)&req_msg );
  if ( result != 0 ) {
    log_err("Send Service delete request to Service Controller error." );
    return -1;
  }
  
  return 0;
}


/**
 * Virtual machine migration request to Service Controller
 */
int
sc_request_vm_migration( uint32_t from_pm_ip_address ) {
  srm_virtual_machine_migrate_request req_msg;
  int           result        = 0;
  uint32_t      from_vm_ip_address;
  uint32_t      to_pm_ip_address;
  int           pos;
  
  req_msg.hdr.type   = SRM_VIRTUAL_MACHINE_MIGRATE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  pos = 0;
  while( get_next_vm_ip_address( from_pm_ip_address, &from_vm_ip_address, &pos ) ) {
    
    //@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    to_pm_ip_address = compute_vm_relocation( from_vm_ip_address );
    
    req_msg.from_virtual_machine_ip_address = from_vm_ip_address;
    req_msg.to_physical_machine_ip_address  = to_pm_ip_address;
    
    //@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    log_info("Send virtual machine allocate request to Service Controller. from_vm_ip=(%s)",
              get_ip_address_string( req_msg.from_virtual_machine_ip_address ) );
    log_info("Send virtual machine allocate request to Service Controller. to_pm_ip=(%s)",
              get_ip_address_string( req_msg.to_physical_machine_ip_address  ) );
    
    result = loc_send_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("Send virtual machine allocate request error." );
      return -1;
    }
  }
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

