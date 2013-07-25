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


#ifndef DB_MAPPER_PRIV_H
#define DB_MAPPER_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <mysql.h>


#define A_CAST( st )  ( st * )
#define A_DATA( obj ) ( A_CAST( db_wrapper )( obj ) )
#define DATA_PTR( a_data ) ( A_DATA( a_data )->data )


typedef void ( *free_fn_t )( void * );


typedef struct db_wrapper {
  free_fn_t free;
  void *data;
} db_wrapper;


struct mysql {
  MYSQL handler;
  char connection;
};


struct mysql_res {
  MYSQL_RES *res;
};


typedef struct mapper {
  db_wrapper *conn_handler;
  db_wrapper *result;
} mapper;


void db_init( mapper *mapper );
int db_connect( mapper *mapper, const mapper_args *args );


CLOSE_EXTERN
#endif // DB_MAPPER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
