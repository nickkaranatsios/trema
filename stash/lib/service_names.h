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


#ifndef SERVICE_NAMES_H
#define SERVICE_NAMES_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#define STR( s ) #s


#define OSS_BSS_ADD_SERVICE  STR( oss_bss_add_service )
#define OSS_BSS_DEL_SERVICE  STR( oss_bss_del_service )


// Service Manager service names SM_*
#define SM_SC_SERVICE_MODULE_REG_REQUEST STR( sm_sc_service_module_reg_request )
#define SM_NC_ADD_SERVICE_PROFILE_REQUEST STR( sm_nc_add_service_profile_request )
#define SM_NC_DEL_SERVICE_PROFILE_REQUEST STR( sm_nc_del_service_profile_request )


#define SM_SERVICE_MODULE_REG_REQUEST "sm_service_module_regist"
#define SM_ADD_SERVICE_PROFILE "sm_add_service_profile"
#define SM_DEL_SERVICE_PROFILE "sm_del_service_profile"
#define SM_IGNITE_SSNC "sm_ignite_ssnc"
#define SM_SHUTDOWN_SSNC "sm_shutdown_ssnc"
#define SM_REMOVE_SERVICE  "sm_remove_service"
#define SM_PHYSICAL_MACHINE_INFO "sm_physical_machine_info"

// System Resource Manager service names "SRM_*"
#define SRM_ADD_SERVICE_VM "srm_add_service_vm"
#define SRM_UPD_SERVICE_PROFILE "srm_upd_service_profile"
#define SRM_VM_MIGRATE "srm_virtual_machine_migrate"
#define SRM_NC_STATISTICS "srm_nc_statistics"
#define SRM_NC_LINK_UTILIZATION "srm_nc_link_utilization"

#define PM_INFO_PUBLISH STR( pm_info_publish )
#define STATS_COLLECT STR( stats_collect )
#define VM_ALLOCATE STR( vm_allocate )
#define SERVICE_DELETE STR( service_del )


CLOSE_EXTERN
#endif // SERVICE_NAMES_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */