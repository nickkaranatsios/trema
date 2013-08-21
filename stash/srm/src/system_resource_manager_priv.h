/*
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


#ifndef SYSTEM_RESOURCE_MANAGER_PRIV_H
#define SYSTEM_RESOURCE_MANAGER_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


typedef struct vm_spec {
  char *key;
  uint32_t pm_ip_address;
  uint32_t ip_address;
  uint32_t user_count;
  uint16_t cpu_count;
  uint16_t memory_size;
} vm_spec;


typedef struct vm_table {
  vm_spec **vm_specs;
  uint32_t vm_specs_nr;
  uint32_t vm_specs_alloc;
} vm_table;


typedef struct pm_spec {
  char *key;
  uint32_t ip_address;
  char *igmp_group_address_format;
  uint16_t igmp_group_address_start;
  uint16_t igmp_group_address_end;
  char *vm_ip_address_format;
  uint16_t vm_ip_address_start;
  uint16_t vm_ip_address_end;
  char *data_plane_ip_address_format;
  uint64_t data_plane_mac_address;
  vm_table vm_tbl;
} pm_spec;


typedef struct pm_table {
  pm_spec **pm_specs;
  uint32_t pm_specs_nr;
  uint32_t pm_specs_alloc;
} pm_table;
  
  
typedef struct system_resource_manager {
  emirates_iface *emirates;
  pm_table pm_tbl;
  jedex_schema *schema;
} system_resource_manager;


// init.c
system_resource_manager *system_resource_manager_initialize( int argc, char **argv, system_resource_manager **srm_ptr ); 

// parse_options.c
void parse_options( int argc, char **argv, void *user_data );


CLOSE_EXTERN
#endif // SYSTEM_RESOURCE_MANAGER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
