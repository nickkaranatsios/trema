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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "arg_assert.h"
#include "log_writer.h"

#include "api_info.h"
#include "system_resource_manager_interface.h"
#include "nc_srm_message.h"

#include "srm_data_pool.h"
#include "create_json_file.h"
#include "send_json_file.h"

typedef struct stat Stat;

static int
get_localtime(char* datetime, size_t buff_size) {
  int rc = 0;
  
  if (datetime && buff_size>20) {
    time_t timer;
    struct tm *t_st;

    time(&timer);

    t_st = localtime(&timer);
    sprintf(datetime,"%04d/%02d/%02d %02d:%02d:%02d",
    t_st->tm_year+1900,
    t_st->tm_mon+1,
    t_st->tm_mday,
    t_st->tm_hour,
    t_st->tm_min,
    t_st->tm_sec);
    rc = 1;
  }
  return rc;
}


static int
do_mkdir(const char *path, mode_t mode) {
  Stat st;
  int  status = 0;

  if (stat(path, &st) != 0) {
      /* Directory does not exist. EEXIST for race condition */
      if (mkdir(path, mode) != 0 && errno != EEXIST)
          status = -1;
  }
  else if (!S_ISDIR(st.st_mode)) {
      errno = ENOTDIR;
      status = -1;
  }

  return(status);
}


static int
mkpath(const char *path, mode_t mode) {
    char *pp;
    char *sp;
    int  status;
    char *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0) {
        if (sp != pp) {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0) {
        status = do_mkdir(path, mode);
    }
    free(copypath);
    return (status);
}


/*
 * Returns the number of total vms in physical_machine_stats
 */
static int
get_all_vm_count(  
  uint32_t physical_machine_count,
  const physical_machine_stats* all_physical_machine_stats) {
    
  int count = 0;
  uint32_t i = 0;

  for ( i = 0; i<physical_machine_count; i++ ) {
    physical_machine_stats machine_stats = all_physical_machine_stats[i];
    int vm_count = machine_stats.virtual_machine_count;
        
    uint32_t j = 0;
    
    for ( j = 0; j<vm_count; j++ ) {
      virtual_machine_stats vm_stats = machine_stats.virtual_machine_stats[j];
      uint32_t vm_ip_addr = vm_stats.ip_address;
      if (vm_ip_addr) {
        count++;
      }       
    } //vm
  } //physical_machine

  return count; 
}


/*
 * Write vms information as a JSON array into the file specfied by fp.
 */
static int
write_vm_json_file(FILE* fp, 
  uint32_t physical_machine_count,
  const physical_machine_stats* all_physical_machine_stats) { 
    
  int all_vm_count = get_all_vm_count( 
      physical_machine_count,
      all_physical_machine_stats);
  
  log_info("Writes vm json file all_vm_count:%d", all_vm_count);

  int rc = RET_OK;
  
  int vm_number = 0;
  
  fprintf(fp, "\"%s\":[\n", KEY_VMS);
  uint32_t i = 0;
  for ( i = 0; i<physical_machine_count; i++ ) {
    physical_machine_stats machine_stats = all_physical_machine_stats[i];
    uint32_t               machine_ip_address = get_pm_machine_id( machine_stats.ip_address );

    char machine_id[NAME_LEN] = {0};
    char ip_address[NAME_LEN] = {0};
    char vm_id[NAME_LEN*2] = {0};

    //JSON machine_id 
    sprintf(machine_id, "0x%x", machine_ip_address);

    int vm_count = machine_stats.virtual_machine_count;
    
    uint32_t j = 0;    
    for ( j = 0; j<vm_count; j++ ) {
      
      virtual_machine_stats vm_stats = machine_stats.virtual_machine_stats[j];
      uint32_t vm_ip_addr = vm_stats.ip_address;
      if (vm_ip_addr == 0) {
        continue;
      }
      vm_number++;
      
      //vm_id will be a combination of "0x+machine_ipd_addr" - "0x+vm_ip_addr"
      
      sprintf(vm_id, "0x%x-0x%x", machine_ip_address, vm_ip_addr);
      
      sprintf(ip_address, "0x%x", vm_ip_addr);
      const int blen = SERVICE_NAME_LENGTH*SERVICE_MAX + SERVICE_MAX*strlen("\"")*2 
        + strlen("[]")+ 1 ;
      
      char service_names[blen];      
      memset(service_names, 0, sizeof(service_names));

      char* service_name= service_names; 

      strcat(service_name++, "[");
      uint32_t k = 0;

      log_info("VM_stats.service_count %d", vm_stats.service_count);
      
      char temp_service_name[SERVICE_NAME_LENGTH+1];
      char vm_name[NAME_LEN]   = {0};
      
      for ( k = 0; k < vm_stats.service_count; k++) {
          int l;
          
          if ( k >0 ) {
             //strcat(service_name++, ",");
             sprintf(service_name++, "%s", ",");
          }
          sprintf(service_name++, "\"");
          // strcat(service_name++, "\"");
          // Is vm_stats.service_stats[k].service_name NULL terminated string?
          
          memset(temp_service_name, 0, sizeof(temp_service_name));
          memcpy(temp_service_name, vm_stats.service_stats[k].service_name, SERVICE_NAME_LENGTH);
          temp_service_name[SERVICE_NAME_LENGTH] = '\0';
          
          strcpy(service_name, temp_service_name);
          log_debug("service_name %s\n",  temp_service_name);
          int len = strlen(temp_service_name);
          service_name += len;
          sprintf(service_name++, "\"");

          if ( strcmp( temp_service_name, "Internet_Access_Service" ) != 0 ) {
            continue;
          }
          
          sprintf( vm_name, "0" );
          for ( l = 0; l < vm_stats.service_stats[k].param_count; l++ ) {
            if ( strcmp( vm_stats.service_stats[k].param_stats[l].param_name, SERVICE_PARAMETER_NAME_BRAS0 ) == 0 ) {
              sprintf( vm_name, "%"PRIu64, vm_stats.service_stats[k].param_stats[l].param_value );
              break;
            }
          }
      }
      strcat(service_name, "]");


     fprintf(fp, "{\n");

     fprintf(fp, HASH_VALUE_STRING, KEY_MACHINE_ID, machine_id, ",");
     fprintf(fp, HASH_VALUE_STRING, KEY_VM_ID,      vm_id, ",");
     fprintf(fp, HASH_VALUE_ARRAY,  KEY_SERVICE,    service_names, ",");

     fprintf(fp, HASH_VALUE_STRING, KEY_NAME, vm_name, "");
     
     if (vm_number < all_vm_count) {
          fprintf(fp, "},\n");       
     } else {
          fprintf(fp, "}\n");    
     }
    } //vm
  } //physical_machine

  fprintf(fp, "]\n");

  return rc;   
}


