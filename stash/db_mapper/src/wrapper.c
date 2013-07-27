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


#include "db_mapper.h"
#include "mapper_priv.h"


static int
db_connect( db_info *db ) {
  db->db_handle = mysql_init( NULL );
  if ( mysql_real_connect( db->db_handle,
                           db->host,
                           db->user,
                           db->passwd,
                           NULL,
                           0,
                           db->socket,
                           0 ) == NULL ) {
    log_err( "Mysql error(%d): %s", mysql_errno( db->db_handle ), mysql_error( db->db_handle ) );
    printf( "Mysql error(%d): %s\n", mysql_errno( db->db_handle ), mysql_error( db->db_handle ) );
    return EINVAL;
  }
  log_info( "Connected to database %s", db->name );
  db->db_handle->reconnect = 0;

  return 0;
}


uint32_t
query( MYSQL *db_handle, const char *format, ...) {
  size_t size = 256;
  char stackbuffer[ size ];
  char *stmt_str = stackbuffer;
  va_list argptr;

  va_start( argptr, format );
  int required = vsnprintf( stmt_str, size, format, argptr );
  va_end( argptr );

  if ( required < 0 ) {
    printf( "Failed to format query\n" );
    return EINVAL;
  }
  size_t needed_size = ( size_t ) required;
  if ( needed_size >= size) {
    size = needed_size + 1;
    stmt_str = ( char * ) xmalloc( size );
    va_start( argptr, format );
    vsnprintf( stmt_str, size, format, argptr );
    va_end( argptr );
    needed_size = size;
  }

  if ( mysql_real_query( db_handle, stmt_str, needed_size ) ) {
    log_err( "Failed to execute the queury %s %s", stmt_str, mysql_error( db_handle ) );
    printf( "Failed to execute the queury %s %u:%s\n", stmt_str, mysql_errno( db_handle ), mysql_error( db_handle ) );
  }
  if ( stmt_str != stackbuffer ) {
    xfree( stmt_str );
  }

  return mysql_errno( db_handle );
}


int
connect_and_create_db( mapper *mapper ) {
  uint32_t i;
  uint32_t db_error = EINVAL;

  for ( i = 0; i < mapper->dbs_nr; i++ ) {
    db_info *db = mapper->dbs[ i ];
    if ( !db_connect( db ) ) {
      db_error = query( db->db_handle, "drop database if exists %s", db->name );
      if ( !db_error ) {
        db_error = query( db->db_handle, "create database %s character set 'utf8'", db->name );
      }
    }
  }

  return db_error == EINVAL ? -1 : 0;
}


  


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
