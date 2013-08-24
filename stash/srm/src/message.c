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
   bool found = false;
   /* 
    * first time we receive stats for this pm check if the ip address matches
    * the ip address of the configuration file.
   */
   for ( uint32_t i = 0; i < tbl->pm_specs_nr && found == false; i++ ) {
     if ( tbl->pm_specs[ i ]->ip_address == ip_address ) {
       found = true; 
     }
   }
   if ( found == false ) {
     log_err( "Failed to locate pm ip address in local db %u\n", ip_address );
     return NULL;
   }
   ALLOC_GROW( tbl->pms, tbl->pms_nr + 1, tbl->pms_alloc );
   size_t nitems = 1;
   pm *self = xcalloc( nitems, sizeof( *self ) );
   self->ip_address = ip_address;
   tbl->pms[ tbl->pms_nr++ ] = self;

   return self;
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
   
    for ( uint32_t i = 0; i < pm->vms_nr; i++ ) {
      if ( pm->vms[ i ]->ip_address == ip_address ) {
        return pm->vms[ i ];
      }
    }
    ALLOC_GROW( pm->vms, pm->vms_nr + 1, pm->vms_alloc );
    size_t nitems = 1;
    vm *self = xcalloc( nitems, sizeof( *self ) );
    self->ip_address = ip_address;
    pm->vms[ pm->vms_nr++ ] = self;
    return self;
  }

  return NULL;
}


static port *
port_create( int if_index, uint32_t ip_address, uint64_t mac_address, pm *pm ) {
  for ( uint32_t i = 0; i < pm->ports_nr; i++ ) {
    if ( pm->ports[ i ]->if_index == if_index &&
         pm->ports[ i ]->ip_address == ip_address &&
         pm->ports[ i ]->mac_address == mac_address ) {
      return pm->ports[ i ];
    }
  }
  ALLOC_GROW( pm->ports, pm->ports_nr + 1, pm->ports_alloc );
  size_t nitems = 1;
  port *p = xcalloc( nitems, sizeof( *p ) );
  p->if_index = if_index;
  p->ip_address = ip_address;
  p->mac_address = mac_address;
  pm->ports[ pm->ports_nr++ ] = p;

  return p;
}


static cpu *
cpu_create( uint32_t id, pm *pm ) {
  for ( uint32_t i = 0; i < pm->cpus_nr; i++ ) {
    if ( pm->cpus[ i ]->id == id ) {
      return pm->cpus[ i ];
    }
  }
  ALLOC_GROW( pm->cpus, pm->cpus_nr + 1, pm->cpus_alloc );
  size_t nitems = 1;
  cpu *c = xcalloc( nitems, sizeof( *c ) );
  c->id = id;
  pm->cpus[ pm->cpus_nr++ ] = c;

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
    for ( uint32_t j = 0; j < pm->ports_nr; j++ ) {
      pm->ports[ j ]->status = 0;
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

    port *p = port_create( if_index, ip_address, mac_address, self );
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
    for ( uint32_t j = 0; j < pm->cpus_nr; j++ ) {
      pm->cpus[ j ]->status = 0;
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
    cpu *c = cpu_create( id, self );
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

    jedex_value_get_by_name( stats, "vm_count", &field, NULL );
    jedex_value_get_int( &field, &tmp );
    self->vm_count = ( uint32_t ) tmp;

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
#ifdef LATER
  foreach_stats( &vm_port_stats_ary, vm_port_stats_create, &self->pm_tbl );
  foreach_stats( &vm_cpu_stats_ary, vm_cpu_stats_create, &self->pm_tbl );
  foreach_stats( &service_stats_ary, service_stats_create, &self->pm_tbl );
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
