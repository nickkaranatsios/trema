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

#define  SRM_NC_POLLING_CYCLE       3   //[sec]

/**
 * Initialize api information
 */
int
srm_init_nc_transmission();

/**
 * Destroy allocated memories
 */
void
srm_close_nc_transmission();


/**
 * add service vm to Network Controller
 */
int
nc_request_add_service_vm( const char *service );


/**
 * add service profile to Network Controller
 */
int
nc_request_add_service_profile( const char *service, uint64_t band_width, uint64_t n_subscribers );


/**
 * delete service from Network Controller
 */
int
nc_request_delete_service( const char *service );


/**
 * Virtual machine migration request to Network Controller
 */
int
nc_request_vm_migration( uint32_t from_pm_ip_address );


#endif // NC_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
