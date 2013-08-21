/*
 * Service Controller transmission
 *
 * SC external system interface
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */

#ifndef SC_TRANSMISSION_H
#define SC_TRANSMISSION_H

/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "service_manager_interface.h"

#define  SRM_SC_POLLING_CYCLE       3   //[sec]

/**
 * Initialize api information
 */
int
srm_init_sc_transmission();

/**
 * Destroy allocated memories
 */
void
srm_close_sc_transmission();


/**
 * Virtual machine allocate request to Service Controller
 */
int
sc_request_vm_allocate( const char *service, uint64_t band_width, uint64_t n_subscribers );


/**
 * delete service from Network Controller
 */
int
sc_request_delete_service( const char *service );


/**
 * Virtual machine migration request to Service Controller
 */
int
sc_request_vm_migration( uint32_t from_pm_ip_address );


#endif // NC_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
