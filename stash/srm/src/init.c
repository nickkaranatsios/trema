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


#include "system_resource_manager.h"
#include "system_resource_manager_priv.h"


static void
pm_spec_set( const char *subkey, const char *value, pm_spec *spec ) {
  if ( !prefixcmp( subkey, ".igmp_group_address_format" ) ) {
    spec->igmp_group_address_format = strdup( value );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_start" ) ) {
    spec->igmp_group_address_start = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_end " ) ) {
    spec->igmp_group_address_end = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_format" ) ) {
    spec->vm_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_start" ) ) {
    spec->vm_ip_address_start = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_end" ) ) {
    spec->vm_ip_address_end = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".data_plane_ip_address_format" ) ) {
    spec->data_plane_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".data_plane_mac_address" ) ) {
    int base = 0;
    spec->data_plane_mac_address = ( uint64_t ) strtoll( value, NULL, base );
  }
}


static void
vm_spec_set( const char *subkey, const char *value, vm_spec *spec ) {
  int base;

  if ( !prefixcmp( subkey, ".cpu_count" ) ) {
    spec->cpu_count = ( uint16_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".memory_size" ) ) {
    spec->memory_size = ( uint32_t ) atoi( value );
  }
  else if ( !prefixcmp( subkey, ".user_count" ) ) {
    spec->user_count = ( uint32_t ) strtol( value, NULL, base );
  }
}


static pm_spec *
pm_spec_create( const char *key, size_t key_len, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    if ( !strncmp( tbl->pm_specs[ i ]->key, key, key_len ) ) {
      return tbl->pm_specs[ i ];
    }
  }

  ALLOC_GROW( tbl->pm_specs, tbl->pm_specs_nr + 1, tbl->pm_specs_alloc );
  size_t nitems = 1;
  pm_spec *spec = xcalloc( nitems, sizeof( *spec ) );
  spec->key = strdup( key );
  spec->key[ key_len ] = '\0';
  tbl->pm_specs[ tbl->pm_specs_nr++ ] = spec;

  return spec;
}


static vm_spec *
vm_spec_create( const char *key, size_t key_len, vm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->vm_specs_nr; i++ ) {
    if ( !strncmp( tbl->vm_specs[ i ]->key, key, key_len ) ) {
      return tbl->vm_specs[ i ];
    }
  }

  ALLOC_GROW( tbl->vm_specs, tbl->vm_specs_nr + 1, tbl->vm_specs_alloc );
  size_t nitems = 1;
  vm_spec *spec = xcalloc( nitems, sizeof( *spec ) );
  spec->key = strdup( key );
  spec->key[ key_len ] = '\0';
  tbl->vm_specs[ tbl->vm_specs_nr++ ] = spec;

  return spec;
}

static int
handle_config( const char *key, const char *value, void *user_data ) {
  pm_table *tbl = user_data;
  const char *subkey;
  const char *name;

  // [physical_machine_spec "172.16.4.2"]
  const char *keyword = "physical_machine_spec.";
  if ( !prefixcmp( key, keyword ) ) {
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );

    // for each physical_machine_spec keyword found we create one record
    pm_spec *spec = pm_spec_create( name, ( size_t ) ( subkey - name ), tbl );
    if ( spec && subkey ) {
      pm_spec_set( subkey, value, spec );
    }
  }
  keyword = "virtual_machine_spec.";
  // [virtual_machine_spec "172.16.4.2"]
  if ( !prefixcmp( key, keyword ) ) {
    name = key + strlen( keyword );
    subkey = strrchr( name, '.' );
    // find the corresponding pm_spec 
    pm_spec *spec = pm_spec_create( name, ( size_t ) ( subkey - name - 2 ), tbl );
    if ( spec && subkey ) {
      vm_spec *vm_spec = vm_spec_create( name, ( size_t ) ( subkey - name ), &spec->vm_tbl );
      if ( vm_spec ) {
        vm_spec_set( subkey, value, vm_spec );
      }
    }
  }

  return 0;
}


system_resource_manager *
system_resource_manager_initialize( int argc, char **argv, system_resource_manager **srm_ptr ) {
  size_t nitems = 1;

  system_resource_manager_args *args = xcalloc( nitems, sizeof( *args ) );
  args->progname = basename( argv[ 0 ] );
  parse_options( argc, argv, args );

  if ( args->config_fn == NULL || !strlen( args->config_fn ) ) {
    args->config_fn = "srm.conf";
  }
  if ( args->schema_fn == NULL || !strlen( args->schema_fn ) ) {
    args->schema_fn = "ipc";
  }
  if ( args->daemonize ) {
    // only check for root priviledges if daemonize
    if ( geteuid() != 0 ) {
      printf( "%s must be run with root privilege.\n", args->progname );
      return NULL;
    }
    if ( pid_file_exists( args->progname ) ) {
      log_err( "Another %s is running", args->progname );
      return NULL;
    }
    bool ret = create_pid_file( args->progname );
    if ( !ret ) {
      log_err( "Failed to create a pid file." );
      return NULL;
    }
  }

  system_resource_manager *self = *srm_ptr;
  self = xcalloc( nitems, sizeof( *self ) );
  
  // load/read the system resource manager configuration file.
  read_config( args->config_fn, handle_config, &self->pm_tbl );

  /*
   * read in all the schema information that the system resource manager
   * would use. Start off with the main schema and proceed to sub schemas.
   */
  self->schema = jedex_initialize( args->schema_fn );

  return self;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
