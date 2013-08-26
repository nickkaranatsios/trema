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


static vm *
vm_next( vm_table *tbl ) {
  uint32_t start_idx = tbl->selected_vm;
  uint32_t end_idx = start_idx;

  do {
    // if status is not equal to reserve
    if ( tbl->vms[ start_idx ]->status != reserved ) {
      tbl->vms[ start_idx ]->status = about_to_reserve;
      tbl->selected_vm = ( start_idx + 1 ) % tbl->vms_nr;
      return tbl->vms[ start_idx ];
    }
    start_idx = ( start_idx + 1 ) % tbl->vms_nr;
  } while ( start_idx != end_idx );

  // all vms are reserved.
  return NULL;
}


static pm *
pm_next( pm_table *tbl ) {
  uint32_t start_idx = tbl->selected_pm;
  uint32_t end_idx = start_idx;

  if ( !tbl->pms[ start_idx ]->max_vm_count ) {
    return NULL;
  }
  do {
    // when the whole pm is reserved.
    if ( tbl->pms[ start_idx ]->status != reserved ) {
      // if all vms in this pm are reserve set the pm status to reserve also.
      vm_table *vm_tbl = &tbl->pms[ start_idx ]->vm_tbl;
      uint32_t reserve_count = 0;
      for ( uint32_t i = 0; i < vm_tbl->vms_nr; i++ ) {
        if ( vm_tbl->vms[ i ]->status == reserved || vm_tbl->vms[ i ]->status == about_to_reserve ) {
          reserve_count++;
        }
      }
      if ( reserve_count == vm_tbl->vms_nr ) {
        tbl->pms[ start_idx ]->status = reserved;
      }
      tbl->selected_pm = ( start_idx + 1 ) % tbl->pms_nr;
      return tbl->pms[ start_idx ];
    }
    start_idx = ( start_idx + 1 ) % tbl->pms_nr;
  } while ( start_idx != end_idx );

  // all pms are reserved
  return NULL;
}


static uint32_t
vm_reserve_count( const vm_table *tbl ) {
  uint32_t count = 0;

  for ( uint32_t i = 0; i < tbl->vms_nr; i++ ) {
    vm *vm = tbl->vms[ i ];
    if ( vm->status == about_to_reserve ) {
      count++;
    }
  }

  return count;
}


uint32_t
compute_n_vms( const char *service_name, uint64_t bandwidth, uint64_t n_subscribers, pm_table *tbl ) {
  UNUSED( bandwidth );
  
  pm *pm = tbl->pms[ tbl->selected_pm ];
  uint32_t n_vms = 0;
  int64_t n_subs = ( int64_t ) n_subscribers;
  // start computing from the current selected pm
  while ( n_subs > 0 ) {
    pm = pm_next( tbl );
    if ( pm ) {
      pm_spec *spec = pm_spec_select( pm->ip_address, tbl );
      service_spec *s_spec = service_spec_select( service_name, &spec->service_class_tbl );
      vm *vm = vm_next( &pm->vm_tbl );
      if ( vm ) {
        n_subs -= s_spec->user_count;
        n_vms++;
      }
    }
    else {
      break;
    }
  }

  return n_subs > 0 ? n_vms : 0;
}


uint32_t
compute_vm_cpus( const char *service, pm_table *tbl ) {
  uint32_t n_cpus = 0;

  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    pm *pm = tbl->pms[ i ];
    uint32_t reserve_count = vm_reserve_count( &pm->vm_tbl );   
    pm_spec *spec = pm_spec_select( pm->ip_address, tbl );
    service_spec *s_spec = service_spec_select( service, &spec->service_class_tbl );
    n_cpus += ( reserve_count * s_spec->cpu_count ); 
  }

  return n_cpus;
}


uint32_t 
compute_vm_memory( const char *service, pm_table *tbl ) {
  uint32_t memory = 0;

  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    pm *pm = tbl->pms[ i ];
    uint32_t reserve_count = vm_reserve_count( &pm->vm_tbl );
    pm_spec *spec = pm_spec_select( pm->ip_address, tbl );
    service_spec *s_spec = service_spec_select( service, &spec->service_class_tbl );
    memory += ( reserve_count * s_spec->memory_size );
  }

  return memory;
}


list_element *
compute_vm_locations( pm_table *tbl ) {
  list_element *ip_address_list;

  create_list( &ip_address_list );
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    pm *pm = tbl->pms[ i ];
    for ( uint32_t j = 0; j < pm->vm_tbl.vms_nr; j++ ) {
      vm *vm = pm->vm_tbl.vms[ j ];
      if ( vm->status == about_to_reserve ) {
        // finaly change the status to reserve.
        vm->status = reserved;
        append_to_tail( &ip_address_list, &pm->ip_address );
      }
    }
  }

  return ip_address_list;
}


    
/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
