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
    printf( "subkey %s name %s %d\n", subkey, name, subkey - name );
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
mapper_initialize( mapper **mptr, int argc, char **argv ) {
  size_t nitems = 1;

  mapper_args *args = xcalloc( nitems, sizeof( *args ) );
  parse_options( argc, argv, args );

  if ( args->config_fn == NULL || !strlen( args->config_fn ) ) {
    args->config_fn = "db_mapper.conf";
  }

  *mptr = xcalloc( nitems, sizeof( mapper ) );
  if ( read_config( handle_config, *mptr, args->config_fn ) < 0 ) {
    log_debug( "Failed to parse config file %s", args->config_fn );
    printf( "Failed to parse config file %s\n", args->config_fn );
  }
  //db_init( mapper );
  if ( db_connect( *mptr ) ) {
    return NULL;
  }
  
  // do not create db if false
  bool hint = false;
  if ( db_create( *mptr, hint ) ) {
    return NULL;
  }

  if ( args->schema_fn == NULL || !strlen( args->schema_fn ) ) {
    args->schema_fn = "test_schema";
  }
  ( *mptr )->schema = jedex_initialize( args->schema_fn );
  check_ptr_return( ( *mptr )->schema, "Failed to initialize main schema" );

  if ( args->request_schema_fn == NULL || !strlen( args->request_schema_fn ) ) {
    args->request_schema_fn = "save_topic";
  }
  ( *mptr )->request_schema = jedex_initialize( args->request_schema_fn );
  check_ptr_return( ( *mptr )->request_schema, "Failed to initialize request schema" );


  // connect to redis cache start the redis server
  struct timeval timeout = { 1, 0 }; // 1 second
  ( *mptr )->rcontext = redisConnectWithTimeout( "127.0.0.1", 6379, timeout );
  if ( ( *mptr )->rcontext == NULL || ( *mptr )->rcontext->err ) {
    if ( ( *mptr )->rcontext ) {
      log_err( "Connection to redis server failed error %s", ( *mptr )->rcontext->errstr ); 
      redisFree( ( *mptr )->rcontext );
      ( *mptr )->rcontext = NULL;
    }
    check_ptr_return( ( *mptr )->rcontext, "Connection to redis server failed" );
  }
  
  
  int flag = 0;
  ( *mptr )->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER | SUBSCRIBER ) );
  check_ptr_return( ( *mptr )->emirates, "Failed to initialize emirates" );

  ( *mptr )->emirates->set_service_request( ( *mptr )->emirates,
                                            "save_topic",
                                            ( *mptr )->request_schema,
                                            *mptr,
                                            request_save_topic_callback );
  sleep( 1 );
  set_ready( ( *mptr )->emirates );

  return *mptr;
}


void
mapper_finalize( mapper **mptr ) {
  emirates_finalize( &( *mptr )->emirates );
  redisFree( ( *mptr )->rcontext );
  jedex_finalize( &( *mptr )->request_schema );
  jedex_finalize( &( *mptr )->schema );
  for ( uint32_t i  = 0; i < ( *mptr )->dbs_nr; i++ ) {
    db_info *db = ( *mptr )->dbs[ i ];
    mysql_close( db->db_handle );
    xfree( db->name );
	}
}



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
