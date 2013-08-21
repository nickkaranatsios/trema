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
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "arg_assert.h"
#include "read_properties.h"


static int is_quoted_string(char* val) {
  int rc = _false;
  char* q1 = strchr(val, '\"');
  char* q2 = strrchr(val, '\"');
  if (q1 && q2 && q1<q2) {
     rc = _true;
  }
  return rc;
}

static int is_integer(char* val)
{
  int rc = _false;
  // "  Hello world "
  char* q1 = strchr(val, '\"');
  char* q2 = strrchr(val, '\"');
  if (q1 || q2) {
     //string
     return rc; //false
  }

  int flag = _true;
  int i;
  for (i = 0; i<strlen(val); i++) {
     if (!isdigit(val[i]) ) {
        flag = _false;
        break;
     }
  } 
  return flag;
}

static char* trim_quote(char* val)
{
  char* trimmed = NULL;

  // "  Hello world "
  char* lq = strchr(val, '\"');
  char* rq = strrchr(val, '\"');
  if (lq && rq && lq < rq) {
     //quoted_string;  
      lq++;
      int len = rq - lq ;
      trimmed = (char*)malloc(len+1);
      memcpy(trimmed, lq, len);
      trimmed[len] = '\0'; 
  }
  return trimmed;
}


static char* trim_space(const char* line)
{
  arg_assert(line);
  
  char* trimmed = NULL;
  const char* ptr = line;
  while(*ptr == ' ' || *ptr == '\t') {
    ptr++;
  }
	
   const char* tail = ptr+strlen(ptr)-1;
   while(*tail == ' ' || *tail == '\t') {
     tail--;
   }

  int len = tail - ptr +1;
  //printf("len = %d\n", len);
  if (len >0) {
    trimmed = (char*)malloc(len+1);
    memcpy(trimmed, ptr, len);
    trimmed[len] = '\0';	
  }
   return trimmed;
}


void dump_properties(properties* props)
{
  arg_assert(props);
  
  int i;
  for(i = 0; i<props->count; i++) {

     if (props->property[i].value.type == TYPE_STRING) {
        printf("%s\n", props->property[i].value.string);
     } 
     if (props->property[i].value.type == TYPE_INTEGER) {
        printf("%d\n", props->property[i].value.integer);
     } 
  }  
}

void free_properties(properties* props)
{
  arg_assert(props);
  
  int i;
  for(i = 0; i<props->count; i++) {
     //printf("%s:", props.property[i].name);
     free(props->property[i].name);
     props->property[i].name = NULL;
     if (props->property[i].value.type == TYPE_STRING) {
      //  printf("%s\n", props.property[i].value.string);
       free(props->property[i].value.string);
       props->property[i].value.string = NULL;
     } 
  }  
}


int get_property_value(properties* props, const char* name, value** value)
{
  arg_assert(props);
  arg_assert(name);
  
  int rc = _false;
  int i;
  for(i = 0; i<props->count; i++) {
   
     if(strcmp(props->property[i].name, name) == 0) {
     //printf("Found %s %s\n", props.property[i].name, name);

      *value= &(props->property[i].value);
      rc = _true;
      break;
     }
  }  
  return rc;
}

const char* get_server_address(properties* props) {
  const char* server_ip = NULL;
  value* value = NULL;
  if (get_property_value(props, SERVER_ADDRESS, &value) ) {
    if (value->type == TYPE_STRING) {
       server_ip = value->string;
    }
  }
  return server_ip;
}

unsigned short get_server_port(properties* props) {
  value* value = NULL;
  unsigned short server_port = 0;
  if(get_property_value(props, SERVER_PORT, &value)) {
    if (value->type == TYPE_INTEGER) {
       server_port = (unsigned short)value->integer;
    }
  }
  return server_port;
}

int read_properties(const char* filename, properties* props)
{
  arg_assert(filename);
  arg_assert(props);
  
  int rc = NG;
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open a file:%s\n", filename);
    return rc;
  }

  int count = 0;

  char line[1024];
  while (fgets(line, sizeof(line), fp)) {

    int len = strlen(line);
    if (line[len-1] == '\n') {
	line[len-1] = '\0';
    }

    if (strlen(line)<1) {
      continue;
    }

    char* comment = strstr(line, "//");
    if (comment) {
	*comment = '\0';
    }
			
    if (strlen(line)<1) {
      continue;
    }

    char* colon = strchr(line, ':');
    if (colon == NULL) {
      continue;
    }
//	printf("Line:[%s]\n", line);

    char* name = line;
    if (colon) {
      *colon = '\0';
      //  printf("name [%s]\n", name);			   
      props->property[count].name = trim_space(name);
      //  printf("name [%s]\n", props->property[count].name);			   

      char* value = ++colon;
      char* val = trim_space(value);

      if (is_quoted_string(val)) {
        props->property[count].value.type   = TYPE_STRING;
        props->property[count].value.string = trim_quote(val);
        count++;
	      props->count = count; 
      } else if (is_integer(val)) {
        props->property[count].value.type   = TYPE_INTEGER;
        props->property[count].value.integer = atoi(val); 
        count++; 
        props->count= count;
      } else {
	       printf("Unsupported type\n");
      }
      free(val);
    }
  }

  fclose(fp);
  return rc = OK;
}


#ifdef TEST
int main(int argc, const char* argv[])
{
  if (argc != 2) {
   return ;
  }
  
  const char* filename = argv[1];
  //const char* filename = "./config.properties";

  properties props;
  int rc = read_properties(filename, &props);
  dump_properties(props);

  value* value;
  if (get_property_value(props, SERVER_ADDRESS, &value) ) {
     if (value->type==TYPE_STRING) {
	     printf("%s:%s\n", SERVER_ADDRESS, value->string);
     }
  }

  if (get_property_value(props, PORT, &value) ) {
     if (value->type==TYPE_INTEGER) {
	     printf("%s:%d\n", PORT, value->integer);
     }
  }
 free_properties(props);

 return 1;
}
#endif

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

