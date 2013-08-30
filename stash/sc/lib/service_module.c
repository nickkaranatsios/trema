/*
 * Copyright (C) 2008-2013 NEC Corporation
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
#include <string.h>
#include <assert.h>
#include <linux/limits.h>
#include "log_writer.h"
#include "service_module.h"
#include "service_controller.h"

static const char *RECIPE_DIR = "/home/y-shinohara/PoCv2/service_controller/lib";

int
configure_service_module( char *service_name, char *recipe, uint32_t ip_address ) {
  int ret = RET_OK;

  char cmd_buf[ PATH_MAX ];
  cmd_buf[ 0 ] = '\0';
  if ( !strncmp( service_name, SERVICE_NAME_BRAS, SERVICE_NAME_LENGTH ) ) {
    sprintf( cmd_buf, "sudo %s/accel-ppp-recipe.rb -r %s -t %u --pppoe ,%u &",
      RECIPE_DIR,
      recipe,
      ip_address,
      ip_address );
  } else if ( !strncmp( service_name, SERVICE_NAME_IPTV, SERVICE_NAME_LENGTH ) ) {
    sprintf( cmd_buf, "sudo %s/vlc-recipe.rb -r %s -t %u -n %u &",
      RECIPE_DIR,
      recipe,
      ip_address,
      ip_address );
  } else {
    log_err( "No such service %s", service_name );
    assert( 0 );
  }
  if ( cmd_buf[ 0 ] != '\0' ) {
    ret = system( cmd_buf );
  }
  return ret;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
