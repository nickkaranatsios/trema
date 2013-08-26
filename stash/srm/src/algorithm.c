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
    if ( tbl->vms[ start_idx ]->status != 2 ) {
      tbl->vms[ start_idx ]->status = 2;
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
    if ( tbl->pms[ start_idx ]->status != 2 ) {
      // if all vms in this pm are reserve set the pm status to reserve also.
      vm_table *vm_tbl = &tbl->pms[ start_idx ]->vm_tbl;
      uint32_t reserve_count = 0;
      for ( uint32_t i = 0; i < vm_tbl->vms_nr; i++ ) {
        if ( vm_tbl->vms[ i ]->status == 2 ) {
          reserve_count++;
        }
      }
      if ( reserve_count == vm_tbl->vms_nr ) {
        tbl->pms[ start_idx ]->status = 2;
      }
      tbl->selected_pm = ( start_idx + 1 ) % tbl->pms_nr;
      return tbl->pms[ start_idx ];
    }
    start_idx = ( start_idx + 1 ) % tbl->pms_nr;
  } while ( start_idx != end_idx );

  // all pms are reserved
  return NULL;
}


uint32_t
compute_n_vms( const char *service_name, uint64_t bandwidth, uint64_t n_subscribers, pm_table *tbl ) {
  UNUSED( bandwidth );
  
  pm *pm = tbl->pms[ tbl->selected_pm ];
  uint32_t pm_ip_address = pm->ip_address;
  pm_spec *spec = pm_spec_select( pm_ip_address, tbl );
  service_spec *s_spec = service_spec_select( service_name, &spec->service_class_tbl );
  uint32_t n_users = s_spec->user_count;
  

  uint32_t n_vms = 0;
  int64_t n_subs = ( int64_t ) n_subscribers;
  // start computing from the current selected pm
  while ( n_subs > 0 ) {
    pm = pm_next( tbl );
    if ( pm ) {
      vm *vm = vm_next( &pm->vm_tbl );
      if ( vm ) {
        n_subs -= n_users;
        n_vms++;
      }
    }
    else {
      break;
    }
  }

  return n_subs > 0 ? n_vms : 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
