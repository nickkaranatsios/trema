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
#define REP_MSG_OK                  "SUCCESS"
#define REP_MSG_NG                  "FAILURE"

/**
 * Initialize api information
 */
int
sm_init_ob_transmission();

/**
 * Destroy allocated memories
 */
void
sm_close_ob_transmission();

#endif // OB_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
