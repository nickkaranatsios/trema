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


static char *
format_routine( const char *format, va_list params ) {
  char *buf;
  size_t bufsize = 32;

  buf = xmalloc( bufsize );
  vsnprintf( buf, bufsize, format, params );

  return buf;
}


static char *
format_ip( const char *format, ... ) {
  va_list params;
  char *buf;

  va_start( params, format );
  buf = format_routine( format, params );
  va_end( params );

  return buf;
}


void
port_free( port_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->ports_nr; i++ ) {
    xfree( tbl->ports[ i ] );
  }
  if ( tbl->ports_nr ) {
    xfree( tbl->ports );
  }
}


port *
port_select( int if_index, uint32_t ip_address, uint64_t mac_address, port_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->ports_nr; i++ ) {
    if ( tbl->ports[ i ]->if_index == if_index &&
         tbl->ports[ i ]->ip_address == ip_address &&
         tbl->ports[ i ]->mac_address == mac_address ) {
      return tbl->ports[ i ];
    }
  }

  return NULL;
}


port *
port_create( int if_index, uint32_t ip_address, uint64_t mac_address, port_table *tbl ) {
  port *p = port_select( if_index, ip_address, mac_address, tbl );
  if ( p ) {
    return p;
  }
  ALLOC_GROW( tbl->ports, tbl->ports_nr + 1, tbl->ports_alloc );
  size_t nitems = 1;
  p = xcalloc( nitems, sizeof( *p ) );
  p->if_index = if_index;
  p->ip_address = ip_address;
  p->mac_address = mac_address;
  tbl->ports[ tbl->ports_nr++ ] = p;

  return p;
}


void
cpu_free( cpu_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->cpus_nr; i++ ) {
    xfree( tbl->cpus[ i ] );
  }
  if ( tbl->cpus_nr ) {
    xfree( tbl->cpus );
  }
}


cpu *
cpu_create( uint32_t id, cpu_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->cpus_nr; i++ ) {
    if ( tbl->cpus[ i ]->id == id ) {
      return tbl->cpus[ i ];
    }
  }
  ALLOC_GROW( tbl->cpus, tbl->cpus_nr + 1, tbl->cpus_alloc );
  size_t nitems = 1;
  cpu *c = xcalloc( nitems, sizeof( *c ) );
  c->id = id;
  tbl->cpus[ tbl->cpus_nr++ ] = c;

  return c;
}


vm *
vm_select( uint32_t ip_address, vm_table *tbl ) {
  for( uint32_t i = 0; i < tbl->vms_nr; i++ ) {
    if ( tbl->vms[ i ]->ip_address == ip_address ) {
      return tbl->vms[ i ];
    }
  }

  return NULL;
}


void
vms_create( uint32_t pm_ip_address, pm_table *tbl, pm *pm ) {
  // check with config if the max_vm_count is within limits
  pm_spec *spec = pm_spec_select( pm_ip_address, tbl );
  if ( spec ) {
    if ( pm->max_vm_count > ( spec->vm_ip_address_end - spec->vm_ip_address_start ) ) {
      log_err( "Max vm count exceeded allowed limit %u", pm->max_vm_count );
      return;
    }
    vm_table *vm_tbl = &pm->vm_tbl;
    for ( uint32_t i = 0; i < pm->max_vm_count; i++ ) {
      ALLOC_GROW( vm_tbl->vms, vm_tbl->vms_nr + 1, vm_tbl->vms_alloc );
      size_t nitems = 1;
      vm *self = xcalloc( nitems, sizeof( *self ) );
      self->pm_ip_address = pm_ip_address;

      char *buf;
      buf = format_ip( spec->vm_ip_address_format, spec->vm_ip_address_start + i );
      self->ip_address = ip_address_to_i( buf );
      free( buf );

      buf = format_ip( spec->data_plane_ip_address_format, spec->vm_ip_address_start + i );
      self->data_plane_ip_address = ip_address_to_i( buf );
      free( buf );

      uint32_t igmp_address_prefix = ( uint32_t ) ( ( spec->igmp_group_address_start + i ) %  
        ( uint32_t ) ( spec->igmp_group_address_end - spec->igmp_group_address_start + 1 ) );
      buf = format_ip( spec->igmp_group_address_format,  igmp_address_prefix );
      self->igmp_group_address = ip_address_to_i( buf );
      free( buf );

      self->data_plane_mac_address = spec->data_plane_mac_address | ( uint8_t ) ( i & 0xff );
      vm_tbl->vms[ vm_tbl->vms_nr++ ] = self;
    }
  }
}


void
vm_free( vm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->vms_nr; i++ ) {
    service_table *s_tbl = &tbl->vms[ i ]->service_tbl;
    service_free( s_tbl );
    port_table *p_tbl = &tbl->vms[ i ]->port_tbl;
    port_free( p_tbl );
    cpu_table *c_tbl = &tbl->vms[ i ]->cpu_tbl;
    cpu_free( c_tbl );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
