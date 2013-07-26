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


static db_info *
make_db( mapper *mapper, const char *name, size_t len ) {
  uint32_t i;

  for ( i = 0; i < mapper->dbs_nr; i++ ) {
    if ( !strncmp( mapper->dbs[ i ]->name, name, len ) ) {
      return mapper->dbs[ i ];
    }
  }
  ALLOC_GROW( mapper->dbs, mapper->dbs_nr + 1, mapper->dbs_alloc );
  db_info *db = xcalloc( 1, sizeof( db_info ) );
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
handle_config( mapper *mapper, const char *key, const char *value ) {
  if ( !prefixcmp( key, "db_connection." ) ) {
    const char *subkey;
    const char *name;
    name = key + 14;
    subkey = strrchr( name, '.' );
    db_info *db = make_db( mapper, name, ( size_t ) ( subkey - name ) );
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
static void
mapper_init( int argc, char **argv ) {
  const char *config_fn = "db_mapper.conf";

  mapper *mapper = xcalloc( 1, sizeof( mapper ) );
  if ( read_config( mapper, handle_config, config_fn ) < 0 ) {
    log_debug( "Failed to parse config file %s", config_fn );
    printf( "Failed to parse config file %s\n", config_fn );
  }
  mapper_args *args = xmalloc( sizeof( *args ) );
  parse_options( args, argc, argv );
  //db_init( mapper );
  db_connect( mapper );
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );

  mapper_init( argc, argv );
  // after initialization call emirates main loop
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
