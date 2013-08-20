/*
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


#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include "wrapper.h"
#include "checks.h"
#include "log_writer.h"
#include "daemon.h"
#include "array_util.h"
#include "strbuf.h"
#include "config.h"
#include "service_name.h"
#include "nc_sm_message.h"
#include "jedex_iface.h"
#include "emirates.h"
#include "parse_options.h"
#include "service_manager_priv.h"


CLOSE_EXTERN
#endif // SERVICE_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
