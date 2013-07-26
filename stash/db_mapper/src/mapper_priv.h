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


#ifndef MAPPER_PRIV_H
#define MAPPER_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <mysql.h>
#include "db_mapper.h"


#define A_CAST( st )  ( st * )
#define A_DATA( obj ) ( A_CAST( db_wrapper )( obj ) )
#define DATA_PTR( a_data ) ( A_DATA( a_data )->data )


typedef struct query_info {
  MYSQL_RES *res;
} query_info;


typedef struct db_info {
  char *name;
  char *host;
  char *user;
  char *passwd;
  char *socket;
  query_info **queries;
  MYSQL *db_handle;
} db_info;


typedef struct mapper {
  db_info **dbs;
  uint32_t dbs_nr;
  uint32_t dbs_alloc;
} mapper;



int prefixcmp( const char *str, const char *prefix );
int suffixcmp( const char *str, const char *suffix );

typedef int ( *config_fn_t ) ( mapper *mapper, const char *key, const char *value );
int read_config( mapper *mapper, config_fn_t fn, const char *filename );

void db_init( mapper *mapper );
int db_connect( mapper *mapper );


CLOSE_EXTERN
#endif // MAPPER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
