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
pm_info_publish( system_resource_manager *self ) {
  pm_table *tbl = &self->pm_tbl;
  jedex_value *rval = jedex_value_find( "physical_machine_info", self->rval );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( rval, "ip_address", &field, &index );

  for ( uint32_t i = 0; i < tbl->pm_specs_nr; i++ ) {
    jedex_value_set_int( &field, ( int32_t ) tbl->pm_specs[ i ]->ip_address ); 
    self->emirates->publish_value( self->emirates, PM_INFO_PUBLISH, rval );
  }
}


static void
stats_collect_request_to_sc( system_resource_manager *self ) {
  jedex_value *rval = jedex_value_find( "nc_statistics_status_request", self->rval );
  self->emirates->send_request( self->emirates, STATS_COLLECT, rval, self->schema );
}


#ifdef LATER
void
dispatch_service_to_sc( int operation, jedex_value *val, system_resource_manager *self ) {
  if ( val && val->iface ) {
    jedex_value field;
    size_t index;

    jedex_value_get_by_name( val, "service_name", &field, &index );
    const char *service;
    size_t size;
    jedex_value_get_string( &field, &service, &size );

    double bandwidth;
    jedex_value_get_by_name( val, "band_width", &field, &index ); 
    jedex_value_get_double( &field, &bandwidth );

    uint64_t subscribers;
    jedex_value_get_by_name( val, "subscribers", &field, &index );
    jedex_value_set_long( &field, ( int64_t ) &subscribers );
  }
}
#endif

static pm *
pm_select( jedex_value *stats, pm_table *tbl ) {
   jedex_value field;

   jedex_value_get_by_name( stats, "pm_ip_address", &field, NULL );
   int tmp;
   jedex_value_get_int( &field, &tmp );
   uint32_t ip_address = ( uint32_t ) tmp;

   for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
     if ( tbl->pms[ i ]->ip_address == ip_address ) {
       return tbl->pms[ i ];
     }
   }
   log_err( "Failed to locate pm ip address in local db %u\n", ip_address );

   return NULL;
}


static vm *
vm_create( uint32_t ip_address, vm_table *tbl ) {
  for( uint32_t i = 0; i < tbl->vms_nr; i++ ) {
    if ( tbl->vms[ i ]->ip_address == ip_address ) {
      return tbl->vms[ i ];
    }
  }

  return NULL;
}


static vm *
vm_select( jedex_value *stats, pm_table *tbl ) {
  pm *pm = pm_select( stats, tbl );

  if ( pm != NULL ) {
    jedex_value field;

    jedex_value_get_by_name( stats, "vm_ip_address", &field, NULL );
    int tmp;
    jedex_value_get_int( &field, &tmp );
    uint32_t ip_address = ( uint32_t ) tmp;
   
    return vm_create( ip_address, &pm->vm_tbl );
  }

  return NULL;
}


static port *
port_create( int if_index, uint32_t ip_address, uint64_t mac_address, port_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->ports_nr; i++ ) {
    if ( tbl->ports[ i ]->if_index == if_index &&
         tbl->ports[ i ]->ip_address == ip_address &&
         tbl->ports[ i ]->mac_address == mac_address ) {
      return tbl->ports[ i ];
    }
  }
  ALLOC_GROW( tbl->ports, tbl->ports_nr + 1, tbl->ports_alloc );
  size_t nitems = 1;
  port *p = xcalloc( nitems, sizeof( *p ) );
  p->if_index = if_index;
  p->ip_address = ip_address;
  p->mac_address = mac_address;
  tbl->ports[ tbl->ports_nr++ ] = p;

  return p;
}



static cpu *
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


static void
pm_port_stats_reset( pm_table *tbl ) {
  /*
   * clear the status of all ports to zero effectively reseting stats for
   * each port.
   */
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    pm *pm = tbl->pms[ i ];
    port_table *tbl = &pm->port_tbl;
    for ( uint32_t j = 0; j < tbl->ports_nr; j++ ) {
      tbl->ports[ j ]->status = 0;
    }
  }
}


/*
 * A port may be added or removed go up or down.
 */
