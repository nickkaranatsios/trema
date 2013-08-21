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

#ifndef READ_PROPERTIES_H
#define READ_PROPERTIES_H

#define TYPE_STRING 1
#define TYPE_INTEGER 2

#define TYPE_SMALL_INTEGER 3
#define TYPE_LARGE_INTEGER 4
#define TYPE_DOUBLE        5
#define MAX_NAME       256

#define _true 1
#define _false 0

#define SERVER_ADDRESS  "server_address"
#define SERVER_PORT     "server_port"

#define isdigit(c) ((c >= '0' && c <= '9') ? _true : _false)

#define MAX_PROPS 100

#define MAX_LEN   1024

#define OK  0
#define NG (-1)

typedef struct {
  int type;
  union {
    char*    string;
    int      integer;
    uint64_t large_integer;
    double   real;
  };
} value;


typedef struct {
  char* name;
  value value;
} property;


typedef struct {
  int count;
  property property[MAX_PROPS];
} properties;


int  read_properties(const char* filename, properties* props);

void  dump_properties(properties* props);

int get_property_value(properties* props, const char* name, value** value);

void free_properties(properties* props);

const char* get_server_address(properties* props);

unsigned short get_server_port(properties* props);

#endif //READ_PROPERTIES_H

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

