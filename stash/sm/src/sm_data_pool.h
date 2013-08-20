/*
 * Service Manager data pool
 *
 * Config file data administrator
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * NEC Confidential
 *
 */

#ifndef SM_DATA_POOL_H
#define SM_DATA_POOL_H

/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "nc_sm_message.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define SM_CONFIG_INI_PATH              "./service_manager.ini"

typedef enum {
  SM_CONFIG_SERVICE_INDEX_INTERNET = 0,
  SM_CONFIG_SERVICE_INDEX_VIDEO,
  SM_CONFIG_SERVICE_INDEX_PHONE,
  SM_CONFIG_MAX_SERVICE_COUNT
} SM_CONFIG_SERVICE_INDEX;

/*----------------------------------------------------------------------------
 * Configuration
 *----------------------------------------------------------------------------*/
// Service Modules
#define SERVICE_MODULE_INTERNET         "internet_service_module"
#define SERVICE_MODULE_VIDEO            "video_service_module"
#define SERVICE_MODULE_PHONE            "phone_service_module"

// Chef Recipes
#define CHEF_RECIPE_INTERNET            "internet_recipe"
#define CHEF_RECIPE_VIDEO               "video_recipe"
#define CHEF_RECIPE_PHONE               "phone_recipe"

// Service Name
#define SERVICE_NAME_INTERNET           "Internet_Access_Service"
#define SERVICE_NAME_VIDEO              "Video_Service"
#define SERVICE_NAME_PHONE              "Phone_Service"

/******************************************************************************
 * Structs
 ******************************************************************************/
typedef struct {
  int                            info_count;
  sm_add_service_profile_request info[ 2 ];
} sm_config_service_profile_table;

/**
 * load initial configuration from file
 */
int
sm_load_configuration();

const char*
sm_get_configuration_service_module_internet();
  
const char*
sm_get_configuration_service_module_video();

const char*
sm_get_configuration_service_module_phone();

const char*
sm_get_configuration_chef_recipe_internet();
  
const char*
sm_get_configuration_chef_recipe_video();

const char*
sm_get_configuration_chef_recipe_phone();

int
sm_get_configuration_service_profile_internet_info_count();

int
sm_get_configuration_service_profile_video_info_count();

int
sm_get_configuration_service_profile_phone_info_count();

sm_add_service_profile_request*
sm_get_configuration_service_profile_internet( int index );

sm_add_service_profile_request*
sm_get_configuration_service_profile_video( int index );

sm_add_service_profile_request*
sm_get_configuration_service_profile_phone( int index );


#endif // SM_DATA_POOL_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