static void
pm_port_stats_create( jedex_value *stats, pm_table *tbl ) {
  pm *self = pm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "if_index", &field, NULL );
    int if_index;
    jedex_value_get_int( &field, &if_index );

    jedex_value_get_by_name( stats, "ip_address", &field, NULL );
    int tmp;
    jedex_value_get_int( &field, &tmp );
    uint32_t ip_address = ( uint32_t ) tmp;

    jedex_value_get_by_name( stats, "mac_address", &field, NULL );
    int64_t tmp_long;
    jedex_value_get_long( &field, &tmp_long );
    uint64_t mac_address = ( uint64_t ) tmp_long;

    port *p = port_create( if_index, ip_address, mac_address, &self->port_tbl );
    if ( p ) {
       p->status = 1;
      jedex_value_get_by_name( stats, "if_speed", &field, NULL );
      jedex_value_get_int( &field, &p->if_speed );

      jedex_value_get_by_name( stats, "tx_byte", &field, NULL );
      jedex_value_get_long( &field, &tmp_long );
      p->tx_byte = ( uint64_t ) tmp_long;

      jedex_value_get_by_name( stats, "rx_byte", &field, NULL );
      jedex_value_get_long( &field, &tmp_long );
      p->rx_byte = ( uint64_t ) tmp_long;
    }
  }
}


static void
pm_cpu_stats_reset( pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    pm *pm = tbl->pms[ i ];
    cpu_table *tbl = &pm->cpu_tbl;
    for ( uint32_t j = 0; j < tbl->cpus_nr; j++ ) {
      tbl->cpus[ j ]->status = 0;
    }
  }
}


static void
pm_cpu_stats_create( jedex_value *stats, pm_table *tbl ) {
  pm *self = pm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "id", &field, NULL );

    int tmp;
    jedex_value_get_int( &field, &tmp );
    // first reported cpu load starts from 1 hence subtract 1 to index.
    uint32_t id = ( uint32_t ) ( tmp - 1 );
    cpu *c = cpu_create( id, &self->cpu_tbl );
    if ( c ) {
      jedex_value_get_by_name( stats, "load", &field, NULL );
      jedex_value_get_double( &field, &c->load );
      c->status = 1;
    }
  }
}


static void
vm_cpu_stats_create( jedex_value *stats, pm_table *tbl ) {
  vm *self = vm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "id", &field, NULL );
    int tmp;
    jedex_value_get_int( &field, &tmp );
    // first reported cpu load starts from 1 hence subtract 1 to index.
    uint32_t id = ( uint32_t ) ( tmp - 1 );
    cpu *c = cpu_create( id, &self->cpu_tbl );
    if ( c ) {
      jedex_value_get_by_name( stats, "load", &field, NULL );
      jedex_value_get_double( &field, &c->load );
      c->status = 1;
    }
  }
}


static void
pm_stats_reset( pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    tbl->pms[ i ]->status = 0;
  }
}

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


static void
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
      buf = format_ip( spec->data_plane_ip_address_format, spec->vm_ip_address_start + i );
      self->data_plane_ip_address = ip_address_to_i( buf );
      free( buf );
      self->data_plane_mac_address = spec->data_plane_mac_address | ( uint8_t ) ( i & 0xff );
      vm_tbl->vms[ vm_tbl->vms_nr++ ] = self;
    }
  }
}


/*
 * Here we need to create the vms for calculated from the pm->cpu_count field.
 */
static void
pm_stats_create( jedex_value *stats, pm_table *tbl ) {
  pm *self = pm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "port_count", &field, NULL );

    int tmp;
    jedex_value_get_int( &field, &tmp );
    self->port_count = ( uint16_t ) tmp;

    jedex_value_get_by_name( stats, "cpu_count", &field, NULL );
    jedex_value_get_int( &field, &tmp );
    self->cpu_count = ( uint16_t ) tmp;

    // this is the first time we receive stats for this pm
    if ( !self->max_vm_count ) {
      // calculate the max allowed vms for this pm.
      self->max_vm_count = ( uint16_t ) ( self->cpu_count - 1 );

      jedex_value_get_by_name( stats, "pm_ip_address", &field, NULL );
      jedex_value_get_int( &field, &tmp );
      uint32_t pm_ip_address = ( uint32_t ) tmp;

      // create max_mv_count vms
      vms_create( pm_ip_address, tbl, self );
    }

    jedex_value_get_by_name( stats, "vm_count", &field, NULL );
    jedex_value_get_int( &field, &tmp );
    self->active_vm_count = ( uint32_t ) tmp;

    int64_t tmp_long;
    jedex_value_get_by_name( stats, "memory_size", &field, NULL );
    jedex_value_get_long( &field, &tmp_long );
    self->memory_size = ( uint64_t ) tmp_long;

    jedex_value_get_by_name( stats, "available_memory", &field, NULL );
    jedex_value_get_long( &field, &tmp_long );
    self->avail_memory = ( uint64_t ) tmp_long;
    self->status = 1;
  }
}


