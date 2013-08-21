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


#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <errno.h> 

#include "arg_assert.h"
#include "log_writer.h"
#include "read_properties.h"

#define OK   0
#define NG   (-1)

#define MAX_PATH_SIZE  FILENAME_MAX //stdio.h
#define READ_BUFF_SIZE 1024
#define MAX_BUFFER_SIZE 1024

#define ERR_FILE_SIZE  (-1)
#define ERR_FILE_OPEN  (-2)
#define ERR_FILE_READ  (-3)
#define ERR_FILE_NOTFOUND  (-4)
#define ERR_INVALID_FILENAME (-5)


#define ERR_SOCK_CREATE      (-100)
#define ERR_SOCK_INVALID_IP  (-101)
#define ERR_SOCK_CONNECT     (-102)
#define ERR_SOCK_SEND        (-104)


#define ERR_SOCK_RECV        (-106)
#define ERR_REPLY_BUFFER_SIZE (-107)

#define MESSAGE_HEADER_SIZE 1024

#define SUCCESS  "SUCCESS"
#define FAILURE  "FAILURE"

#define REPLY_BUFFER_SIZE  1024

static const char* SERVER_CONFIG = "server.config";

#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

static size_t get_file_length(const char* filename) {
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}

/*
 * Try to send bytes of buffer of size to sockfd
 */
size_t do_send(int sockfd, void* buffer, int size) {
  int sent_bytes = 0;
  int remain = size; 

  while (remain>0) {
    
    int sent = send(sockfd, buffer, remain, 0);
   
#if 0
    if (sent < 0 ) {

      fd_set writefd;
      FD_ZERO(&writefd);
      FD_SET(sockfd, &writefd);

      if (select(sockfd+1, NULL, &writefd, NULL, NULL) != 1) {
        log_err("ERROR waiting to write to socket");
      }
      log_info("select()==1. continue");
      continue;
    }
#endif
        
    if (sent == -1) {
      /* send failed */
      log_err("Send failed: %s\n", strerror(errno) );
      break;
    } 

    sent_bytes += sent;
    buffer += sent;
    remain -= sent;
    log_info("Sent %d remain %d\n", sent, remain); 
  }
  log_info("return sent_bytes=%d",sent_bytes);
  return sent_bytes;
}
/*
 * Validate filename
 */
static int validate_filename(const char* filename)
{
  int rc = NG;
  static const char* allowed_json_files[] = { 
    // Allowed topology.json or vm.json files.

    "topology_db/add_service/inet_service/topology.json",
    "topology_db/add_service/inet_service/vm.json",

    "topology_db/add_service/video_service/topology.json",
    "topology_db/add_service/video_service/vm.json",

    "topology_db/versionup_server/topology.json",
    "topology_db/versionup_server/vm.json",

    // Allowed performance.json files,
    
    "service_db/add_service/inet_service/performance.json",
    "service_db/add_service/video_service/performance.json"
  };
  int i;
  for (i = 0; i<ARRAY_SIZE(allowed_json_files); i++) {
    if (strstr(filename, allowed_json_files[i])) {
      rc = OK;
      break;
    }
  }
  return rc;
}

/*
 * Send the contents of a file specified by filename.
 */
int send_file(int client_socket, const char* filename) {

  int rc = OK;
  //0 We need to check filename.
  if (validate_filename(filename) != OK) {
    log_err("Invalid filename:%s", filename);
    return rc = ERR_INVALID_FILENAME;
  }

  struct stat sts;
  //1 Does the file exist? 
  if ((stat (filename, &sts)) != 0) {
    log_err("File not found:%s", filename);
    return rc = ERR_FILE_NOTFOUND;
  }
  
  size_t file_size = get_file_length(filename);
  //2 Is File size OK? 
  if (file_size <= 0) {
     log_err("Illegal file size :%d\n", file_size);
     return rc = ERR_FILE_SIZE;
  }

  unsigned long length = MESSAGE_HEADER_SIZE;
  unsigned long nl = htonl(length);
  //3 Send MESSAGE_HEADR_SIZE in network_byte_order
  int r = do_send(client_socket, &nl, sizeof(nl));
  if (r != sizeof(nl)) {
     log_err("Failed to send a message size\n");
     return rc = ERR_SOCK_SEND;
  }

  char message[MESSAGE_HEADER_SIZE+1];
  //fill message buffer with 0
  memset(message, 0, sizeof(message));

  //copy filename to the message buffer
  memcpy(message, filename, strlen(filename)+1); 

  //4 Send the message
  //r = do_send(client_socket, message, sizeof(message)); 
  r = do_send(client_socket, message, MESSAGE_HEADER_SIZE); 
  //if (r != sizeof(message)) {
  if (r != MESSAGE_HEADER_SIZE) {
     log_err("Failed to send a message\n");
     return rc = ERR_SOCK_SEND;
  } 

  //We are ready to send a file-size and file contents
  FILE* fp = fopen(filename, "rb");

  if (fp == NULL) {
    log_err("Failed to open file %s\n", filename);
    return rc = ERR_FILE_OPEN;
  } 
    
  unsigned long n_file_size = htonl(file_size);
  //5 send the file_sze in network_byte_order.
  r = do_send(client_socket, &n_file_size, sizeof(n_file_size));
  if ( r != sizeof(n_file_size)) {
    log_err("Failed to send a file size\n");
    fclose(fp);
    return r = ERR_SOCK_SEND;
  } 

  char buffer[READ_BUFF_SIZE+1];
  int sent_bytes = 0;

  int total_read_bytes = 0;
  int total_sent_bytes = 0;

  //6 Read the content of the file and send them to server through socket
  while (!feof(fp)) {
    memset(buffer, 0, sizeof(buffer));
    //7 Read bytes from the file 
    int read_bytes = fread(buffer, sizeof(char), sizeof(buffer)-1, fp);
    if (read_bytes <= 0) {
       rc = ERR_FILE_READ;
       log_err("ERROR reading file");
       break;
    }

    total_read_bytes += read_bytes;
    char *pbuf = buffer;
    //8 Send the read bytes through the client_s socket
    while (read_bytes > 0) {
        sent_bytes = do_send(client_socket, pbuf, read_bytes);
       
        if (sent_bytes != read_bytes) {
          log_err("Maybe DISCONNECTED while writing to socket");
          rc = ERR_SOCK_SEND;
          break;
        }

        pbuf       += sent_bytes;
        read_bytes -= sent_bytes;
        total_sent_bytes += sent_bytes;
    } //while

    if (rc == ERR_SOCK_SEND) {
      break;
    }

  } //while(!feof(fp)

  fclose(fp);
  log_info("total_sent_bytes %d file_size %d\n",
          total_sent_bytes, file_size);

  if (total_sent_bytes != file_size) {
    log_err("Failed to send :total_sent_bytes %d != file_size %d\n",
          total_sent_bytes, file_size);
    
    rc = ERR_SOCK_SEND;
  }

  return rc;
}

