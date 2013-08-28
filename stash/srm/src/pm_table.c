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
pm_create( uint32_t ip_address, pm_table *tbl ) {
  ALLOC_GROW( tbl->pms, tbl->pms_nr + 1, tbl->pms_alloc );
  size_t nitems = 1;
  pm *self = xcalloc( nitems, sizeof( *self ) );
  self->ip_address = ip_address;
  tbl->pms[ tbl->pms_nr++ ] = self;
}


uint32_t
ip_address_to_i( const char *ip_address_s ) {
  struct in_addr in_addr;
  uint32_t ip_address;
  
  if ( inet_aton( ip_address_s, &in_addr ) != 0 ) {
    ip_address = ntohl( in_addr.s_addr );
  }
  else {
    ip_address = 0;
  }
  
  return ip_address;
}


pm_spec *
pm_spec_create( const char *key, size_t key_len, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    if ( !strncmp( tbl->pm_specs[ i ]->key, key, key_len ) ) {
      return tbl->pm_specs[ i ];
    }
  }
  ALLOC_GROW( tbl->pm_specs, tbl->pm_specs_nr + 1, tbl->pm_specs_alloc );
  size_t nitems = 1;
  pm_spec *spec = xcalloc( nitems, sizeof( *spec ) );
  tbl->selected_pm = tbl->pm_specs_nr;
  spec->key = strdup( key );
  spec->key[ key_len ] = '\0';
  spec->ip_address = ip_address_to_i( spec->key );
  tbl->pm_specs[ tbl->pm_specs_nr++ ] = spec;
  pm_create( spec->ip_address, tbl );

  return spec;
}


void
pm_spec_set( const char *subkey, const char *value, pm_spec *spec ) {
  int base = 0;
  if ( !prefixcmp( subkey, ".igmp_group_address_format" ) ) {
    spec->igmp_group_address_format = strdup( value );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_start" ) ) {
    spec->igmp_group_address_start = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".igmp_group_address_end" ) ) {
    spec->igmp_group_address_end = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_format" ) ) {
    spec->vm_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_start" ) ) {
    spec->vm_ip_address_start = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".vm_ip_address_end" ) ) {
    spec->vm_ip_address_end = ( uint16_t ) strtol( value, NULL, base );
  }
  else if ( !prefixcmp( subkey, ".data_plane_ip_address_format" ) ) {
    spec->data_plane_ip_address_format = strdup( value ); 
  }
  else if ( !prefixcmp( subkey, ".data_plane_mac_address" ) ) {
    spec->data_plane_mac_address = ( uint64_t ) strtoll( value, NULL, base );
  }
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

static service_spec *
service_spec_find( const char *key, size_t key_len, service_class_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    if ( !strncmp( tbl->service_specs[ i ]->key, key, key_len ) ) {
      return tbl->service_specs[ i ];
    }
  }

  return NULL;
}


static void
service_spec_free( service_class_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->service_specs_nr; i++ ) {
    service_spec *spec = tbl->service_specs[ i ];
    xfree( spec->key );
  }
  xfree( tbl->service_specs );
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


pm_spec *
pm_spec_select( const uint32_t ip_address, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    if ( tbl->pm_specs[ i ]->ip_address == ip_address ) {
      return tbl->pm_specs[ i ];
    }
  }

  return NULL;
}


void
pm_table_finalize( pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    pm_spec *pm_spec = tbl->pm_specs[ i ];
    service_class_table scls_tbl = pm_spec->service_class_tbl;
    service_spec_free( &scls_tbl );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
