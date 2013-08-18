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


#define check_ptr_return( ptr, retval, msg ) \
  do { \
    if ( ( ptr ) == NULL  ) { \
      log_err( msg ); \
      return retval; \
    } \
  } while ( 0 )


#define MAX_QUERIES_PER_TABLE 10
/*
 * this is the row_count value of the limit clause of the select statement.
 * TODO: Eventually this should be set to 50.
 */
#define ROW_COUNT 1
#define INSERT_CLAUSE 1
#define WHERE_CLAUSE 2
#define REDIS_CLAUSE 3

typedef struct query_info {
  MYSQL_RES *res;
  char client_id[ IDENTITY_MAX ];
  uint32_t used;
  uint32_t row_count;
} query_info;


typedef struct key_info {
  // only primitive type keys supported
  jedex_type schema_type;
  // the name of the primary key
  const char *name;
  // the converted sql type string used from the jedex type.
  const char *sql_type_str;
} key_info;
  

typedef struct table_info {
  const char *name;
  
  /*
   * the first query used for storing intermediate results.
   * should start from 1.
  */
  query_info queries[ MAX_QUERIES_PER_TABLE ];
  // information about each primary key
  char *merge_key;
  size_t keys_nr;
  size_t keys_alloc;
  key_info **keys;
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
  jedex_schema *request_reply_schema;
  /*
   * we often send replies so it better to save the jedex object
   * at initialization. We don't that with the request that's why
   * we still keep the request_reply_schema pointer
   */
  jedex_value *reply_val;
  redisContext *rcontext;
  uint32_t dbs_nr;
  uint32_t dbs_alloc;
} mapper;


typedef struct ref_data {
  strbuf *command;
  jedex_value *val;
} ref_data;


typedef int ( *config_fn ) ( const char *key, const char *value, void *user_data );
int read_config( const char *filename, config_fn fn, void *user_data );

// message.c
void request_save_topic_callback( jedex_value *val,
                                  const char *json,
                                  const char *client_id,
                                  void *user_data );
void request_find_record_callback( jedex_value *val,
                                   const char *json,
                                   const char *client_id,
                                   void *user_data );
void request_find_all_records_callback( jedex_value *val,
                                        const char *json,
                                        const char *client_id,
                                        void *user_data );
void request_find_next_record_callback( jedex_value *val,
                                        const char *json,
                                        const char *client_id,
                                        void *user_data );
void request_update_record_callback( jedex_value *val,
                                     const char *json,
                                     const char *client_id,
                                     void *user_data );
void request_delete_record_callback( jedex_value *val,
                                     const char *json,
                                     const char *client_id,
                                     void *user_data );
void request_insert_record_callback( jedex_value *val,
                                     const char *json,
                                     void *user_data );
typedef void ( *primary_key_fn ) ( key_info *kinfo, void *user_data ); 
typedef db_mapper_error ( *unpack_record_fn ) ( const char *tbl_name,
                                                const char *client_id,
                                                const char *json,
                                                jedex_value *val,
                                                mapper *self );

// init.c
mapper *mapper_initialize( mapper **mptr, int argc, char **argv );
void db_init( mapper *mptr );
int db_connect( mapper *mptr );
int db_create( mapper *mptr, bool hint );

// wrapper.c
int query( db_info *db, query_info *qinfo, const char *format, ...);
my_ulonglong query_num_rows( query_info *qinfo );
my_ulonglong query_affected_rows( db_info *db );
void query_free_result( query_info *qinfo );
int query_fetch_result( query_info *qinfo, strbuf *rbuf );


redisReply *cache_get( const char *key,
                       redisContext *rcontext );

redisContext *cache_connect( void );

// db_utils.c - access to db_mapper's internal dataabse information object
db_info *db_info_get( const char *name, mapper *self );
table_info *table_info_get( const char *name, db_info *db );
query_info *query_info_get( table_info *table );
query_info *query_info_get_by_client_id( table_info *table, const char *client_id );
void query_info_reset( query_info *qinfo );
void insert_clause( key_info *kinfo, void *user_data ); 
void where_clause( key_info *kinfo, void *user_data );
void redis_clause( key_info *kinfo, void *user_data );
void foreach_primary_key( table_info *tinfo, primary_key_fn fn, void *user_data );
const char * fld_type_str_to_sql( jedex_schema *fld_schema );
db_mapper_error set_table_name( const char *db_name, jedex_value *val, mapper *self );
const char *db_info_dbname( mapper *self, const char *tbl_name );

// redis cache wrapper access functions
void cache_set( redisContext *rcontext,
                table_info *tinfo,
                const char *json,
                ref_data *ref,
                strbuf *sb );

void cache_del( redisContext *rcontext,
                table_info *tinfo,
                ref_data *ref,
                strbuf *sb );
/*
 * jedex_utils.c - jedex object manipulation utility function to either extract
 * schema information or jedex value information.
 */
const char *table_name_get( const char *jscontent, jedex_value *val );
jedex_schema *table_schema_get( const char *tbl_name, jedex_schema *schema );
void get_string_field( jedex_value *val, const char *name, const char **field_name );
void db_reply_set( jedex_value *val, db_mapper_error err );


CLOSE_EXTERN
#endif // MAPPER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
