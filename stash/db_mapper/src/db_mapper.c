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
topic_subscription_callback( void *data ) {
  assert( data );
}


static void
request_save_topic_callback( jedex_value *val, const char *json, void *user_data ) {
  UNUSED( json );
  assert( user_data );
  assert( val );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "topic", &field, &index );
  const char *topic = NULL;
  size_t size = 0;
  jedex_value_get_string( &field, &topic, &size );
  mapper *mapper = user_data;
  const char *schemas[] = { NULL };
  mapper->emirates->set_subscription( mapper->emirates, topic, schemas, topic_subscription_callback );
}


static db_info *
make_db( mapper *mapper, const char *name, size_t len ) {
  uint32_t i;

  for ( i = 0; i < mapper->dbs_nr; i++ ) {
    if ( !strncmp( mapper->dbs[ i ]->name, name, len ) ) {
      return mapper->dbs[ i ];
    }
  }
  ALLOC_GROW( mapper->dbs, mapper->dbs_nr + 1, mapper->dbs_alloc );
  size_t nitems = 1;
  db_info *db = xcalloc( nitems, sizeof( db_info ) );
  mapper->dbs[ mapper->dbs_nr++ ] = db;
  if ( len ) {
    db->name = strndup( name, len );
  }
  else {
    db->name = xstrdup( name );
  }
  
  return db; 
}


static void
assign_db_value( db_info *db, const char *subkey, const char *value ) {
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


static int
handle_config( const char *key, const char *value, void *user_data ) {
  assert( user_data );

  if ( !prefixcmp( key, "db_connection." ) ) {
    const char *subkey;
    const char *name;
    name = key + 14;
    subkey = strrchr( name, '.' );
    mapper *mptr = user_data;
    db_info *db = make_db( mptr, name, ( size_t ) ( subkey - name ) );
    if ( subkey ) {
      assign_db_value( db, subkey, value );
    }
    printf( "subkey %s name %s %d\n", subkey, name, subkey - name );
  }

  return 0;
}


/*
 * Parse command line arguments.
 * Read schema using jedex_initialize
 * Initialize and connect to db.
 */
static mapper *
mapper_init( mapper **mptr, int argc, char **argv ) {
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
  if ( connect_and_create_db( *mptr ) ) {
    return NULL;
  }

  if ( args->schema_fn == NULL || strlen( args->schema_fn ) ) {
    args->schema_fn = "save_all_records";
  }
  ( *mptr )->request_schema = jedex_initialize( args->schema_fn );
  check_ptr_return( ( *mptr )->request_schema, "Failed to initialize jedex schema" );


  int flag = 0;
  ( *mptr )->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER | SUBSCRIBER ) );
  check_ptr_return( ( *mptr )->emirates, "Failed to initialize emirates " );

  ( *mptr )->emirates->set_service_request( ( *mptr )->emirates,
                                         "save_topic",
                                         ( *mptr )->request_schema,
                                         *mptr,
                                         request_save_topic_callback );
  sleep( 1 );
  set_ready( ( *mptr )->emirates );

  return *mptr;
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );

  mapper *mptr = NULL;
  mptr = mapper_init( &mptr, argc, argv );

  assert( mptr );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */