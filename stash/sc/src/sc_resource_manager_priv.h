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


#ifndef SC_RESOURCE_MANAGER_PRIV_H
#define SC_RESOURCE_MANAGER_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#define VIRTUAL_MACHINE_NAME_LENGTH   32


typedef struct port {
  int port_no;
} port;


typedef struct port_table {
  port **ports;
  uint32_t ports_nr;
  uint32_t ports_alloc;
} port_table;


typedef struct vm_info {
  int vm_id;
} vm_info;


typedef struct vm_info_table {
  vm_info **vm_infos;
  uint32_t vm_infos_nr;
  uint32_t vm_infos_alloc;
} vm_info_table;


typedef struct service {
  char *name;
  vm_info_table vm_info_table;
} service;


typedef struct service_table {
  service **services;
  uint32_t services_nr;
  uint32_t services_alloc;
} service_table;


typedef struct vm_service {
  char *name;
} vm_service;


typedef struct vm_service_table {
  vm_service **vm_services;
  uint32_t vm_services_nr;
  uint32_t vm_services_alloc;
} vm_service_table;


typedef struct vm {
  uint32_t ip_address;
  int64_t mac_address;
  int id;
  char name[ VIRTUAL_MACHINE_NAME_LENGTH ];
  port_table port_tbl;
  vm_service_table vm_service_tbl;
} vm;


typedef struct vm_table {
  vm **vms;
  uint32_t vms_nr;
  uint32_t vms_alloc;
} vm_table;


typedef struct pm {
  uint32_t ip_address;
  int id;
  port_table port_table;
  vm_info_table active_vm_tbl;
  service_table service_tbl;
  vm_table vm_tbl;
} pm;


typedef struct pm_table {
  pm **pms;
  uint32_t pms_nr;
  uint32_t pms_alloc;
} pm_table;


typedef struct sc_resource_manager {
  emirates_iface *emirates;
  redisContext *rcontext;
  sc_resource_manager_args *args;
  // pointer to the main schema
  jedex_schema *schema;
  // pointer to a union value that represents the entire schema.
  jedex_value *uval;
  // pointers to the sub_schemas
  jedex_schema *sub_schema[ 12 ];
  // pointers to the translated values for all sub-schemas
  jedex_value *rval[ 12 ];
  // store the subscription schemas here to be able to free.
  jedex_schema **schemas;
  pm_table pm_tbl;
} sc_resource_manager;


// init.c
sc_resource_manager *sc_resource_manager_initialize( int argc, char **argv, sc_resource_manager **sc_rm_ptr );

// parse_options.c
void parse_options( int argc, char **argv, void *user_data );

// message.c
void vm_allocate_handler( jedex_value *val,
                          const char *json,
                          const char *client_id,
                          void *user_data );

void service_module_reg_handler( jedex_value *val,
                                 const char *json,
                                 const char *client_id,
                                 void *user_data );

void pm_info_handler( jedex_value *val,
                      const char *json,
                      void *user_data );


CLOSE_EXTERN
#endif // SC_RESOURCE_MANAGER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
