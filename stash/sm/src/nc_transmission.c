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
#include "arg_assert.h"
#include "log_writer.h"
#include "module_ports.h"
#include "ipc.h"
#include "sm_data_pool.h"
#include "nc_sm_message.h"

static int NC_requester_id = -1;

/**
 * Initialize api information
 */
int
sm_init_nc_transmission() {
  int ret;

  log_debug("\n_/_/_/_/_/_/_/ NC INITIALIZATION START _/_/_/_/_/_/_/_/_/_/_/");
  ret = create_requester( LOCAL_IP_ADDRESS, SM_NC );
  if ( ret < 0 ) {
    log_crit("unable to add requester [Network Controller]");
    return -1;
  }
  NC_requester_id = ret;
  log_debug("_/_/_/_/_/_/_/ NC INITIALIZATION COMPLETE _/_/_/_/_/_/_/_/_/_/_/\n");
  return 0;
}


/**
 * Destroy allocated memories
 */
void
sm_close_nc_transmission() {
  log_debug("\n_/_/_/_/_/_/_/ NC CLOSE START    _/_/_/_/_/_/_/_/_/_/");
  if ( NC_requester_id != -1 ) {
    destroy_requester( NC_requester_id );
  }
  log_debug("_/_/_/_/_/_/_/ NC CLOSE COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
}


static int
loc_send_request_add_service( sm_add_service_profile_request *req_msg ) {
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( req_msg );
  
  log_info("Send add service profile request to Network Controller. service=(%s)", req_msg->name );
  
  req_msg->hdr.type   = SM_ADD_SERVICE_PROFILE_REQUEST;
  req_msg->hdr.length = sizeof(sm_add_service_profile_request);
  
  result = request_message( NC_requester_id, (const char*)req_msg, req_msg->hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Add service profile request to Network Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_ADD_SERVICE_PROFILE_REQUEST_REPLY ) {
    log_info("Received add service profile reply from Network Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err("Received add service profile reply from Network Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  if( rep_msg != NULL ) {
    free( rep_msg );
  }
  return return_value;
}


/**
 * add service to Network Controller
 */
int
nc_request_add_service( const char *service ) {
  sm_add_service_profile_request *req_msg = NULL;
  int   ii, result;
  
  arg_assert( service );
  
  // Internet Access Service
  if ( !strcmp( service, INTERNET_ACCESS_SERVICE ) ) {
    for ( ii = 0; ii < sm_get_configuration_service_profile_internet_info_count(); ii++ ) {
      req_msg = sm_get_configuration_service_profile_internet( ii );
      result = loc_send_request_add_service( req_msg );
      if ( result != 0 ) {
        return -1;
      }
    }
  }
  // Video Service
  else if ( !strcmp( service, VIDEO_SERVICE ) ) {
    for ( ii = 0; ii < sm_get_configuration_service_profile_video_info_count(); ii++ ) {
      req_msg = sm_get_configuration_service_profile_video( ii );
      result = loc_send_request_add_service( req_msg );
      if ( result != 0 ) {
        return -1;
      }
    }
  }
  // Phone Service
  else if ( !strcmp( service, PHONE_SERVICE ) ) {
    for ( ii = 0; ii < sm_get_configuration_service_profile_phone_info_count(); ii++ ) {
      req_msg = sm_get_configuration_service_profile_phone( ii );
      result = loc_send_request_add_service( req_msg );
      if ( result != 0 ) {
        return -1;
      }
    }
  }
  // Other Service
  else {
    log_err( "Unknown Service :(%s)", service );
    return -1;
  }
  
  // OK
  return 0;
}


/**
 * delete service from Network Controller
 */
int
nc_request_delete_service( const char *service ) {
  sm_del_service_profile_request req_msg;
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( service );
  
  strcpy( req_msg.name, service );
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
  
  log_info("Send delete service profile request to Network Controller. service=(%s)", req_msg.name );
  
  req_msg.hdr.type   = SM_DEL_SERVICE_PROFILE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  result = request_message( NC_requester_id, (const char*)&req_msg, req_msg.hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Delete service profile request to Network Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_DEL_SERVICE_PROFILE_REQUEST_REPLY ) {
    log_info("Received delete service profile reply from Network Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err("Received delete service profile reply from Network Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  if( rep_msg != NULL ) {
    free( rep_msg );
  }
  return return_value;
}


/**
 * ignite Service Specific Network Controller
 */
int
nc_request_ssnc_ignition( const char *service ) {
  sm_ignite_ssnc_request req_msg;
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( service );
  
  strcpy( req_msg.name, service );
  // Internet Access Service
  if ( !strcmp( service, SERVICE_NAME_INTERNET ) ) {
    ; // OK
  }
  // Video Service
  else if ( !strcmp( service, SERVICE_NAME_VIDEO ) ) {
    ; // OK
  }
  // Phone Service
  else if ( !strcmp( service, SERVICE_NAME_PHONE ) ) {
    ; // OK
  }
  // Other Service
  else {
    log_err( "Unknown Service :(%s)", service );
    return -1;
  }
  
  log_info( "Send Ignite Service Specific Network Controller request to Network Controller. service=(%s)", req_msg.name );
  
  req_msg.hdr.type   = SM_IGNITE_SSNC_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  result = request_message( NC_requester_id, (const char*)&req_msg, req_msg.hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Ignite Service Specific Network Controller request to Network Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_IGNITE_SSNC_REQUEST_REPLY ) {
    log_info("Received Ignite Service Specific Network Controller reply from Network Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err("Received Ignite Service Specific Network Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  if( rep_msg != NULL ) {
    free( rep_msg );
  }
  return return_value;
}


/**
 * shutdown Service Specific Network Controller
 */
int
nc_request_ssnc_shutdown( const char *service ) {
  sm_shutdown_ssnc_request req_msg;
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( service );
  
  strcpy( req_msg.name, service );
  // Internet Access Service
  if ( !strcmp( service, SERVICE_NAME_INTERNET ) ) {
    ; // OK
  }
  // Video Service
  else if ( !strcmp( service, SERVICE_NAME_VIDEO ) ) {
    ; // OK
  }
  // Phone Service
  else if ( !strcmp( service, SERVICE_NAME_PHONE ) ) {
    ; // OK
  }
  // Other Service
  else {
    log_err( "Unknown Service :(%s)", service );
    return -1;
  }
  
  log_info( "Send Shutdown Service Specific Network Controller request to Network Controller. service=(%s)", req_msg.name );
  
  req_msg.hdr.type   = SM_SHUTDOWN_SSNC_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  result = request_message( NC_requester_id, (const char*)&req_msg, req_msg.hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Shutdown Service Specific Network Controller request to Network Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_SHUTDOWN_SSNC_REQUEST_REPLY ) {
    log_info("Received Shutdown Service Specific Network Controller reply from Network Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err( "Received Shutdown Service Specific Network Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  if( rep_msg != NULL ) {
    free( rep_msg );
  }
  return return_value;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