/*
 * Receive a reply from the json_file_server.
 */
int confirm_reply(int sockfd) {

  int rc = NG;
  unsigned long reply_len = 0;
    
  int n = read(sockfd, &reply_len, sizeof(reply_len));
  if (n != sizeof(reply_len) ){
    return ERR_SOCK_RECV;
  }

  reply_len = ntohl(reply_len);
  if (reply_len > REPLY_BUFFER_SIZE) {
    return ERR_REPLY_BUFFER_SIZE;
  }
 
  char recv_buff[REPLY_BUFFER_SIZE+1];
  n = read(sockfd, recv_buff, reply_len);
   
  if(n < 0) {
    log_err("Read error");
    return rc = NG;
  }

  recv_buff[n] = '\0';
  log_info("Reply = %s\n", recv_buff);
  if(strcmp(recv_buff, "SUCCESS") == 0) {
     rc = OK; 
  } else {
     rc = NG;
  }
  return rc;
}

/*
 * Create a socket from the server_ip and port, and connect to the server of server_ip.
 */
int connect_to_server(const char* server_ip, unsigned short port) {
  arg_assert(server_ip);
  arg_assert(port);
  
  int sockfd;

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
     log_err("Could not create socket \n");
     return ERR_SOCK_CREATE;
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);


  if ( inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <=0 ){
    log_err("inet_pton error occured\nProbably, error=%s\n",
        strerror(errno));
    return ERR_SOCK_INVALID_IP;
  }

  if ( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    return ERR_SOCK_CONNECT;
  }
  return sockfd;
}

/*
 * Send the json file of file_name to the server specified 
 * in a server.config placed on the same folder of this
 * program.
 */
int send_json_file(const char* file_name)
{
  char program[MAX_PATH_SIZE+1];
  if (readlink("/proc/self/exe", program, sizeof(program)-1) == -1) {
     return -1;
  }
  
  //printf("program %s\n", program);
  char* slash = strrchr(program, '/');
  if (slash) {
    *slash = '\0';
  }
  
  char server_config[MAX_PATH_SIZE+1] = {0};
  sprintf(server_config, "%s/%s", program, SERVER_CONFIG);
 
  log_info("Send json file started: server.config:\"%s\"", server_config);

  properties props;
  memset(&props, 0, sizeof(props));
 
  if (read_properties(server_config, &props) !=OK) {
    log_err("Not found a file %s\n", server_config);
    return 0;
  }

  log_info("Read a configuration file:%s\n", server_config);
  //OK found a server_config.
  
  const char* server_ip = get_server_address(&props); 
  //The returned server_ip is merely a pointer to a null-termained string
  //stored in props object.  So, it will be an invalid pointer after 
  //calling free_properties.  

  unsigned short server_port = get_server_port(&props);
   
  if (server_ip == NULL || server_port <= 0) {
    log_err("Setting error in server.config a server %s: port:%d\n",  server_ip, server_port);
    free_properties(&props);
    return 1;  
  }
  
  int sockfd = connect_to_server(server_ip, server_port);
  if (sockfd <0) {
    log_err("Failed to connect a server %s: port:%d\n",  server_ip, server_port);
    return 1;
  }

 
  int rc = send_file(sockfd, file_name);
  if (rc == OK) {
    log_info("OK sent a file \n");
    if (confirm_reply(sockfd) == OK) {
      log_info("Server reply OK\n");
    } else {
      log_err("Server reply NG\n");
    }
      
  } else {
    log_err("Failed to send a file\n");
  }

  free_properties(&props);

  close(sockfd);

  return 0;
}

#if 0 // sample
int main(int argc, const char* argv[])
{
  //const char* file_name = "./topology_db/add_service/inet_service/topology.json";
  //const char* file_name = "./topology.json";
  const char* file_name = "/var/www/nec/topology_db/add_service/inet_service/topology.json";
  //const char* file_name = "./topology_db/add_service/video_service/topology.json";
  send_json_file(file_name);
  
  return 0;
}
#endif

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

