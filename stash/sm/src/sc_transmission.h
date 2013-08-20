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

/**
 * Initialize api information
 */
int
sm_init_sc_transmission();

/**
 * Destroy allocated memories
 */
void
sm_close_sc_transmission();


/**
 * add service to Network Controller
 */
int
sc_request_add_service( const char *service );


/**
 * delete service from Network Controller
 */
int
sc_request_delete_service( const char *service );


#endif // NC_TRANSMISSION_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
