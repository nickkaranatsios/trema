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


int
db_connect( db_info *db ) {
  db->db_handle = mysql_init( NULL );
  if ( mysql_real_connect( db->db_handle, 
                             db->host,
                             db->user,
                             db->passwd,
                             db->name,
                             0,
                             db->socket, 0 ) == NULL ) {
    log_err( "Mysql error(%d): %s", mysql_errno( db->db_handle ), mysql_error( db->db_handle ) );
    return EINVAL;
  }
  log_info( "Connected to database %s", db->name );
  db->db_handle->reconnect = 0;

  return 0;
}


int
connect_and_create_db( mapper *mapper ) {
  uint32_t i;

  for ( i = 0; i < mapper->dbs_nr; i++ ) {
    db_info *db = mapper->dbs[ i ];
    if ( !db_connect( mapper ) ) {
      query( db->db_handle, "create database %s", db->name );
    }
  }
}


  


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
