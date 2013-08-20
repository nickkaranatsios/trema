/*
 * Network Controller transmission
 *
 * NC external system interface
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */

#ifndef NC_TRANSMISSION_H
#define NC_TRANSMISSION_H

/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


/**
 * Initialize api information
 */
int
sm_init_nc_transmission();

/**
 * Destroy allocated memories
 */
void
sm_close_nc_transmission();


/**
 * add service to Network Controller
 */
int
nc_request_add_service( const char *service );


/**
 * delete service from Network Controller
 */
int
nc_request_delete_service( const char *service );


/**
 * ignite Service Specific Network Controller
 */
int
nc_request_ssnc_ignition( const char *service );


/**
 * shutdown Service Specific Network Controller
 */
int
nc_request_ssnc_shutdown( const char *service );

#endif // NC_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
