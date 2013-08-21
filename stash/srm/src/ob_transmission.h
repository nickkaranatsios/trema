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

#ifndef OB_TRANSMISSION_H
#define OB_TRANSMISSION_H

/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define REQ_MSG_TYPE_ADD_SERVICE    "add_service"
#define REQ_MSG_TYPE_DEL_SERVICE    "del_service"
#define REQ_MSG_TYPE_VER_UP_SERVER  "versionup_server"
#define REP_MSG_OK                  "SUCCESS"
#define REP_MSG_NG                  "FAILURE"

extern SRM_LAST_STATE               srm_last_state;
extern SRM_CONFIG_SERVICE_INDEX     srm_last_add_service;
extern char                         srm_last_versionup_server[ 64 ];
extern service_info_table           srm_service_info[ SRM_CONFIG_MAX_SERVICE_COUNT ];


/**
 * Initialize api information
 */
int
srm_init_ob_transmission();

/**
 * Destroy allocated memories
 */
void
srm_close_ob_transmission();

#endif // OB_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
