/*
 * OSS/BSS transmission
 *
 * OSS/BSS request handler
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
#include "sc_transmission.h"
#include "nc_transmission.h"
#include "ob_transmission.h"
#include "srm_dmy_core.h"

SRM_LAST_STATE               srm_last_state = SRM_STATE_INITIAL;
SRM_CONFIG_SERVICE_INDEX     srm_last_add_service = SRM_CONFIG_SERVICE_INDEX_INTERNET;
char                         srm_last_versionup_server[ 64 ];
service_info_table           srm_service_info[ SRM_CONFIG_MAX_SERVICE_COUNT ];

static int OB_replier_id = -1;


/**
 * system resource manager add service control
 */
static int
loc_add_service( const char *service, uint64_t band_width, uint64_t n_subscribers ) {
  int result = 0;
  
  arg_assert( service );
  
  log_info("add service: service='%s' band_width=%llu subscribers=%llu", service, band_width, n_subscribers );
  
  result = sc_request_vm_allocate( service, band_width, n_subscribers );
  if ( result != 0 ) {
    return -1;
  }
  result = nc_request_add_service_vm( service );
  if ( result != 0 ) {
    return -1;
  }
  result = nc_request_add_service_profile( service, band_width, n_subscribers );
  if ( result != 0 ) {
    return -1;
  }
  
  
  // success.
  return 0;
}


/**
 * system resource manager delete service control
 */
static int
loc_delete_service( const char *service ) {
  int result = 0;
  
  arg_assert( service );
  
  log_info("delete service: service='%s'", service );
  
  result = nc_request_delete_service( service );
  if ( result != 0 ) {
    return -1;
  }
  result = sc_request_delete_service( service );
  if ( result != 0 ) {
    return -1;
  }
  
  // success.
  return 0;
}


/**
 * system resource manager migration server
 */
static int
loc_migration_server( const char *server ) {
  int result = 0;
  
  arg_assert( server );
  
  log_info("migration server: server='%s'", server );
  
  result = nc_request_vm_migration( get_ip_address_int( server ) );
  if ( result != 0 ) {
    return -1;
  }
  result = sc_request_vm_migration( get_ip_address_int( server ) );
  if ( result != 0 ) {
    return -1;
  }
  
  // success.
  return 0;
}

/**
 * service manager request handler
 */
static int
loc_handle_oss_bss( int id, const char *req, int req_size, char **rep, int *rep_size, void *user_data ) {
  char     *requested_message = NULL;
  char     *reply_message     = NULL;
  char      type[64];
  char      service[64];
  char      server[64];
  uint64_t  band_width;
  double    band_width_f;
  uint64_t  n_subscribers;
  int       param_count;
  int       result;
  
  reply_message = malloc( 256 );
  memset( reply_message, '\0', 256 );
  *rep      = reply_message;
  *rep_size = 0;
  
  arg_assert( req );
  arg_assert( req_size > 0 );
  arg_assert( rep );
  arg_assert( rep_size );
  
  log_debug("\n_/_/_/_/_/_/_/ RECV request from OSS/BSS _/_/_/_/_/_/_/");
  
  requested_message = malloc( req_size + 1 );
  memset( requested_message, '\0' , ( req_size + 1 ) );
  memcpy( requested_message, req, req_size );
  log_info("received message:(%s) size=%d", requested_message, req_size );
  
  // Analize request.
  param_count = sscanf(requested_message, "%s", type );
  
  // Add Service
  if (      strcmp( type, REQ_MSG_TYPE_ADD_SERVICE ) == 0 ) {
    log_info("\n_/_/_/_/_/_/_/ RECV request from OSS/BSS Add Service _/_/_/_/_/_/_/");
    param_count = sscanf(requested_message, "%s%s%lf%llu", type, service, &band_width_f, &n_subscribers );
    arg_assert( param_count == 4 );
    srm_last_add_service = get_service_id( service );
    band_width = (uint64_t)( band_width_f * 10 );
    srm_service_info[ srm_last_add_service ].band_width_f  = band_width_f;
    srm_service_info[ srm_last_add_service ].band_width = band_width;
    srm_service_info[ srm_last_add_service ].n_subscribers = n_subscribers;
    srm_last_add_service = get_service_id( service );
    srm_last_state = SRM_STATE_STARTED;
    result = loc_add_service( service, band_width, n_subscribers );
    if ( result != 0 ) {
      log_err("loc_add_service() error. result=%d", result );
    }
  }
  // Delete Service
  else if ( strcmp( type, REQ_MSG_TYPE_DEL_SERVICE ) == 0 ) {
    log_info("\n_/_/_/_/_/_/_/ RECV request from OSS/BSS Delete Service _/_/_/_/_/_/_/");
    param_count = sscanf(requested_message, "%s%s", type, service );
    arg_assert( param_count == 2 );
    result = loc_delete_service(  service );
  }
  // Version up server
  else if ( strcmp( type, REQ_MSG_TYPE_VER_UP_SERVER ) == 0 ) {
    log_info("\n_/_/_/_/_/_/_/ RECV request from OSS/BSS Version up server _/_/_/_/_/_/_/");
    param_count = sscanf(requested_message, "%s%s", type, server );
    arg_assert( param_count == 2 );
    result = loc_migration_server( server );
    if ( result == 0 ) {
      strcpy( srm_last_versionup_server, server );
      srm_last_state = SRM_STATE_VERSIONUP;
    }
  }
  else {
    log_err("Illegal request type. type=(%s)", type );
    result = -1;
  }
  
  if ( result == 0 ) {
    strcpy( reply_message, REP_MSG_OK );
  }
  else {
    strcpy( reply_message, REP_MSG_NG );
  }
  *rep_size = strlen( reply_message );
  
  free( requested_message );
  log_debug("_/_/_/_/_/_/_/ FIN  request from OSS/BSS _/_/_/_/_/_/_/\n");
  return 0;
}


/**
 * Initialize api information
 */
int
srm_init_ob_transmission() {
  int ret;

  log_debug("\n_/_/_/_/_/_/_/ OSS/BSS INITIALIZATION START _/_/_/_/_/_/_/_/_/_/_/");
  /*==================================================================*/
  /*                            OSS/BSS                               */
  /*==================================================================*/
  /*1. Replier*/
  if ( ( ret = create_replier( OSS_SRM ) ) < 0 ) {
    log_crit("unable to create replier for OSS/BSS port_no=(%d) ret=(%d)",OSS_SRM,ret);
    return -1;
  }
  OB_replier_id = ret;
  if ( ( ret = add_received_request_message_callback( OB_replier_id, loc_handle_oss_bss, NULL ) ) < 0 ) {
    log_crit("unable to add replier callback for OSS/BSS port_no=(%d) ret=(%d)",OSS_SRM,ret);
    return -1;
  }
  
  log_debug("_/_/_/_/_/_/_/ OSS/BSS INITIALIZATION COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
  return 0;
}

/**
 * Destroy allocated memories
 */
void
srm_close_ob_transmission() {
  log_debug("\n_/_/_/_/_/_/_/ OSS/BSS CLOSE START    _/_/_/_/_/_/_/_/_/_/");
  if ( OB_replier_id != -1 ) {
    destroy_replier( OB_replier_id );
  }
  log_debug("_/_/_/_/_/_/_/ OSS/BSS CLOSE COMPLETE _/_/_/_/_/_/_/_/_/_/\n");
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
