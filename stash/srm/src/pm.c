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


pm *
pm_select( const uint32_t ip_address, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    if ( tbl->pms[ i ]->ip_address == ip_address ) {
      return tbl->pms[ i ];
    }
  }
  log_err( "Failed to locate pm ip address in local db %u\n", ip_address );

  return NULL;
}


void
pm_create( uint32_t ip_address, pm_table *tbl ) {
  ALLOC_GROW( tbl->pms, tbl->pms_nr + 1, tbl->pms_alloc );
  size_t nitems = 1;
  pm *self = xcalloc( nitems, sizeof( *self ) );
  self->ip_address = ip_address;
  tbl->pms[ tbl->pms_nr++ ] = self;
}


void
pm_free( pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    cpu_table *c_tbl = &tbl->pms[ i ]->cpu_tbl;
    cpu_free( c_tbl );
    port_table *p_tbl = &tbl->pms[ i ]->port_tbl;
    port_free( p_tbl );
    vm_table *v_tbl = &tbl->pms[ i ]->vm_tbl;
    vm_free( v_tbl );
    xfree( tbl->pms[ i ] );
  }
  if ( tbl->pms_nr ) {
    xfree( tbl->pms );
  }
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
pm_spec_free( pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    pm_spec *spec = tbl->pm_specs[ i ];
    service_spec_free( &spec->service_class_tbl );
    xfree( spec->key );
    xfree( spec->igmp_group_address_format );
    xfree( spec->data_plane_ip_address_format );
    xfree( spec->vm_ip_address_format );
    xfree( spec );
  }  
  if ( tbl->pm_specs_nr ) {
    xfree( tbl->pm_specs );
  }
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
pm_table_free( pm_table *tbl ) {
  pm_spec_free( tbl );
  pm_free( tbl );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