/*
 * Create a json file specified by json_file parameter  for 'add_service' request,
 * and write some basic attributes, and vms information into the file.
 */
static int
create_vm_json_file_for_add_service( const srm_statistics_status_reply* reply, 
                const char* service_type, 
                double band_width,
                int number_of_subscribers,
                char* json_file ) {
                        
  int rc = RET_OK;

  if (reply == NULL) {
    return rc = ERR_NULL_ARGUMENT;
  }
   
  int result = reply->result;
  if (result != RET_OK ) {
    log_err("srm_statistics_status_reply: result error:%d\n", result);
    return result; //2013/03/17 rc;
  }

  uint32_t physical_machine_count = reply-> physical_machine_count ;
  const physical_machine_stats* all_physical_machine_stats = reply-> physical_machine_stats;
  if (physical_machine_count < 1 || all_physical_machine_stats == NULL) {
    log_err("srm_statistics_status_reply: invalid data");
    return ERR_REPLY_INVALID_DATA;
  }


  FILE* fp = fopen(json_file, "w");
  if (fp == NULL) {
    log_err("Failed to open a file:%s\n", json_file);
    return ERR_FILE_OPEN;
  }

  char datetime[32] = {0};
  get_localtime(datetime, sizeof(datetime));

  fprintf(fp, "{\n");
 
  fprintf(fp, HASH_VALUE_STRING, KEY_CREATED_AT,   datetime,    ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_PROCESS,      ADD_SERVICE,  ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_SERVICE_TYPE, service_type, ",");
  
  fprintf(fp, HASH_VALUE_FLOAT, KEY_BAND_WIDTH, band_width, ",");
  fprintf(fp, HASH_VALUE_INTEGER, KEY_NUMBER_OF_SUBSCRIBERS, number_of_subscribers, ",");
  
  rc =  write_vm_json_file(fp, physical_machine_count, all_physical_machine_stats);
  
  if (rc == RET_OK) {
    //Write a closing "}" string 
    fprintf(fp, "\n}\n");
    fclose(fp);
  } else {
    fclose(fp);
    unlink(json_file);
  }
  return rc;
}


/*
 * Create a json file specified by json_file parameter  for 'versionup_server' request,
 * and write some basic attributes, and vms information into the file.
 */
static int
create_vm_json_file_for_versionup_server( const srm_statistics_status_reply* reply, 
                                          const char* server_name, 
                                          const char* json_file ) {

  int rc = RET_OK;

  if (reply == NULL) {
    return rc = ERR_NULL_ARGUMENT;
  }
   
  int result = reply->result;
  if (result != RET_OK ) {
    log_err("srm_statistics_status_reply: result error:%d\n", result);    
    return result; //2013/03/17 rc;
  }

  uint32_t physical_machine_count = reply-> physical_machine_count ;
  const physical_machine_stats* all_physical_machine_stats = reply-> physical_machine_stats;

  if (physical_machine_count < 1 || all_physical_machine_stats == NULL) {
    log_err("srm_statistics_status_reply: invalid data");
    return ERR_REPLY_INVALID_DATA;
  }


  FILE* fp = fopen(json_file, "w");
  if (fp == NULL) {
    log_err("Failed to open a file:%s", json_file);
    return ERR_FILE_OPEN;
  }

  char datetime[32];
  get_localtime(datetime, sizeof(datetime));
  //Begin JSON file.
  fprintf(fp, "{\n");

  fprintf(fp, HASH_VALUE_STRING, KEY_CREATED_AT, datetime,        ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_PROCESS,    VERSIONUP_SERVER, ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_SERVER_NAME, server_name,    ",");

  rc =  write_vm_json_file(fp, physical_machine_count, all_physical_machine_stats);
  if (rc == RET_OK) {
    //Write a closing "}" string 
    fprintf(fp, "\n}\n");
    fclose(fp);
  } else {
    fclose(fp);
    unlink(json_file);
  }
  return rc;
}


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'add_service' request. 
 */
int
parse_physical_machine_stats_for_add_service( const char* service_type,
                                              double      band_width,
                                              int         number_of_subscribers,
                                              const char* reply) {
  log_info("Started parse_physical_machine_stats_for_add_service");
  
  int rc = RET_OK;

  header* hdr = (header*)reply;
  if (hdr == NULL) {
     return rc = ERR_REPLY_NO_HEADER;
  }

  int type   = hdr -> type;
  int length = hdr -> length;

  char json_file[MAX_PATH_SIZE];
  char base_path[MAX_PATH_SIZE];
  if (strcmp(service_type, SERVICE_NAME_INTERNET) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, INET_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, INET_SERVICE, VM_JSON_FILE); 
  }
  else if (strcmp(service_type, SERVICE_NAME_VIDEO) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, VIDEO_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, VIDEO_SERVICE, VM_JSON_FILE); 
  }

  struct stat sts;
  if ((stat (base_path, &sts)) == -1) {
     fprintf (stderr, "The file %s doesn't exist...\n", base_path);
     int r = mkpath(base_path, 0777);
     if (r != 0) {
       log_err("Failed to create a path:%s, error:%s", base_path, strerror(errno));
       return ERR_FILE_OPEN;
     }
  }

  log_info("Started parse_physical_machine_stats_for_add_service:file_name=%s", json_file);
 
  if (type == SRM_STATISTICS_STATUS_REPLY) {
 
      if ( length != sizeof( srm_statistics_status_reply )) {
        log_err("srm_statistics_status_reply:length error: %d\n", length);
        return rc = ERR_REPLY_DATA_SIZE;
      }
      
      rc = create_vm_json_file_for_add_service( 
                (const srm_statistics_status_reply*)reply, 
                service_type, 
                band_width,
                number_of_subscribers,
                json_file );

  }
  else {
    log_err("srm_statistics_status_reply:bad type:%d", type);
    rc = ERR_REPLY_BAD_TYPE;  
  }

  if ( rc == RET_OK ) {
    rc = send_json_file(json_file);
  }
  return rc;
}


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'versionup_server' request. 
 */
int
parse_physical_machine_stats_for_versionup_server(  const char* server_name,
                                                    const char* reply) {

  int rc = RET_OK;

  header* hdr = (header*)reply;
  if (hdr == NULL) {
     return rc = ERR_REPLY_NO_HEADER;
  }

  int type   = hdr -> type;
  int length = hdr -> length;

  char json_file[MAX_PATH_SIZE];
  char base_path[MAX_PATH_SIZE];
  sprintf(json_file, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, VERSIONUP_SERVER, VM_JSON_FILE); 
  sprintf(base_path, "%s/%s/%s", WORK_DIR, TOPOLOGY_DB, VERSIONUP_SERVER); 

  struct stat sts;
  if ((stat (base_path, &sts)) == -1) {
     log_info("The file %s doesn't exist...", base_path);
     int r = mkpath(base_path, 0777);
     if (r != 0) {
       log_err("Failed to create a path: %s, eror:%s", base_path, strerror(errno));
       return ERR_FILE_OPEN;
     }
  }

  if ( type == SRM_STATISTICS_STATUS_REPLY) {
      if ( length != sizeof( srm_statistics_status_reply )) {
        log_err("srm_statistics_status_reply:length error: %d", length);
        return rc = ERR_REPLY_DATA_SIZE;
      }
      
      rc = create_vm_json_file_for_versionup_server( 
                (const srm_statistics_status_reply*)reply, 
                server_name,
                json_file );
      
  } else {
    log_err("srm_statistics_status_reply:bad type:%d", type);
    rc = ERR_REPLY_BAD_TYPE;    
  }

  if ( rc == RET_OK ) {
    rc = send_json_file(json_file);
  }

  return rc;
}


