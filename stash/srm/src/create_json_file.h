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

#pragma once

#ifndef CREATE_JSON_FILE_H
#define CREATE_JSON_FILE_H

#define ERR_REPLY_BAD_TYPE              -1

#define ERR_NULL_ARGUMENT               -2
#define ERR_REPLY_NO_HEADER             -3
#define ERR_REPLY_DATA_SIZE             -4
#define ERR_REPLY_INVALID_DATA          -5
#define ERR_FILE_OPEN                   -6
#define ERR_REPLY_INVALID_SERVICE       -7

#define NAME_LEN                        64
#define MAX_PATH_SIZE                 1024
#define SHORT_STRING_SIZE              256

//Probably, it's much better to change this 'WORK_DIR' value to other temporary folder.
//#define WORK_DIR                          "/var/www/nec"
#define WORK_DIR                          "."
#define TOPOLOGY_DB                       "topology_db"
#define SERVICE_DB                        "service_db"
#define ADD_SERVICE                       "add_service"
#define INET_SERVICE                      "inet_service"
#define VIDEO_SERVICE                     "video_service"
#define VERSIONUP_SERVER                  "versionup_server"

#define VM_JSON_FILE                      "vm.json"
#define TOPOLOGY_JSON_FILE                "topology.json"
#define PERFORMANCE_JSON_FILE             "performance.json"

#define HASH_VALUE_STRING                 "\"%s\":\"%s\"%s\n"
//#define HASH_VALUE_UINT64_HEX_STRING   "\"0x%llx\":\"%s\"%s\n"
#define HASH_VALUE_FLOAT                  "\"%s\":%f%s\n" 
#define HASH_VALUE_INTEGER                "\"%s\":%d%s\n"
#define HASH_VALUE_UINT64                 "\"%s\":%llu%s\n"
#define HASH_VALUE_ARRAY                  "\"%s\":%s%s\n"
#define HASH_VALUE_HEX_UINT64_HEX_UINT64  "\"%s\": \"0x%llx-0x%llx\"%s\n"

// Hash key names in JSON

#define KEY_CREATED_AT                    "created_at"
#define KEY_PROCESS                       "process"
#define KEY_SERVICE_TYPE                  "service_type"
#define KEY_BAND_WIDTH                    "band_width"
#define KEY_NUMBER_OF_SUBSCRIBERS         "number_of_subscribers"

#define KEY_TOPOLOGY                      "topology"
#define KEY_LINK                          "link"
#define KEY_NODE                          "node"

#define KEY_VMS                           "vms"

#define KEY_MACHINE_ID                    "machine_id"
#define KEY_VM_ID                         "vm_id"
#define KEY_SERVICE                       "service"
#define KEY_NAME                          "name"
#define KEY_SERVER_NAME                   "server_name"

#define KEY_FROM_TO                       "from_to"

#define KEY_UTILIZATION                   "utilization"
#define KEY_TOTAL_USERS                   "total_users"
#define KEY_INET_USERS                    "inet_users"
#define KEY_VIDEO_USERS                   "video_users"

#define KEY_PERFORMANCE                   "performance"
#define KEY_ACHIEVEMENT                   "achievement"
#define KEY_LABEL                         "label"
#define KEY_VALUE                         "value"

#define INTERNET_ACCESS_SERVICE_NAME      "Internet_Access_Service"
#define VIDEO_SERVICE_NAME                "Video_Service"
#define PHONE_SERVICE_NAME                "Phone_Service"



typedef struct {
  uint64_t from_dp_id;
  uint32_t to_count;
  uint64_t to_dp_id[MAX_LINK];
} from_to_node;


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'add_service' request. 
 */
int
parse_physical_machine_stats_for_add_service( const char* service_type,
                                              double      band_width,
                                              int         number_of_subscribers,
                                              const char* reply);

/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'versionup_server' request. 
 */
int
parse_physical_machine_stats_for_versionup_server(  const char* server_name,
                                                    const char* reply);


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'add_service' request. 
 * struct srm_nc_link_utilization_reply
 */
int
parse_physical_machine_topology_for_add_service(const char* service_type,
                                                double      band_width,
                                                int         number_of_subscribers,
                                                const char* reply);

/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'versionup_server' request. 
 */
int
parse_physical_machine_topology_for_versionup_server( const char* server_name,
                                                      const char* reply);

/*
 * Parse the data of struct srm_nc_statistics_reply pointer, and create 
 * a service performance.json file
 * for 'add_service' request. 
 */
int
parse_service_statistics_for_add_service( const char* service_type,
                                          double band_width,
                                          int    number_of_subscribers,
                                          const char* reply);


void
dump_node_list(  from_to_node nodes[], int count);


#endif //CREATE_JSON_FILE_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */




