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


#ifndef DB_MAPPER_H
#define DB_MAPPER_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <getopt.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/eventfd.h>
#endif
#include <limits.h>
#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

#include "checks.h"
#include "wrapper.h"
#include "log_writer.h"
#include "emirates.h"
#include "parse_options.h"
#include "array_util.h"
#include "strbuf.h"


CLOSE_EXTERN
#endif // DB_MAPPER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