static int
create_node_list(from_to_node* nodes, int node_size,
  int link_count, const api_link* all_api_link) {
  int i, j, n;
  int count = 0;
  
  for (j = 0; j<node_size; j++) {
      from_to_node entry = nodes[j];
      memset(&entry, 0, sizeof(entry));  
  }
    
  for(i = 0; i< link_count; i++) {
     api_link alink = all_api_link[i];
     
     int found = 0;
     for (n = 0; n<count; n++) {
       if (nodes[n].from_dp_id == alink.from_dp_id) {
         int pos = nodes[n].to_count++;
         nodes[n].to_dp_id[pos] = alink.to_dp_id;
         found = 1;
         break;
       }
     } 
     if (found != 1) {
       if (count<node_size) {
         int n = count++;
         nodes[n].from_dp_id = alink.from_dp_id;
         int pos = nodes[n].to_count++;
         nodes[n].to_dp_id[pos] = alink.to_dp_id;
       }
     }
  }
  return count;
}


void
dump_node_list(  from_to_node nodes[], int count) {
  log_debug("Dump node_list\n");
  int i, j;
  for (i = 0; i<count; i++) {
    from_to_node node = nodes[i];
    log_debug("From 0x%llx\n", node.from_dp_id);
    for (j = 0; j<node.to_count; j++) {
      log_debug(" 0x%llx ", node.to_dp_id[j]);
    }
    log_debug("\n");
  }
}

/*
 * Write topology information into the file specfied by fp.
 */
