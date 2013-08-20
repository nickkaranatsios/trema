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


#ifndef SERVICE_NAME_H
#define SERVICE_NAME_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


//OSS-BSS -> Service Manager & System Resource Manager
#define OSS_BSS_ADD_SERVICE  "oss_bss_add_service"
#define OSS_BSS_DEL_SERVICE  "oss_bss_del_service"


//Service Manager service name SM_*
#define SM_SERVICE_MODULE_REGIST "sm_service_module_regist"
#define SM_ADD_SERVICE_PROFILE "sm_add_service_profile"
#define SM_DEL_SERVICE_PROFILE "sm_del_serivce_profile"
#define SM_IGNITE_SSNC "sm_ignite_ssnc"
#define SM_SHUTDOWN_SSNC "sm_shutdown_ssnc"
#define SM_REMOVE_SERVICE  "sm_remove_service"
#define SM_PHYSICAL_MACHINE_INFO "sm_physical_machine_info"


CLOSE_EXTERN
#endif // SERVICE_NAME_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
