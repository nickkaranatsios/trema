/*
 * Copyright (C) 2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/eventfd.h>
#endif
#include <limits.h>
#include <hiredis/hiredis.h>
#include "common_defs.h"
#include "config.h"


#define SERVICE_MAX 64

typedef struct {
  char          name[ SERVICE_MAX ];
  uint16_t      match_id;
  uint32_t      wildcards;
  uint64_t      dl_src;
  uint64_t      dl_dst;
  uint16_t      dl_vlan;
  uint8_t       dl_vlan_pcp;
  uint16_t      dl_type;
  uint8_t       nw_tos;
  uint8_t       nw_proto;
  uint32_t      nw_src;
  uint32_t      nw_dst;
  uint16_t      tp_src;
  uint16_t      tp_dst;
} sm_add_service_profile_request;

#define init_internet_profile_spec0 \
  { "Internet_Access_Service", 1, 0x123, 0, 0, 0, 0, 0x8863, 0, 0, 0, 0, 0, 0 }

#define init_internet_profile_spec1 \
  { "Internet_Access_Service", 2, 0x456, 0, 0, 0, 0, 0x8864, 0, 0, 0, 0, 0, 0 }
 
#define init_internet_profile_spec \
  init_internet_profile_spec0, \
  init_internet_profile_spec1

#define init_video_profile_spec { "Video_Service", 1, 0x323f3f, 0, 0, 0, 0, 0x0800, 0, 0, 0, 0xe0010100, 0, 0 }

#define init_phone_profile_spec { "Phone_Service", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

static sm_add_service_profile_request profile_spec[] = {
  init_internet_profile_spec,
  init_video_profile_spec,
  init_phone_profile_spec
};


static int
handle_config( const char *key, const char *value, void *user_data ) {
}


/*
 * Too many definitions of internet_service_name 
 */
int
main( int argc, char **argv ) {
#ifdef TEST
  for ( size_t i = 0; i < sizeof( profile_spec ) / sizeof( profile_spec[ 0 ] ); i++ ) {
#ifdef HAVE_CARRIER_EDGE
    printf( "service name %s\n", profile_spec[ i ].name );
#endif
  }
#endif
  read_config( "service_manager.conf", handle_config, NULL );
  printf( "internet service %s\n", INTERNET_ACCESS_SERVICE );
  printf( "internet service %s\n", VIDEO_SERVICE );
  printf( "phone service %s\n", PHONE_SERVICE );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