static int
write_topology_json_file(FILE* fp, int link_count, const api_link* org_api_link) { 
  
  int i, j;
  api_link  all_api_link[ MAX_LINK ];
  api_link  ave_link[ MAX_LINK ];
  int       ave_count = 0;
  int rc = RET_OK;
  
  if (link_count < 0 ) {
    return rc = ERR_REPLY_INVALID_DATA;
  }
  for ( i = 0; i < link_count; i++ ) {
    all_api_link[i] = org_api_link[i];

    for ( j = 0; j < API_MAX_LINK_PARAM; j++ ) {
      memset( all_api_link[i].param_l[j].name, 0, (API_MAX_LEN_PARAM_NAME+1) );
      all_api_link[i].param_l[j].value = 0;
    } 
    for ( j = 0; j < API_MAX_LINK_PARAM; j++ ) {
      if ( j >= org_api_link[i].param_count ) {
        continue;
      }

      if( strcmp( org_api_link[i].param_l[j].name, "Total" ) == 0 ) {
        all_api_link[i].param_l[0].value = org_api_link[i].param_l[j].value;
        log_debug("link[%d]-param[%d]: Total=(%"PRIu64")", i, j, all_api_link[i].param_l[0].value );
      }
      else if ( strcmp( org_api_link[i].param_l[j].name, "Internet_Access_Service" ) == 0 ) {
        all_api_link[i].param_l[1].value = org_api_link[i].param_l[j].value;
        log_debug("link[%d]-param[%d]: Internet_Access_Service=(%"PRIu64")", i, j, all_api_link[i].param_l[1].value );
      }
      else if ( strcmp( org_api_link[i].param_l[j].name, "Video_Service" ) == 0 ) {
        all_api_link[i].param_l[2].value = org_api_link[i].param_l[j].value;
        log_debug("link[%d]-param[%d]: Video_Service=(%"PRIu64")", i, j, all_api_link[i].param_l[2].value );
      }
      else {
        ; // nop
      }
    }
  }
  // Up-link and a down-link are unified and averaged.
  log_debug("1st proc start   link_count=%d", link_count );
  ave_count = 0;
  j = 0;
  for ( i = 0; i < link_count; i++ ) {
    if ( all_api_link[i].from_dp_id < all_api_link[i].to_dp_id ) {
      ave_link[j] = all_api_link[i];
      log_debug("link:(%llu)->(%llu) utl=(%f) total=(%llu) inet=(%llu) video=(%llu)",
                 ave_link[j].from_dp_id, ave_link[j].to_dp_id,
                 ave_link[j].utilization,
                 ave_link[j].param_l[0].value,
                 ave_link[j].param_l[1].value,
                 ave_link[j].param_l[2].value );
      ave_count++;
      j++;
    }
  }
  log_debug("1st proc end");
  log_debug("2nd proc start");
  for ( i = 0; i < link_count; i++ ) {
    for ( j = 0; j < ave_count; j++ ) {
      if ( ( ave_link[j].from_dp_id == all_api_link[i].to_dp_id   ) &&
           ( ave_link[j].to_dp_id   == all_api_link[i].from_dp_id ) ) {
        ave_link[j].utilization = ( ave_link[j].utilization + all_api_link[i].utilization ) / 2;
        ave_link[j].param_l[0].value = ( ave_link[j].param_l[0].value + all_api_link[i].param_l[0].value ) / 2;
        ave_link[j].param_l[1].value = ( ave_link[j].param_l[1].value + all_api_link[i].param_l[1].value ) / 2;
        ave_link[j].param_l[2].value = ( ave_link[j].param_l[2].value + all_api_link[i].param_l[2].value ) / 2;
        log_debug("link:(%llu)->(%llu) utl=(%f) total=(%llu) inet=(%llu) video=(%llu)",
                 ave_link[j].from_dp_id, ave_link[j].to_dp_id,
                 ave_link[j].utilization,
                 ave_link[j].param_l[0].value,
                 ave_link[j].param_l[1].value,
                 ave_link[j].param_l[2].value );
        break;
      }
    }
  }
  log_debug("2nd proc end");
  
  fprintf(fp, "\"%s\":{\n", KEY_TOPOLOGY);
  // 1 Write link
  fprintf(fp, "\"%s\": {\n", KEY_LINK);
  for ( i = 0; i<ave_count; i++ ) {
       api_link link = ave_link[i];
       fprintf(fp, "\"0x%llx\":{", link.link_id);
         fprintf(fp, HASH_VALUE_HEX_UINT64_HEX_UINT64, KEY_FROM_TO,  link.from_dp_id, link.to_dp_id, ",");
                         
         fprintf(fp, HASH_VALUE_FLOAT,  KEY_UTILIZATION,  link.utilization,     ",");
         fprintf(fp, HASH_VALUE_UINT64, KEY_TOTAL_USERS,  link.param_l[0].value, ",");
         fprintf(fp, HASH_VALUE_UINT64, KEY_INET_USERS,  link.param_l[1].value, ",");
         fprintf(fp, HASH_VALUE_UINT64, KEY_VIDEO_USERS,  link.param_l[2].value, "");
       if (i == ave_count-1) {
         fprintf(fp, "}\n");
       } else {
         fprintf(fp, "},\n");
       }
  }

  fprintf(fp, "\n},\n");

  //2 Write node
  fprintf(fp, "\"%s\":{\n", KEY_NODE);
  from_to_node nodes[MAX_LINK];
  memset( nodes, 0, sizeof(from_to_node) * MAX_LINK );
  int count = create_node_list(nodes, MAX_LINK,
       ave_count, ave_link);
 
  for (i = 0; i<count; i++) {
    from_to_node node = nodes[i];
    //fprintf(stderr, "From 0x%llx\n", node.from_dp_id);
    fprintf(fp, "\"0x%llx\":[", node.from_dp_id);

    for (j = 0; j<node.to_count; j++) {
      //fprintf(stderr, " 0x%llx ", node.to_dp_id[j]);
      fprintf(fp, "\"0x%llx\"", node.to_dp_id[j]);
      if (j != node.to_count-1) {
          fprintf(fp, ","); 
      }
    }

    if (i != count-1) {
      fprintf(fp, "],\n");        
    } else {
      fprintf(fp, "]\n");
    }
  }
  
  fprintf(fp, "\n}\n");

  fprintf(fp, "}\n");

  return rc;   
}


/*
 * Create a json file specified by json_file parameter  for 'add_service' request,
 * and write some basic attributes, and vms information into the file.
 */
static int
create_topology_json_file_for_add_service( const srm_nc_link_utilization_reply* reply, 
       const char* service_type, 
       double band_width,
       int number_of_subscribers,
       char* json_file ) {
                        
  int rc = RET_OK;

  if (reply == NULL) {
    return rc = ERR_NULL_ARGUMENT;
  }
   
  int link_count = reply-> info_num ;
  const api_link* all_api_link = reply-> info;
  if (link_count < 1 || all_api_link == NULL) {
    log_err("Invalid link data:empty");
    return ERR_REPLY_INVALID_DATA;
  }


  FILE* fp = fopen(json_file, "w");
  if (fp == NULL) {
    log_err("Failed to open a file:%s", json_file);

    return rc = ERR_FILE_OPEN;
  }

  char datetime[32] = {0};
  get_localtime(datetime, sizeof(datetime));

  fprintf(fp, "{\n");
 
  fprintf(fp, HASH_VALUE_STRING, KEY_CREATED_AT, datetime,    ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_PROCESS,     ADD_SERVICE,  ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_SERVICE_TYPE, service_type, ",");
  
  fprintf(fp, HASH_VALUE_FLOAT, KEY_BAND_WIDTH, band_width, ",");
  fprintf(fp, HASH_VALUE_INTEGER, KEY_NUMBER_OF_SUBSCRIBERS, number_of_subscribers, ",");
  
  rc =  write_topology_json_file(fp, link_count, all_api_link);
  
  if (rc == RET_OK) {
    //Write a closing "}" string 
    fprintf(fp, "\n}\n");
    fclose(fp);
  } else {
    fclose(fp);
    unlink(json_file);
  }
  return rc;
}


