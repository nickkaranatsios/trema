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


#ifndef SERVICE_MANAGER_PRIV_H
#define SERVICE_MANAGER_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


typedef sm_add_service_profile_request service_profile_spec_value;

typedef struct service_profile_spec {
  char *key;
  service_profile_spec_value spec_value;
} service_profile_spec;

typedef struct service_spec {
  char *key;
  char *name;
  char *service_module;
  char *chef_recipe;
  service_profile_spec **service_profile_specs;
  uint32_t service_profile_specs_nr;
  uint32_t service_profile_specs_alloc;
} service_spec;


typedef struct service_profile_table {
  service_spec **service_specs;
  uint32_t service_specs_nr;
  uint32_t service_specs_alloc;
} service_profile_table;


typedef struct service_manager {
  emirates_iface *emirates;
  service_profile_table service_profile_tbl;
  // a pointer to the main schema
  jedex_schema *schema;
  // pointers to the sub_schemas
  jedex_schema *sub_schema[ 9 ];
  jedex_value *rval[ 9 ];
} service_manager;


// init.c
service_manager *service_manager_initialize( int argc, char **argv, service_manager **sm_ptr ); 

// parse_options.c
void parse_options( int argc, char **argv, void *user_data );

// message.c

void service_module_registration_handler( const uint32_t tx_id,
                                          jedex_value *val,
                                          const char *json,
                                          void *user_data );

void service_module_remove_handler( const uint32_t tx_id,
                                    jedex_value *val,
                                    const char *json,
                                    void *user_data );

void add_service_profile_handler( const uint32_t tx_id,
                                  jedex_value *val,
                                  const char *json,
                                  void *user_data );

void del_service_profile_handler( const uint32_t tx_id,
                                  jedex_value *val,
                                  const char *json,
                                  void *user_data );

void start_service_specific_nc_handler( const uint32_t tx_id,
                                        jedex_value *val,
                                        const char *json,
                                        void *user_data );

void oss_bss_add_service_handler( jedex_value *val,
                                  const char *json,
                                  const char *client_id,
                                  void *user_data );

void oss_bss_del_service_handler( jedex_value *val,
                                  const char *json,
                                  const char *client_id,
                                  void *user_data );


CLOSE_EXTERN
#endif // SERVICE_MANAGER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
