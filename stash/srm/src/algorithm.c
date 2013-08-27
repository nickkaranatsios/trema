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
    if ( tbl->vms[ start_idx ]->status == resource_free ) {
      tbl->vms[ start_idx ]->status = booked;
      tbl->selected_vm = ( start_idx + 1 ) % tbl->vms_nr;
      return tbl->vms[ start_idx ];
    }
    start_idx = ( start_idx + 1 ) % tbl->vms_nr;
  } while ( start_idx != end_idx );

  // all vms are reserved.
  return NULL;
}


static int
is_vm_available( pm *pm ) {
  int avail = 0;

  vm_table *tbl = &pm->vm_tbl;
  for ( uint32_t i = 0; i < tbl->vms_nr; i++ ) {
    if ( tbl->vms[ i ]->status == resource_free ) {
      avail = 1;
      break;
    }
  }

  return avail;
}


static pm *
pm_next( pm_table *tbl ) {
  uint32_t start_idx = tbl->selected_pm;
  uint32_t end_idx = start_idx;

  if ( !tbl->pms[ start_idx ]->max_vm_count ) {
    return NULL;
  }
  do {
    // if there is a vm available can select this pm otherwise skip.
    if ( is_vm_available( tbl->pms[ start_idx ] ) ) {
      tbl->selected_pm = ( start_idx + 1 ) % tbl->pms_nr;
      return tbl->pms[ start_idx ];
    }
    start_idx = ( start_idx + 1 ) % tbl->pms_nr;
  } while ( start_idx != end_idx );

  // all pms are reserved
  return NULL;
}


uint32_t
compute_n_vms( const char *service_name, uint64_t n_subscribers, pm_table *tbl ) {
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
        vm->s_spec = s_spec;
        n_subs -= s_spec->user_count;
        n_vms++;
      }
    }
    else {
      break;
    }
  }
  /*
   * allocate request could not be satisfied allocation is incomplete, release
   * occupied booked resources.
   */
  if ( n_subs > 0 && n_vms > 0 ) {
    for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
      vm_table *vm_tbl = &tbl->pms[ i ]->vm_tbl;
      for ( uint32_t j = 0; j < vm_tbl->vms_nr; j++ ) {
        if ( vm_tbl->vms[ j ]->status == booked ) {
          vm_tbl->vms[ j ]->status = resource_free;
        }
      }
    }
  }

  return n_subs <= 0 ? n_vms : 0;
}


void
vms_release( const char *service, pm_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->pms_nr; i++ ) {
    vm_table *vm_tbl = &tbl->pms[ i ]->vm_tbl;
    for ( uint32_t j = 0; j < vm_tbl->vms_nr; j++ ) {
      vm *vm = vm_tbl->vms[ j ];
      if ( vm->status != resource_free ) {
        if ( !strncmp( service, vm->s_spec->key, strlen( vm->s_spec->key ) ) ) {
          vm->status = resource_free;
        }
      }
    }
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
