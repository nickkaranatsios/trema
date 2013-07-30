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


#include "db_mapper.h"


#define check_ptr_return( ptr, msg ) \
  do { \
    if ( ( ptr ) ==  NULL ) { \
      log_err( msg ); \
      return NULL; \
    } \
  } while ( 0 )


typedef struct query_info {
  MYSQL_RES *res;
} query_info;


typedef struct table_info {
  const char *name;
  uint32_t queries_nr;
  uint32_t queries_alloc;
  query_info **queries;
} table_info;


typedef struct db_info {
  char *name;
  char *host;
  char *user;
  char *passwd;
  char *socket;
  table_info **tables;
  uint32_t tables_nr;
  uint32_t tables_alloc;
  MYSQL *db_handle;
} db_info;


typedef struct mapper {
  db_info **dbs;
  emirates_iface *emirates;
  jedex_schema *schema;
  jedex_schema *request_schema;
  uint32_t dbs_nr;
  uint32_t dbs_alloc;
} mapper;


typedef struct ref_data {
  strbuf *command;
  jedex_value *val;
  json_t *json_val;
} ref_data;


typedef int ( *config_fn ) ( const char *key, const char *value, void *user_data );
int read_config( config_fn fn, void *user_data, const char *filename );

// message.c
void request_save_topic_callback( jedex_value *val, const char *json, void *user_data );
void request_find_record_callback( jedex_value *val, const char *json, void *user_data );
typedef void ( *primary_key_fn ) ( const char *pk_name, const char *type_str, void *user_data ); 

// init.c
mapper *mapper_initialize( mapper **mptr, int argc, char **argv );
void db_init( mapper *mptr );
int connect_and_create_db( mapper *mptr );

// wrapper.c
uint32_t query( db_info *db, const char *format, ...);


CLOSE_EXTERN
#endif // MAPPER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
