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
assign_db_value( const char *subkey, const char *value, db_info *db ) {
  if ( subkey ) {
    if ( !prefixcmp( subkey, ".host" ) ) {
      db->host = strdup( value );
    }
    else if ( !prefixcmp( subkey, ".user" ) ) {
      db->user = strdup( value );
    }
    else if ( !prefixcmp( subkey, ".passwd" ) ) {
      db->passwd = strdup( value );
    }
    else if ( !prefixcmp( subkey, ".socket" ) ) {
      db->socket = strdup( value );
    }
  }
}


static db_info *
make_db( const char *name, size_t len, mapper *self ) {
  uint32_t i;

  for ( i = 0; i < self->dbs_nr; i++ ) {
    if ( !strncmp( self->dbs[ i ]->name, name, len ) ) {
      return self->dbs[ i ];
    }
  }
  ALLOC_GROW( self->dbs, self->dbs_nr + 1, self->dbs_alloc );
  size_t nitems = 1;
  db_info *db = xcalloc( nitems, sizeof( db_info ) );
  self->dbs[ self->dbs_nr++ ] = db;
  if ( len ) {
    db->name = strndup( name, len );
  }
  else {
    db->name = xstrdup( name );
  }
  
  return db; 
}


static int
handle_config( const char *key, const char *value, void *user_data ) {
  assert( user_data );

  if ( !prefixcmp( key, "db_connection." ) ) {
    const char *subkey;
    const char *name;
    name = key + 14;
    subkey = strrchr( name, '.' );
    mapper *self = user_data;
    db_info *db = make_db( name, ( size_t ) ( subkey - name ), self );
    if ( subkey ) {
      assign_db_value( subkey, value, db );
    }
  }

  return 0;
}


/*
 * Parse command line arguments.
 * Read db_mapper's configuration file
 * Read schema using jedex_initialize
 * Initialize and connect to db.
 * Finally initialize emirates
 */
mapper *
mapper_initialize( int argc, char **argv, mapper **mptr ) {
  size_t nitems = 1;

  mapper *self = *mptr;
  mapper_args *args = xcalloc( nitems, sizeof( *args ) );
  parse_options( argc, argv, args );

  if ( args->config_fn == NULL || !strlen( args->config_fn ) ) {
    args->config_fn = "db_mapper.conf";
  }

  self = xcalloc( nitems, sizeof( mapper ) );
  if ( read_config( args->config_fn, handle_config, self ) < 0 ) {
    log_debug( "Failed to parse config file %s", args->config_fn );
    printf( "Failed to parse config file %s\n", args->config_fn );
  }
  //db_init( mapper );
  if ( db_connect( self ) ) {
    return NULL;
  }
  
  // do not create db if false
  bool hint = args->create_db;
  if ( db_create( self, hint ) ) {
    return NULL;
  }

  if ( args->schema_fn == NULL || !strlen( args->schema_fn ) ) {
    args->schema_fn = "test_schema";
  }
  self->schema = jedex_initialize( args->schema_fn );
  check_ptr_retval( self->schema, NULL, "Failed to initialize main schema" );

  if ( args->request_reply_schema_fn == NULL || !strlen( args->request_reply_schema_fn ) ) {
    args->request_reply_schema_fn = "request_reply";
  }
  self->request_reply_schema = jedex_initialize( args->request_reply_schema_fn );
  check_ptr_retval( self->request_reply_schema, NULL, "Failed to initialize request/reply schema" );

  // we don't need the command line arguments anymore
  xfree( args );

  jedex_schema *reply_schema = jedex_schema_get_subschema( self->request_reply_schema, "db_mapper_reply" );
  check_ptr_retval( reply_schema, NULL, "Failed to get db_mapper_reply schema" );

  jedex_value_iface *val_iface = jedex_generic_class_from_schema( reply_schema );
  self->reply_val = jedex_value_from_iface( val_iface );


  // connect to redis cache start the redis server
  struct timeval timeout = { 1, 0 }; // 1 second
  self->rcontext = redisConnectWithTimeout( "127.0.0.1", 6379, timeout );
  if ( self->rcontext == NULL || self->rcontext->err ) {
    if ( self->rcontext ) {
      log_err( "Connection to redis server failed error %s", self->rcontext->errstr ); 
      redisFree( self->rcontext );
      self->rcontext = NULL;
    }
    check_ptr_retval( self->rcontext, NULL, "Connection to redis server failed" );
  }
  
  
  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER | SUBSCRIBER ) );
  check_ptr_retval( self->emirates, NULL, "Failed to initialize emirates" );

  jedex_schema *request_schema = jedex_schema_get_subschema( self->request_reply_schema, "save_all" );
  check_ptr_retval( request_schema, NULL, "Failed to get request save_all schema" );
  self->emirates->set_service_request( self->emirates,
                                       "save_topic",
                                       request_schema,
                                       self,
                                       request_save_topic_callback );
  self->emirates->set_service_request( self->emirates,
                                       "find_record",
                                       self->schema,
                                       self,
                                       request_find_record_callback );
  self->emirates->set_service_request( self->emirates,
                                       "find_all_records",
                                       self->schema,
                                       self,
                                       request_find_all_records_callback );
  self->emirates->set_service_request( self->emirates,
                                       "find_next_record",
                                       self->schema,
                                       self,
                                       request_find_next_record_callback );
  self->emirates->set_service_request( self->emirates,
                                       "update_record",
                                       self->schema,
                                       self,
                                       request_update_record_callback );
  self->emirates->set_service_request( self->emirates,
                                       "delete_record",
                                       self->schema,
                                       self,
                                       request_delete_record_callback );
  set_ready( self->emirates );

  return self;
}


void
mapper_finalize( mapper **mptr ) {
  mapper *self = *mptr;
  emirates_finalize( &self->emirates );
  redisFree( self->rcontext );
  jedex_finalize( &self->request_reply_schema );
  jedex_finalize( &self->schema );
  for ( uint32_t i  = 0; i < self->dbs_nr; i++ ) {
    db_info *db = self->dbs[ i ];
    mysql_close( db->db_handle );
    xfree( db->name );
	}
  xfree( self );
}



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