/*
 * Create a json file specified by json_file parameter  for 'versionup_server' request,
 * and write some basic attributes, and vms information into the file.
 */
static int
create_topology_json_file_for_versionup_server( const srm_nc_link_utilization_reply* reply, 
                const char* server_name, 
                const char* json_file ) {

  int rc = RET_OK;

  if (reply == NULL) {
    return rc = ERR_NULL_ARGUMENT;
  }
   
  int link_count = reply-> info_num ;
  const api_link* all_api_link = reply-> info;

  if (link_count < 1 || all_api_link == NULL) {
    return rc = ERR_REPLY_INVALID_DATA;
  }


  FILE* fp = fopen(json_file, "w");
  if (fp == NULL) {
    log_err("Failed to open a file: %s", json_file);
    return rc = ERR_FILE_OPEN;
  }

  char datetime[32];
  get_localtime(datetime, sizeof(datetime));
  //Begin JSON file.
  fprintf(fp, "{\n");

  fprintf(fp, HASH_VALUE_STRING, KEY_CREATED_AT, datetime,        ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_PROCESS,    VERSIONUP_SERVER, ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_SERVER_NAME, server_name,    ",");

 
  rc =  write_topology_json_file(fp, link_count, all_api_link);
  if (rc == RET_OK) {
    //Write a closing "}" string 
    fprintf(fp, "\n}\n");
    fclose(fp);
  } else {
    fclose(fp);
    unlink(json_file);
  }
  return rc;
}


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'add_service' request. 
 */
int
parse_physical_machine_topology_for_add_service(const char* service_type,
                                                double      band_width,
                                                int         number_of_subscribers,
                                                const char* reply) {
 
  log_info("Started parse_physical_machine_topology_for_add_service");
  
  int rc = RET_OK;

  header* hdr = (header*)reply;
  if (hdr == NULL) {
     return rc = ERR_REPLY_NO_HEADER;
  }

  int type   = hdr -> type;
  int length = hdr -> length;

  char json_file[MAX_PATH_SIZE];
  char base_path[MAX_PATH_SIZE];
  if (strcmp(service_type, SERVICE_NAME_INTERNET) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, INET_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, INET_SERVICE, TOPOLOGY_JSON_FILE); 
  
  }
  else if (strcmp(service_type, SERVICE_NAME_VIDEO) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, VIDEO_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, ADD_SERVICE, VIDEO_SERVICE, TOPOLOGY_JSON_FILE); 
  
  }

  struct stat sts;
  if ((stat (base_path, &sts)) == -1) {
     log_info("The file %s doesn't exist", base_path);
     int r = mkpath(base_path, 0777);
     if (r != 0) {
       log_err("Failed to create a path : %s, error=%s ", base_path, strerror(errno));
       return ERR_FILE_OPEN;
     } else {
       log_info("Created a path : %s", base_path);
     }
  }

  log_info("Json file name=%s", json_file);
 
  if (type == SRM_NC_LINK_UTILIZATION_REPLY) {
 
      if ( length != sizeof( srm_nc_link_utilization_reply)) {
        log_err("srm_nc_link_utilization_reply size error:%d", length);

        return rc = ERR_REPLY_DATA_SIZE;
      }
      
      rc = create_topology_json_file_for_add_service( (const srm_nc_link_utilization_reply*)reply, 
                service_type, 
          band_width,
                number_of_subscribers,
      json_file );

  } else {
    log_err("Bad srm_nc_statistics_reply type:%d", type);
    rc = ERR_REPLY_BAD_TYPE;
  }

  if ( rc == RET_OK ) {
    rc = send_json_file(json_file);
  }

  return rc;
}


/*
 * Parse the data of struct srm_statistics_status_reply pointer, and create a json file
 * for 'versionup_server' request. 
 */
