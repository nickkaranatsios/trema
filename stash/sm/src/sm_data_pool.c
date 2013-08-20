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


/******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include "arg_assert.h"
#include "log_writer.h"
#include "nc_sm_message.h"
#include "sm_data_pool.h"
#include "openflow.h"

const char* service_module_internet;
const char* service_module_video;
const char* service_module_phone;

// Chef Recipes
const char* chef_recipe_internet;
const char* chef_recipe_video;
const char* chef_recipe_phone;

#define init_internet_profile_spec0 { 1, OFPFW_ALL & ~OFPFW_DL_TYPE, 0, 0, 0, 0, 0x8863, 0, 0, 0, 0, 0, 0 }
#define init_internet_profile_spec1 { 2, OFPFW_ALL & ~OFPFW_DL_TYPE, 0, 0, 0, 0, 0x8864, 0, 0, 0, 0, 0, 0 }

// wildcards = OFPFW_ALL & ~OFPFW_DL_TYPE & ~OFPFW_NW_DST_MASK
// multicast ip address 224.1.1.0
#define init_video_profile_spec { 1, 0x3f23fef, 0, 0, 0, 0, 0x0800, 0, 0, 0, 0xe0010100, 0, 0 }

#define init_phone_profile_spec { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#define init_internet_profile_spec \
  init_internet_profile_spec0, \
  init_internet_profile_spec1

static sm_add_service_profile_request internet_profile_spec[] = {
  init_internet_profile_spec
};

static sm_add_service_profile_request video_profile_spec[] = {
  init_video_profile_spec
};

static sm_phone_profile_request phone_profile_spec[] = {
  init_phone_profile_spec
};

typedef struct service_profile {
  char key[ API_MAX_LEN_SERVICE_ID ];
  sm_add_service_profile_request profile_req;
} service_profile;

typedef struct profile_table {
  service_profile **profiles;
  uint32_t profiles_nr;
  uint32_t profiles_alloc;
} profile_table;

static service_profile *
make_profile( const char *name, size_t name_len, profile_table *self ) {
  uint32_t i;
  
  if ( name_len > API_MAX_LEN_SERVICE_ID ) {
    name_len = API_MAX_LEN_SERVICE_ID - 1;
  }
  for ( i = 0; i < self->profiles_nr; i++ ) {
    if ( !strncmp( self->profiles[ i ]->key, name, name_len ) ) {
      return self->profiles[ i ];
    }
  }
  ALLOC_GROW( self->profiles, self->profiles_nr + 1, self->profiles_alloc );
  size_t nitems = 1;
  service_profile *profile = xcalloc( nitems, sizeof( *profile ) );
  strncpy( profile->key, name, name_len );
  self->profiles[ self->profiles_nr++ ] = profile;

  return profile;
}


static void
assign_service_profile( const char *subkey, const char *value, service_profile *profile ) {
  int base = 0;

  if ( !prefixcmp( subkey, ".name" ) ) {
    strncpy( profile->profile_req.name, value, sizeof( profile->profile_req.name ) - 1 );
  }
  else if ( !prefixcmp( subkey, ".match_id" ) ) {
    profile->profile_req.match_id = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".wildcards" ) ) {
    profile->profile_req.wildcards = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_src" ) ) {
    profile->profile_req.dl_src = ( uint64_t ) strtoll( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_dst" ) ) {
    profile->profile_req.dl_dst = ( uint64_t ) strtoll( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_vlan" ) ) {
    profile->profile_req.dl_vlan = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_vlan_pcp" ) ) {
    profile->profile_req.dl_vlan_pcp = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".dl_type" ) ) {
    profile->profile_req.dl_type = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_tos" ) ) {
    profile->profile_req.nw_tos = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_proto" ) ) {
    profile->profile_req.nw_proto = ( uint8_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_src" ) ) {
    profile->profile_req.nw_src = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".nw_dst" ) ) {
    profile->profile_req.nw_dst = ( uint32_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".tp_src" ) ) {
    profile->profile_req.tp_src = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".tp_dst" ) ) {
    profile->profile_req.tp_dst = ( uint16_t ) strtol( value, NULL, base );
  }
  else {
    log_err( "Failed to assign subkey(%s) with value(%s)", subkey, value );
  }
}

static int
handle_config( const char *key, const char *value, void *user_data ) {
  if ( !prefixcmp( key, "profile_spec." ) ) {
    profile_table *self = user_data;
    const char *subkey;
    const char *name;
    name = key + 13;
    subkey = strrchr( name, '.' );
    service_profile *profile = make_profile( name, ( size_t ) ( subkey - name ),  self );
    if ( profile && subkey ) {
      assign_service_profile( subkey, value, profile );
    }
  }

  return 0;
}


int
sm_load_configuration( const char *config_fn, srm_info *self ) {
  read_config( config_fn, handle_config, &profile_tbl );
  
  return 0;
}

const char*
sm_get_configuration_service_module_internet() {
  return INTERNET_SERVICE_MODULE;
}
  
const char*
sm_get_configuration_service_module_video() {
  return VIDEO_SERVICE_MODULE;
}

const char*
sm_get_configuration_service_module_phone() {
  return PHONE_SERVICE_MODULE;
}

const char*
sm_get_configuration_chef_recipe_internet() {
  return INTERNET_CHEF_RECIPE;
}
  
const char*
sm_get_configuration_chef_recipe_video() {
  return VIDEO_CHEF_RECIPE;
}

const char*
sm_get_configuration_chef_recipe_phone() {
  return PHONE_CHEF_RECIPE;
}

int
sm_get_configuration_service_profile_internet_info_count() {
  return ARRAY_SIZE( internet_profile_spec );
}

int
sm_get_configuration_service_profile_video_info_count() {
  return ARRAY_SIZE( video_profile_spec );
}

int
sm_get_configuration_service_profile_phone_info_count() {
  // if set return ARRAY_SIZE( phone_profile_spec )
  return 0;
}


sm_add_service_profile_request*
sm_get_configuration_service_profile_internet( int index ) {
  arg_assert( index >= 0 );
  arg_assert( index <  ARRAY_SIZE( internet_profile_spec ) );

  return &internet_profile_spec[ index ];
}

sm_add_service_profile_request*
sm_get_configuration_service_profile_video( int index ) {
  arg_assert( index >= 0 );
  arg_assert( index < ARRAY_SIZE( video_profile_spec ) );

  return &video_profile_spec[ index ];
}

sm_add_service_profile_request*
sm_get_configuration_service_profile_phone( int index ) {
  arg_assert( index >= 0 );
  arg_assert( index <  ARRAY_SIZE( phone_profile_spec ) );

  return &phone_profile_spec[ index ];
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
