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


static void
free_mysql( struct mysql *m ) {
  if ( m->connection == true ) {
    mysql_close( &m->handler );
  }
  free( m );
}


void
db_init( mapper *mapper ) {
  db_wrapper *wrapper = xmalloc( sizeof( *wrapper ) );
  mapper->conn_handler = wrapper;

  
  struct mysql *mysql;
  mysql = xmalloc( sizeof( *mysql ) );
  wrapper->data = ( void * ) mysql;
  wrapper->free = ( void * ) free_mysql;
  mysql_init( &mysql->handler );
  mysql->connection = false;
}


int
db_connect( mapper *mapper, const mapper_args *args ) {
  struct mysql *my = DATA_PTR( mapper->conn_handler ); 
  MYSQL *m = &my->handler;

  if ( mysql_real_connect( m, args->db_host, args->db_user, args->db_passwd, args->db_name, 0, args->db_socket, 0 ) == NULL ) {
    log_err( "Mysql error(%d): %s", mysql_errno( m ), mysql_error( m ) );
    return EINVAL;
  }
  log_info( "Connected to database" );
  my->connection = true;
  m->reconnect = 0;

  return 0;
}



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