int
parse_physical_machine_topology_for_versionup_server( const char* server_name,
                                                      const char* reply) {

  log_info("Started parse_physical_machine_topology_for_versionup_server");

  int rc = RET_OK;

  header* hdr = (header*)reply;
  if (hdr == NULL) {
     return rc = ERR_REPLY_NO_HEADER;
  }

  int type   = hdr -> type;
  int length = hdr -> length;

  char json_file[MAX_PATH_SIZE];
  char base_path[MAX_PATH_SIZE];
  sprintf(json_file, "%s/%s/%s/%s", WORK_DIR, TOPOLOGY_DB, VERSIONUP_SERVER, TOPOLOGY_JSON_FILE); 
  sprintf(base_path, "%s/%s/%s", WORK_DIR, TOPOLOGY_DB, VERSIONUP_SERVER); 
  
  struct stat sts;
  if ((stat (base_path, &sts)) == -1) {
     log_info("The file %s doesn't exist.", base_path);
     int r = mkpath(base_path, 0777);
     if (r != 0) {
       log_err("Failed to create a path:%s, error=%sn", base_path, strerror(errno));
       return ERR_FILE_OPEN;
     } else {
       log_info("Created a path:%s.", base_path);
     }
  }

  if ( type == SRM_NC_LINK_UTILIZATION_REPLY) {
      if ( length != sizeof( srm_nc_link_utilization_reply)) {
        log_err("srm_nc_link_utilization_reply size error:%d", length);
        return rc = ERR_REPLY_DATA_SIZE;
      }
      
      rc = create_topology_json_file_for_versionup_server( (const srm_nc_link_utilization_reply*)reply, 
                server_name,
    json_file );
      
  } else {
    log_err("Bad srm_nc_statistics_reply type:%d", type);
    rc = ERR_REPLY_BAD_TYPE;    
  }

  if ( rc == RET_OK ) {
    rc = send_json_file(json_file);
  }

  return rc;
}


//2013/02/28
//////////////////////////////////////
/*
 * Write service performance information as a JSON array into the file specfied by fp.
 */
static int
write_performance_json_file(FILE* fp, 
  const char* service_name,   //"Internet_Access_Service" or "Video_Service", or "Phone_Service
  uint32_t count,
  const api_nc_statistics_status* all_status) { 
  
  int i;
  int index = -1;

  char temp_service_name[ API_MAX_LEN_SERVICE_ID +1] = {0};
    
  for (i = 0; i<count; i++) {  
       api_nc_statistics_status status = all_status[i];
       strcpy(temp_service_name, status.name); 
       
       if (strcmp(status.name, service_name) == 0) {
          index = i;
          log_info("service:%s index:%d", status.name, index );
          break;       
       } 
  }

  int rc = RET_OK;
  
  if (index == -1) {
    log_err("api_nc_statistics_status. invalid service_name: %s", temp_service_name);
    
     return rc = ERR_REPLY_INVALID_SERVICE;    
  }
  
  api_nc_statistics_status status = all_status[index];

  api_parameter parameter = status.param_ncss[0];

  // name = "achieve", value= performance (0.1%)
  fprintf(fp, "\"%s\": %llu,\n",  KEY_ACHIEVEMENT, parameter.value/10);  // to convert 0.1%  1% unit
  
  uint16_t param_count = status.param_count;
  uint16_t n;
  fprintf(fp, "\"%s\": [\n",  KEY_PERFORMANCE);  
  
  for (n = 0; n<param_count; n++) {
    
    api_parameter parameter = status.param_ncss[n];
    log_debug("label: %s , value=%llu", parameter.name, parameter.value);

     const char* PERFORMANCE = "Performance_";
     
     // "performance 0.0-0.2[Mbps]" -> "0.0-0.2"
     char label[API_MAX_LEN_PARAM_NAME+1];
     memset(label, 0, sizeof(label));
     memcpy(label, parameter.name, sizeof(label));
     label[API_MAX_LEN_PARAM_NAME] = '\0';
     
     char* short_label = label;
     if (strncmp(label, PERFORMANCE, strlen(PERFORMANCE)) == 0) {
       short_label += strlen(PERFORMANCE);
       if (*short_label == ' ') {
         short_label++;
       }
     }
     char* bracket = strrchr(label, '[');
     if (bracket) {
       *bracket = '\0';  
     }
     
    log_info("service:%s performance %s :%"PRIu64"", service_name, short_label, parameter.value);
    fprintf(fp, "{\"%s\": \"%s\", \"%s\": %llu}", 
          KEY_LABEL, short_label,
          KEY_VALUE, parameter.value/10);  // to convert 0.1% to 1% unit
    if (n != param_count-1) {
      fprintf(fp, ",\n"); 
    } else {
      fprintf(fp, "\n"); 
    }
    
  } //for param_count

  fprintf(fp, "]\n");
  
  return rc;   
}

/*
 * Create a performance.json file specified by json_file parameter  for 'add_service' request,
 * and write some basic attributes, and service performance information into the file.
 */
static int
create_performance_json_file_for_add_service( const srm_nc_statistics_reply* reply, 
       const char* service_type, 
       double band_width,
       int number_of_subscribers,
       char* json_file ) {
                        
  int rc = RET_OK;

  if (reply == NULL) {
    return rc = ERR_NULL_ARGUMENT;
  }
   
  int count = reply-> info_num ;
  const api_nc_statistics_status* status = reply-> info;
  if (count < 1 || status == NULL) {
    log_err("Invalid statistics data:empty");
    return ERR_REPLY_INVALID_DATA;
  }


  FILE* fp = fopen(json_file, "w");
  if (fp == NULL) {
    log_err("Failed to open a file:%s", json_file);

    return rc = ERR_FILE_OPEN;
  }
  log_debug("Opened a file:%s", json_file);

  char datetime[32] = {0};
  get_localtime(datetime, sizeof(datetime));

  fprintf(fp, "{\n");
 
  fprintf(fp, HASH_VALUE_STRING, KEY_CREATED_AT, datetime,    ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_PROCESS,     ADD_SERVICE,  ",");
  fprintf(fp, HASH_VALUE_STRING, KEY_SERVICE_TYPE, service_type, ",");
  
  fprintf(fp, HASH_VALUE_FLOAT, KEY_BAND_WIDTH, band_width, ",");
  fprintf(fp, HASH_VALUE_INTEGER, KEY_NUMBER_OF_SUBSCRIBERS, number_of_subscribers, ",");
  
  char* service_name = INTERNET_ACCESS_SERVICE_NAME;
  if (strcmp(service_type, SERVICE_NAME_INTERNET) == 0) {
    service_name = INTERNET_ACCESS_SERVICE_NAME;
  } else if (strcmp(service_type, SERVICE_NAME_VIDEO) == 0) {
    service_name = VIDEO_SERVICE_NAME;
  } 
  
  rc =  write_performance_json_file(fp, service_name, count, status);
  
  if (rc == RET_OK) {
    //Write a closing "}" string 
    fprintf(fp, "\n}\n");
    fclose(fp);
  } else {
    fclose(fp);
    unlink(json_file);
  }
  return rc;
}

/*
 * Parse the data of struct srm_nc_statistics_reply pointer, and create 
 * a service performance.json file
 * for 'add_service' request. 
 */
int
parse_service_statistics_for_add_service( const char* service_type,
                                          double band_width,
                                          int    number_of_subscribers,
                                          const char* reply) {
 
  log_info("Started parse_service_statistic_for_add_service");
  
  int rc = RET_OK;

  header* hdr = (header*)reply;
  if (hdr == NULL) {
     return rc = ERR_REPLY_NO_HEADER;
  }

  int type   = hdr -> type;
  int length = hdr -> length;

  char json_file[MAX_PATH_SIZE];
  char base_path[MAX_PATH_SIZE];
  if (strcmp(service_type, SERVICE_NAME_INTERNET) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, SERVICE_DB, ADD_SERVICE, INET_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, SERVICE_DB, ADD_SERVICE, INET_SERVICE, 
          PERFORMANCE_JSON_FILE); 
  
  }
  else if (strcmp(service_type, SERVICE_NAME_VIDEO) == 0) {
    sprintf(base_path, "%s/%s/%s/%s", WORK_DIR, SERVICE_DB, ADD_SERVICE, VIDEO_SERVICE); 
    sprintf(json_file, "%s/%s/%s/%s/%s", WORK_DIR, SERVICE_DB, ADD_SERVICE, VIDEO_SERVICE, 
        PERFORMANCE_JSON_FILE); 
  }

  struct stat sts;
  if ((stat (base_path, &sts)) == -1) {
     log_info("The file %s doesn't exist", base_path);
     int r = mkpath(base_path, 0777);
     if (r != 0) {
       log_err("Failed to create a path : %s, error=%s ", base_path, strerror(errno));
       return ERR_FILE_OPEN;
     } else {
       log_info("Created a path : %s", base_path);
     }
  }

  log_info("Json file name=%s", json_file);
 
  if (type == SRM_NC_STATISTICS_REPLY) {
 
      if ( length != sizeof( srm_nc_statistics_reply )) {
        log_err("srm_nc_statistics_reply size error:%d", length);

        return rc = ERR_REPLY_DATA_SIZE;
      }
      
      rc = create_performance_json_file_for_add_service( (const  srm_nc_statistics_reply*)reply, 
                service_type, 
                band_width,
                number_of_subscribers,
      json_file );

  } else {
    log_err("Bad srm_nc_statistics_reply type:%d", type);
    rc = ERR_REPLY_BAD_TYPE;
  }

  if ( rc == RET_OK ) {
    rc = send_json_file(json_file);
  }

  return rc;
}