static void
vm_stats_create( jedex_value *stats, pm_table *tbl ) {
  vm *self = vm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "port_count", &field, NULL );

    int tmp;
    jedex_value_get_int( &field, &tmp );
    self->port_count = ( uint16_t ) tmp;

    jedex_value_get_by_name( stats, "cpu_count", &field, NULL );
    jedex_value_get_int( &field, &tmp );
    self->cpu_count = ( uint16_t ) tmp;

    int64_t tmp_long;
    jedex_value_get_by_name( stats, "total_memory", &field, NULL );
    jedex_value_get_long( &field, &tmp_long );
    self->total_memory = ( uint64_t ) tmp_long;

    jedex_value_get_by_name( stats, "available_memory", &field, NULL );
    jedex_value_get_long( &field, &tmp_long );
    self->avail_memory = ( uint64_t ) tmp_long;

    jedex_value_get_by_name( stats, "service_count", &field, NULL );
    jedex_value_get_int( &field, &tmp );
    self->service_count = ( uint32_t ) tmp;
  }
}


static void
vm_port_stats_create( jedex_value *stats, pm_table *tbl ) {
  vm *self = vm_select( stats, tbl );
  if ( self != NULL ) {
    jedex_value field;
    jedex_value_get_by_name( stats, "if_index", &field, NULL );
    int if_index;
    jedex_value_get_int( &field, &if_index );

    jedex_value_get_by_name( stats, "ip_address", &field, NULL );
    int tmp;
    jedex_value_get_int( &field, &tmp );
    uint32_t ip_address = ( uint32_t ) tmp;

    jedex_value_get_by_name( stats, "mac_address", &field, NULL );
    int64_t tmp_long;
    jedex_value_get_long( &field, &tmp_long );
    uint64_t mac_address = ( uint64_t ) tmp_long;

    port *p = port_create( if_index, ip_address, mac_address, &self->port_tbl );
    if ( p ) {
       p->status = 1;
      jedex_value_get_by_name( stats, "if_speed", &field, NULL );
      jedex_value_get_int( &field, &p->if_speed );

      jedex_value_get_by_name( stats, "tx_byte", &field, NULL );
      jedex_value_get_long( &field, &tmp_long );
      p->tx_byte = ( uint64_t ) tmp_long;

      jedex_value_get_by_name( stats, "rx_byte", &field, NULL );
      jedex_value_get_long( &field, &tmp_long );
      p->rx_byte = ( uint64_t ) tmp_long;
    }
  }
}

static service *
service_create( const char *name, service_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->services_nr; i++ ) {
    if ( !strncmp( name, tbl->services[ i ]->name, strlen( tbl->services[ i ]->name ) ) ) {
      return tbl->services[ i ];
    }
  }
  ALLOC_GROW( tbl->services, tbl->services_nr + 1, tbl->services_alloc );
  service *self = xmalloc( sizeof( *self ) );
  self->name = strdup( name );
  tbl->services[ tbl->services_nr++ ] = self;

  return self;
}
 

static param_stat *
param_stats_create( const char *name, param_stats_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->param_stats_nr; i++ ) {
    if ( !strncmp( name, tbl->param_stats[ i ]->name, strlen( tbl->param_stats[ i ]->name ) ) ) {
      return tbl->param_stats[ i ];
    }
  }
  ALLOC_GROW( tbl->param_stats, tbl->param_stats_nr + 1, tbl->param_stats_alloc );
  param_stat *stat = xmalloc( sizeof( *stat ) );
  stat->name = strdup( name );
  tbl->param_stats[ tbl->param_stats_nr++ ] = stat;
  
  return stat;
}


static void
vm_service_stats_create( jedex_value *stats, pm_table *tbl ) {
  vm *self = vm_select( stats, tbl );
  if ( self ) {
    service_table *service_tbl = &self->service_tbl;
    jedex_value field;

    jedex_value_get_by_name( stats, "service_name", &field, NULL );
    const char *name;
    size_t size;
    jedex_value_get_string( &field, &name, &size );
    service *s = service_create( name, service_tbl );

    jedex_value_get_by_name( stats, "param_stats", &field, NULL );

    size_t array_size;
    jedex_value_get_size( &field, &array_size );
    for ( size_t i = 0; i < array_size; i++ ) {
      jedex_value element;
      jedex_value_get_by_index( &field, i, &element, NULL );
      jedex_value ary_field;
      jedex_value_get_by_name( &element, "param_name", &ary_field, NULL );
      const char *param_name;
      jedex_value_get_string( &ary_field, &param_name, &size );
      param_stat *ps = param_stats_create( param_name, &s->param_stats_tbl );

      jedex_value_get_long( &ary_field, &ps->value );
    }
  }
}


