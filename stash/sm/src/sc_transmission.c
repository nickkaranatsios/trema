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
#include "sm_data_pool.h"
#include "service_manager_interface.h"
#include "common_defs.h"

static int SC_requester_id = -1;

/**
 * Initialize api information
 */
int
sm_init_sc_transmission() {
  int ret;

  log_debug("\n_/_/_/_/_/_/_/ SM INITIALIZATION START _/_/_/_/_/_/_/_/_/_/_/");
  ret = create_requester( LOCAL_IP_ADDRESS, SM_SC );
  if ( ret < 0 ) {
    log_crit("unable to add requester [Service Controller]");
    return -1;
  }
  SC_requester_id = ret;
  log_debug("_/_/_/_/_/_/_/ SC INITIALIZATION COMPLETE _/_/_/_/_/_/_/_/_/_/_/\n");
  return 0;
}


/**
 * Destroy allocated memories
 */
void
sm_close_sc_transmission() {
  log_debug("\n_/_/_/_/_/_/_/ SC CLOSE START    _/_/_/_/_/_/_/_/_/_/");
  if ( SC_requester_id != -1 ) {
    destroy_requester( SC_requester_id );
  }
  log_debug("_/_/_/_/_/_/_/ SC CLOSE COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
}


/**
 * add service to Service Controller
 */
int
sc_request_add_service( const char *service ) {
  sm_service_module_registration_request req_msg;
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( service );
  
  strcpy( req_msg.service_name, service );
  // Internet Access Service
  if ( !strcmp( service, INTERNET_ACCESS_SERVICE ) ) {
    strcpy( req_msg.service_module, sm_get_configuration_service_module_internet() );
    strcpy( req_msg.recipe, sm_get_configuration_chef_recipe_internet() );
  }
  // Video Service
  else if ( !strcmp( service, VIDEO_SERVICE ) ) {
    strcpy( req_msg.service_module, sm_get_configuration_service_module_video() );
    strcpy( req_msg.recipe, sm_get_configuration_chef_recipe_video() );
  }
  // Phone Service
  else if ( !strcmp( service, PHONE_SERVICE ) ) {
    strcpy( req_msg.service_module, sm_get_configuration_service_module_phone() );
    strcpy( req_msg.recipe, sm_get_configuration_chef_recipe_phone() );
  }
  // Other Service
  else {
    log_err( "Unknown Service :(%s)", service );
    return -1;
  }
  
  log_info( "Send Service module regist request to Service Controller. service=(%s) module=(%s) recipe=(%s)",
            req_msg.service_name, req_msg.service_module, req_msg.recipe );
  
  req_msg.hdr.type   = SM_SERVICE_MODULE_REGIST_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  result = request_message( SC_requester_id, (const char*)&req_msg, req_msg.hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Service module regist request to Service Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_SERVICE_MODULE_REGIST_REPLY ) {
    log_info("Received service module regist reply from Service Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err( "Received service module regist reply from Service Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
              rep_cmd->hdr.type, rep_cmd->hdr.length, rep_cmd->result );
    return_value = -1;
  }
  
  if( rep_msg != NULL ) {
    free( rep_msg );
  }
  return return_value;
}


/**
 * delete service from Service Controller
 */
int
sc_request_delete_service( const char *service ) {
  sm_service_remove_request req_msg;
  char* rep_msg       = NULL;
  int   rep_msg_size  = 0;
  int   result        = 0;
  int   return_value  = 0;
  common_reply  *rep_cmd = NULL;
  
  arg_assert( service );
  
  strcpy( req_msg.service_name, service );
  // Internet Access Service
  if ( !strcmp( service, INTERNET_ACCESS_SERVICE ) ) {
    ; // OK
  }
  // Video Service
  else if ( !strcmp( service, VIDEO_SERVICE ) ) {
    ; // OK
  }
  // Phone Service
  else if ( !strcmp( service, PHONE_SERVICE ) ) {
    ; // OK
  }
  // Other Service
  else {
    log_err( "Unknown Service :(%s)", service );
    return -1;
  }
  
  log_info( "Send Service remove request to Service Controller. service=(%s)",
            req_msg.service_name );
  
  req_msg.hdr.type   = SM_REMOVE_SERVICE_REQUEST;
  req_msg.hdr.length = sizeof(req_msg);
  
  result = request_message( SC_requester_id, (const char*)&req_msg, req_msg.hdr.length, &rep_msg, &rep_msg_size );
  if ( result != 0 ) {
    log_err("Service remove request to Service Controller error. result=(%d)", result );
    return -1;
  }
  arg_assert( rep_msg );
  
  rep_cmd = (common_reply*)rep_msg;
  if( rep_cmd->hdr.type == SM_REMOVE_SERVICE_REPLY ) {
    log_info("Received service remove reply from Service Controller. type=(%d) length=(%d) result=(%d)",
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
    log_err("Received service remove reply from Service Controller is failed. Unknown type=(%d) length=(%d) result=(%d)",
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
