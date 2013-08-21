/*
 * Network Controller IF
 *
 * Handle Network Controller transmission
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
#include "nc_srm_message.h"
#include "system_resource_manager_interface.h"
#include "srm_dmy_core.h"                   // TODO:
#include "nc_transmission.h"
#include "create_json_file.h"
#include "ob_transmission.h"

static int        NC_requester_id = -1;
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
    case SRM_ADD_SERVICE_VM_REQUEST :
      if ( rep_type == SRM_ADD_SERVICE_VM_REPLY ) {
        return_value = 0; // OK
      }
      break;
    case SRM_UPD_SERVICE_PROFILE_REQUEST :
      if ( rep_type == SRM_UPD_SERVICE_PROFILE_REPLY ) {
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
    case SRM_NC_STATISTICS_REQUEST :
      if ( rep_type == SRM_NC_STATISTICS_REPLY ) {
        return_value = 0; // OK
      }
      break;
    case SRM_NC_LINK_UTILIZATION_REQUEST :
      if ( rep_type == SRM_NC_LINK_UTILIZATION_REPLY ) {
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
 * send request
 *  start, stop, migration
 */
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
  req_type = req_msg->type;
  
  lock();
  result = request_message( NC_requester_id, (const char*)req_msg, req_msg->length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Send request error. result=(%d)", result );
    unlock();
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd  = (common_reply*)rep_msg;
  rep_type = rep_cmd->hdr.type;
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
    log_err("Unknown reply. type=(%d) length=(%d) result=(%d)",
              rep_type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  free( rep_msg );
  unlock();
  return return_value;
}

/**
 * received statistics_status processing.
 */
static int
loc_received_statistics( srm_nc_statistics_reply* info ) {
  int result;
  
  result = parse_service_statistics_for_add_service( 
          get_service_name( SRM_CONFIG_SERVICE_INDEX_INTERNET ),
          srm_service_info[ SRM_CONFIG_SERVICE_INDEX_INTERNET ].band_width_f,
          srm_service_info[ SRM_CONFIG_SERVICE_INDEX_INTERNET ].n_subscribers,
          (const char*)info );
  
  if ( result != 0 ) {
    log_err("inet json file creation failed. (nc_statistics_status)");
  }
  
  result = parse_service_statistics_for_add_service( 
          get_service_name( SRM_CONFIG_SERVICE_INDEX_VIDEO ),
          srm_service_info[ SRM_CONFIG_SERVICE_INDEX_VIDEO ].band_width_f,
          srm_service_info[ SRM_CONFIG_SERVICE_INDEX_VIDEO ].n_subscribers,
          (const char*)info );
  
  if ( result != 0 ) {
    log_err("video json file creation failed. (nc_statistics_status)");
  }
  return 0;
}

/**
 * received link_utilization processing.
 */
static int
loc_received_link_utilization( srm_nc_link_utilization_reply* info ) {
  int ii, result;
  arg_assert( info );
  
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  for ( ii = 0; ii < info->info_num; ii++ ) {
    update_link( info->info[ ii ].from_dp_id,
                 info->info[ ii ].to_dp_id,
                 info->info[ ii ].utilization );
  }
  //@@@@ TODO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  
  if ( srm_last_state == SRM_STATE_VERSIONUP ) {
    result = parse_physical_machine_topology_for_versionup_server( 
            srm_last_versionup_server,
            (const char*)info );
  }
  else {
    result = parse_physical_machine_topology_for_add_service( 
            get_service_name( srm_last_add_service ),
            srm_service_info[ srm_last_add_service ].band_width_f,
            srm_service_info[ srm_last_add_service ].n_subscribers,
            (const char*)info );
  }
  
  if ( result != 0 ) {
    log_err("json file creation failed. (link_utilization)");
    return -1;
  }
  
  return 0;
}

/**
 * send request
 *  statistics_status, link_utilization
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
  
  lock();
  result = request_message( NC_requester_id, (const char*)req_msg, req_msg->length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Send request error. result=(%d)", result );
    unlock();
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd  = (common_reply*)rep_msg;
  rep_type = rep_cmd->hdr.type;
  req_type = req_msg->type;
  if( check_repily_type( req_type, rep_type ) == 0 ) {
    log_info("Received reply. type=(%d) length=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length );
    
    result = -1;
    switch ( rep_type ) {
      case SRM_NC_STATISTICS_REPLY :
        result = loc_received_statistics( (srm_nc_statistics_reply*)rep_msg );
        break;
      case SRM_NC_LINK_UTILIZATION_REPLY :
        result = loc_received_link_utilization( (srm_nc_link_utilization_reply*)rep_msg );
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
nc_pooling_thread_routine( void* thread_argument ) {
  srm_statistics_status_request req_msg;
  int result;
  
  req_msg.hdr.length = sizeof(req_msg);
  
  while ( 1 ) {
    
//#if 0 // test
    req_msg.hdr.type   = SRM_NC_STATISTICS_REQUEST;
    log_info("Send SRM_NC_STATISTICS_REQUEST");
    result = loc_send_polling_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("nc_statistics_request polling error. (%d)", result);
    }
    
    req_msg.hdr.type   = SRM_NC_LINK_UTILIZATION_REQUEST;
    log_info("Send SRM_NC_LINK_UTILIZATION_REQUEST");
    result = loc_send_polling_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("link_utilization_request polling error. (%d)", result);
    }
//#endif // test
    
    sleep( SRM_NC_POLLING_CYCLE );
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
  result = pthread_create( &polling_thread, &attr, nc_pooling_thread_routine, NULL );
  if ( result != 0 ){
    log_err("pthread_create() failed. (%s)", strerror(errno) );
    return -1;
  }
  
  return 0;
}

/**
 * Initialize api information
 */
int
srm_init_nc_transmission() {
  int ret;

  log_debug("\n_/_/_/_/_/_/_/ NC INITIALIZATION START _/_/_/_/_/_/_/_/_/_/_/");
  // mutex initialize
  pthread_mutex_init( &requester_mutex, NULL );
  
  ret = create_requester( LOCAL_IP_ADDRESS, SRM_NC );
  if ( ret < 0 ) {
    log_crit("unable to add requester [Network Controller]");
    return -1;
  }
  NC_requester_id = ret;

  // create polling thread;
  ret = create_polling_thread();
  if ( ret < 0 ) {
    log_crit("unable to create polling thread [Network Controller]");
    return -1;
  }
  
  log_debug("_/_/_/_/_/_/_/ NC INITIALIZATION COMPLETE _/_/_/_/_/_/_/_/_/_/_/\n");
  return 0;
}


/**
 * Destroy allocated memories
 */
void
srm_close_nc_transmission() {
  log_debug("\n_/_/_/_/_/_/_/ NC CLOSE START    _/_/_/_/_/_/_/_/_/_/");
  if ( NC_requester_id != -1 ) {
    destroy_requester( NC_requester_id );
  }
  if ( pthread_cancel( polling_thread ) != 0 ) {
    log_err("pthread_cancel() failed. (%s)", strerror(errno) );
  }
  if ( pthread_join( polling_thread, NULL ) != 0 ) {
    log_err("pthread_join() failed. (%s)", strerror(errno) );
  }
  log_debug("_/_/_/_/_/_/_/ NC CLOSE COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
}


/**
 * add service vm to Network Controller
 */
int
nc_request_add_service_vm( const char *service ) {
  srm_add_service_vm_request req_msg;
  vm_info_table vm_info;
  int           service_id;
  int           pos;
  int           result = 0;
  
  arg_assert( service );
  
  memset( &req_msg, 0, sizeof(req_msg) );
  req_msg.hdr.type   = SRM_ADD_SERVICE_VM_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  strcpy( req_msg.info.name, service );
  
  // Internet Access Service
  if (      strcmp( service, SERVICE_NAME_INTERNET ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_INTERNET;
  }
  // Video Service
  else if ( strcmp( service, SERVICE_NAME_VIDEO    ) == 0 ) {
    service_id = SRM_CONFIG_SERVICE_INDEX_VIDEO;
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
  
  pos = 0;
  while( get_next_vm_info( service_id, &vm_info, &pos ) ) {
    req_msg.info.ap.name_id         = API_NAME_ID_SERVICE_VM;
    req_msg.info.ap.action_id       = API_ACTION_ID_ADDED;
    req_msg.info.usage              = (uint64_t)(-1);
    req_msg.info.data_mac           = vm_info.port_mac_address;
    req_msg.info.data_ip            = vm_info.port_ip_address;
    req_msg.info.control_ip         = vm_info.vm_ip_address;
    req_msg.info.param_count        = 2;
    req_msg.info.param_asp[0].value = vm_info.user_count;
    req_msg.info.param_asp[1].value = vm_info.group_address;
    sprintf( req_msg.info.param_asp[0].name, "User_count" );
    sprintf( req_msg.info.param_asp[1].name, "group_address" );
    
    log_info("Send service vm request to Network Controller. service=(%s)", req_msg.info.name );
    log_info("Send service vm request to Network Controller. data_mac=(%llx)", req_msg.info.data_mac );
    log_info("Send service vm request to Network Controller. data_ip=(%s)", get_ip_address_string( req_msg.info.data_ip ) );
    log_info("Send service vm request to Network Controller. ctrl_ip=(%s)", get_ip_address_string( req_msg.info.control_ip ) );
    log_info("Send service vm request to Network Controller. User_count=(%llu)", req_msg.info.param_asp[0].value );
    log_info("Send service vm request to Network Controller. group_address=(%s)", get_ip_address_string( req_msg.info.param_asp[1].value ) );
    
    result = loc_send_request( (header*)&req_msg );
    if ( result != 0 ) {
      log_err("Send service vm request error." );
      return -1;
    }
  }
  return 0;
}


/**
 * add service profile to Network Controller
 */
int
nc_request_add_service_profile( const char *service, uint64_t band_width, uint64_t n_subscribers ) {
  srm_upd_service_profile_request req_msg;
  int           result = 0;
  
  arg_assert( service );
  
  memset( &req_msg, 0, sizeof(req_msg) );
  req_msg.hdr.type   = SRM_UPD_SERVICE_PROFILE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  strcpy( req_msg.name, service );
  req_msg.bandwidth   = band_width;
  req_msg.subscribers = n_subscribers;
  
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
  
  log_info("Send service profile request to Network Controller. service=(%s) band_width=(%llu) subscribers=(%llu)",
              req_msg.name, req_msg.bandwidth, req_msg.subscribers );
    
  result = loc_send_request( (header*)&req_msg );
  if ( result != 0 ) {
    log_err("Send service profile request error." );
    return -1;
  }
  return 0;
}


/**
 * delete service from Network Controller
 */
int
nc_request_delete_service( const char *service ) {
  srm_service_delete_request req_msg;
  int   result        = 0;
  
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
  
  log_info("Send Service delete request to Network Controller. service=(%s)",
            req_msg.service_name );
  
  result = loc_send_request( (header*)&req_msg );
  if ( result != 0 ) {
    log_err("Send Service delete request to Network Controller error." );
    return -1;
  }
  
  return 0;
}


/**
 * Virtual machine migration request to Network Controller
 */
int
nc_request_vm_migration( uint32_t from_pm_ip_address ) {
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
    result = assign_pm_ip_address_to_vm( from_vm_ip_address, to_pm_ip_address );
    if ( result != 0 ) {
      log_err("assign_pm_ip_address_to_vm() error. result=%d", result );
      return -1;
    }
    log_info("Send virtual machine allocate request to Network Controller. from_vm_ip=(%s)",
              get_ip_address_string( req_msg.from_virtual_machine_ip_address ) );
    log_info("Send virtual machine allocate request to Network Controller. to_pm_ip=(%s)",
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