void
foreach_stats( jedex_value *stats, stats_fn stats_fn, pm_table *tbl ) {
  size_t array_size;

  jedex_value_get_size( stats, &array_size );
  for ( size_t i = 0; i < array_size; i++ ) {
    jedex_value element;

    jedex_value_get_by_index( stats, i, &element, NULL );
    stats_fn( &element, tbl );
  }
}


void
stats_collect_handler( const uint32_t tx_id,
                       jedex_value *val,
                       const char *json,
                       void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );

  printf( "stats collect handler %s\n", json );
  system_resource_manager *self = user_data;
  
  jedex_value rval;
  size_t index;

  jedex_value_get_by_name( val, "statistics_status_reply", &rval, &index );
  
  jedex_value pm_stats_ary;
  jedex_value_get_by_name( &rval, "pm_stats", &pm_stats_ary, &index );

  jedex_value pm_cpu_stats_ary;
  jedex_value_get_by_name( &rval, "pm_cpu_stats", &pm_cpu_stats_ary, &index );

  jedex_value pm_port_stats_ary;
  jedex_value_get_by_name( &rval, "pm_port_stats", &pm_port_stats_ary, &index );

  jedex_value vm_stats_ary;
  jedex_value_get_by_name( &rval, "vm_stats", &vm_stats_ary, &index );

  jedex_value vm_port_stats_ary;
  jedex_value_get_by_name( &rval, "vm_port_stats", &vm_port_stats_ary, &index );

  jedex_value vm_cpu_stats_ary;
  jedex_value_get_by_name( &rval, "vm_cpu_stats", &vm_cpu_stats_ary, &index );

  jedex_value service_stats_ary;
  jedex_value_get_by_name( &rval, "service_stats", &service_stats_ary, &index );
  
  pm_stats_reset( &self->pm_tbl );
  foreach_stats( &pm_stats_ary, pm_stats_create, &self->pm_tbl );
  pm_cpu_stats_reset( &self->pm_tbl );
  foreach_stats( &pm_cpu_stats_ary, pm_cpu_stats_create, &self->pm_tbl );
  pm_port_stats_reset( &self->pm_tbl );
  foreach_stats( &pm_port_stats_ary, pm_port_stats_create, &self->pm_tbl );

  foreach_stats( &vm_stats_ary, vm_stats_create, &self->pm_tbl );
  foreach_stats( &vm_port_stats_ary, vm_port_stats_create, &self->pm_tbl );
  foreach_stats( &vm_cpu_stats_ary, vm_cpu_stats_create, &self->pm_tbl );
  foreach_stats( &service_stats_ary, vm_service_stats_create, &self->pm_tbl );
  
#ifdef TEST
  uint32_t n_vms = compute_n_vms( INTERNET_ACCESS_SERVICE, 0, 250, &self->pm_tbl );
  uint32_t n_vms = compute_n_vms( VIDEO_SERVICE, 0, 100, &self->pm_tbl );
  assert( n_vms == 0 );
#endif
}


void
vm_allocate_handler( const uint32_t tx_id,
                     jedex_value *val,
                     const char *json,
                     void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( user_data );

  printf( "vm allocate handler %s\n", json );
}


void
service_delete_handler( const uint32_t tx_id,
                        jedex_value *val,
                        const char *json,
                        void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( user_data );

  printf( "service delete handler %s\n", json );
}


void
oss_bss_add_service_handler( jedex_value *val,
                             const char *json,
                             const char *client_id,
                             void *user_data ) {

  UNUSED( json );
  UNUSED( client_id );
  UNUSED( val );
  UNUSED( user_data );
#ifdef LATER
  system_resource_manager *self = user_data;
  dispatch_service_to_sc( ADD, val, self );
#endif
}


void
periodic_timer_handler( void *user_data ) {
  system_resource_manager *self = user_data;
  // assert( self != NULL );

  if ( !self->should_publish ) {
    pm_info_publish( self );
    self->should_publish = 1;
  }
  else {
    // send statistics request to service_controller
    stats_collect_request_to_sc( self );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */