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


static void
param_stat_free( param_stats_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->param_stats_nr; i++ ) {
    param_stat *ps = tbl->param_stats[ i ];
    xfree( ps->name );
    xfree( ps );
  }
  if ( tbl->param_stats_nr ) {
    xfree( tbl->param_stats );
  }
}



static service_spec *
service_spec_find( const char *key, size_t key_len, service_class_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    if ( !strncmp( tbl->service_specs[ i ]->key, key, key_len ) ) {
      return tbl->service_specs[ i ];
    }
  }

  return NULL;
}


void
service_spec_free( service_class_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    service_spec *spec = tbl->service_specs[ i ];
    xfree( spec->key );
    xfree( spec );
  }
  if ( tbl->service_specs_nr ) {
    xfree( tbl->service_specs );
  }
}


service_spec *
service_spec_select( const char *key, service_class_table *tbl ) {
  return service_spec_find( key, strlen( key ), tbl );
}


service_spec *
service_spec_create( const char *key, size_t key_len, service_class_table *tbl ) {
  service_spec *spec = service_spec_find( key, key_len, tbl );
  if ( spec ) {
    return spec;
  }
  ALLOC_GROW( tbl->service_specs, tbl->service_specs_nr + 1, tbl->service_specs_alloc );
  size_t nitems = 1;
  service_spec *s_spec = xcalloc( nitems, sizeof( *s_spec ) );
  s_spec->key = strdup( key );
  s_spec->key[ key_len ] = '\0';
  tbl->service_specs[ tbl->service_specs_nr++ ] = s_spec;

  return s_spec;
}



static service *
service_select( const char *name, service_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->services_nr; i++ ) {
    if ( !strncmp( name, tbl->services[ i ]->name, strlen( tbl->services[ i ]->name ) ) ) {
      return tbl->services[ i ];
    }
  }
  
  return NULL;
}


static param_stat *
param_stat_select( const char *name, param_stats_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->param_stats_nr; i++ ) {
    if ( !strncmp( name, tbl->param_stats[ i ]->name, strlen( tbl->param_stats[ i ]->name ) ) ) {
      return tbl->param_stats[ i ];
    }
  }

  return NULL;
}

void
service_spec_set( const char *subkey, const char *value, service_spec *spec ) {
  int base = 0;

  if ( !prefixcmp( subkey, ".cpu_count" ) ) {
    spec->cpu_count = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".memory_size" ) ) {
    spec->memory_size = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".user_count" ) ) {
    spec->user_count = ( uint32_t ) strtol( value, NULL, base );
  }
}


param_stat *
param_stat_create( const char *name, param_stats_table *tbl ) {
  param_stat *stat = param_stat_select( name, tbl );
  if ( stat != NULL ) {
    return stat;
  }
  ALLOC_GROW( tbl->param_stats, tbl->param_stats_nr + 1, tbl->param_stats_alloc );
  stat = xmalloc( sizeof( *stat ) );
  stat->name = strdup( name );
  tbl->param_stats[ tbl->param_stats_nr++ ] = stat;
  
  return stat;
}


void
service_free( service_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->services_nr; i++ ) {
    xfree( tbl->services[ i ]->name );
    param_stats_table *ps_tbl = &tbl->services[ i ]->param_stats_tbl;
    param_stat_free( ps_tbl );
    xfree( tbl->services[ i ] );
  }
  if ( tbl->services_nr ) {
    xfree( tbl->services );
  }
}


service *
service_create( const char *name, service_table *tbl ) {
  service *self = service_select( name, tbl );
  if ( self != NULL ) {
    return self;
  }
  ALLOC_GROW( tbl->services, tbl->services_nr + 1, tbl->services_alloc );
  self = xmalloc( sizeof( *self ) );
  self->name = strdup( name );
  tbl->services[ tbl->services_nr++ ] = self;

  return self;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
