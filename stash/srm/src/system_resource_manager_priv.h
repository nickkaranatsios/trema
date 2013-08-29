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


#define jedex_call( method, ... ) \
  do { \
    jedex_value_##method( __VA_ARGS__ ); \
  } while ( 0 )
  

typedef enum allocation_status {
  resource_free,
  booked,
  wait_for_confirmation,
  sc_reserved,
  nc_reserved,
  reserved
} allocation_status;


typedef struct service_spec {
  char *key;
  uint32_t user_count;
  uint16_t cpu_count;
  uint16_t memory_size;
  uint64_t requested_user_count;
  double requested_bandwidth;
} service_spec;


// For each class of service there is a service specification block
typedef struct service_class_table {
  service_spec **service_specs;
  uint32_t service_specs_nr;
  uint32_t service_specs_alloc;
} service_class_table;


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
  service_class_table service_class_tbl;
} pm_spec;


typedef struct cpu {
  double load;
  uint32_t id;
} cpu;


typedef struct port {
  int if_index;
  uint32_t ip_address;
  uint64_t mac_address;
  int if_speed;
  uint64_t tx_byte;
  uint64_t rx_byte;
} port;


typedef struct cpu_table {
  cpu **cpus;
  uint32_t cpus_nr;
  uint32_t cpus_alloc;
} cpu_table;


typedef struct port_table {
  port **ports;
  uint32_t ports_nr;
  uint32_t ports_alloc;
} port_table;


typedef struct param_stat {
  char *name;
  int64_t value;
} param_stat;


typedef struct param_stats_table {
  param_stat **param_stats;
  uint32_t param_stats_nr;
  uint32_t param_stats_alloc;
} param_stats_table;
  

typedef struct service {
  char *name;
  param_stats_table param_stats_tbl;
} service;
  

typedef struct service_table {
  service **services;
  uint32_t services_nr;
  uint32_t services_alloc;
} service_table;


typedef struct vm {
  uint32_t ip_address;
  uint32_t pm_ip_address;
  uint32_t data_plane_ip_address;
  uint32_t igmp_group_address;
  uint64_t data_plane_mac_address;
  uint64_t total_memory;
  uint64_t avail_memory;
  uint32_t service_count;
  uint16_t port_count;
  uint16_t cpu_count;
  uint16_t status;
  cpu_table cpu_tbl;
  port_table port_tbl;
  service_table service_tbl;
  // pointer to the service spec that this vm is being allocated
  service_spec *s_spec;
} vm;


typedef struct vm_table {
  vm **vms;
  uint32_t vms_nr;
  uint32_t vms_alloc;
  uint32_t selected_vm;
} vm_table;


typedef struct pm {
  // at initialization copy the ip address as defined in the config. file.
  uint32_t ip_address;
  uint32_t active_vm_count;
  uint16_t max_vm_count;
  uint64_t memory_size;
  uint64_t avail_memory;
  uint16_t port_count;
  uint16_t cpu_count;
  cpu_table cpu_tbl;
  port_table port_tbl;
  vm_table vm_tbl;
} pm;


typedef struct pm_table {
  pm_spec **pm_specs;
  uint32_t pm_specs_nr;
  uint32_t pm_specs_alloc;
  uint32_t selected_pm;
  pm **pms;
  uint32_t pms_nr;
  uint32_t pms_alloc;
} pm_table;
  
  
typedef struct system_resource_manager {
  emirates_iface *emirates;
  redisContext *rcontext;
  // pointer to supplied command line arguments
  system_resource_manager_args *args;
  pm_table pm_tbl;
  // pointer to the last requested service spec
  service_spec *s_spec;
  // pointer to the main schema
  jedex_schema *schema;
  // pointer to a union value that represents the entire schema.
  jedex_value *uval;
  jedex_schema *sub_schema[ 13 ];
  jedex_value *rval[ 13 ];
  // a flag that indicates if srm should publish pm info to service controller.
  uint32_t should_publish;
} system_resource_manager;


// init.c
system_resource_manager *system_resource_manager_initialize( int argc, char **argv, system_resource_manager **srm_ptr ); 

void system_resource_manager_finalize( system_resource_manager *self );

// parse_options.c
void parse_options( int argc, char **argv, void *user_data );

// message.c
typedef void ( *stats_fn ) ( jedex_value *stats, pm_table *tbl );

void periodic_timer_handler( void *user_data );

void oss_bss_add_service_handler( jedex_value *val,
                                  const char *json,
                                  const char *client_id,
                                  void *user_data );

void oss_bss_del_service_handler( jedex_value *val,
                                  const char *json,
                                  const char *client_id,
                                  void *user_data );

void sc_stats_collect_handler( const uint32_t tx_id,
                               jedex_value *val,
                               const char *json,
                               void *user_data );

void vm_in_service_handler( const uint32_t tx_id,
                            jedex_value *val,
                            const char *json,
                            void *user_data );

void service_profile_upd_handler( const uint32_t tx_id,
                                  jedex_value *val,
                                  const char *json,
                                  void *user_data );

void vm_allocate_handler( const uint32_t tx_id,
                          jedex_value *val,
                          const char *json,
                          void *user_data );

void service_delete_handler( const uint32_t tx_id,
                             jedex_value *val,
                             const char *json,
                             void *user_data );

// ip_utils.c
uint32_t ip_address_to_i( const char *ip_address_s );

// algorithm.c
uint32_t compute_n_vms( const char *service_name, uint64_t n_subscribers, pm_table *tbl );

void vms_release( const char *service_name, pm_table *tbl );

// service.c
service *service_create( const char *name, service_table *tbl );

void service_free( service_table *tbl );

param_stat *param_stat_create( const char *name, param_stats_table *tbl );

service_spec *service_spec_create( const char *key, size_t key_len, service_class_table *tbl );

void service_spec_set( const char *subkey, const char *value, service_spec *spec );

service_spec *service_spec_select( const char *key, service_class_table *tbl );

void service_spec_free( service_class_table *tbl );


// pm.c
void pm_create( uint32_t ip_address, pm_table *tbl );

pm *pm_select( const uint32_t ip_address, pm_table *tbl );

void pm_table_free( pm_table *tbl );

pm_spec *pm_spec_create( const char *key, size_t key_len, pm_table *tbl );

void pm_spec_set( const char *subkey, const char *value, pm_spec *spec );

pm_spec *pm_spec_select( const uint32_t ip_address, pm_table *tbl );


// vm.c
vm *vm_create( uint32_t ip_address, vm_table *tbl );

void vms_create( uint32_t pm_ip_address, pm_table *tbl, pm *pm );

vm *vm_select( uint32_t ip_address, vm_table *tbl );

void vm_free( vm_table *tbl );

cpu *cpu_create( uint32_t id, cpu_table *tbl );

void cpu_free( cpu_table *tbl );

port *port_create( int if_index, uint32_t ip_address, uint64_t mac_address, port_table *tbl );

void port_free( port_table *tbl );


CLOSE_EXTERN
#endif // SYSTEM_RESOURCE_MANAGER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
