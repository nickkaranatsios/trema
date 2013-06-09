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
#include <pthread.h>
#include <jansson.h>

#include "../linked_list.h"
#include "../linked_list.c"

#include "jedex_schema.c"

int
main( int argc, char **argv ) {
  static const char json_schema[] = {
    "{"
    "  \"type\": \"record\","
    "  \"name\": \"test\","
    "  \"fields\": ["
    "    { \"name\": \"b\", \"type\": \"boolean\" },"
    "    { \"name\": \"i\", \"type\": \"int\" },"
    "    { \"name\": \"s\", \"type\": \"string\" },"
    "    { \"name\": \"ds\", \"type\": "
    "      { \"type\": \"array\", \"items\": \"double\" } },"
    "    { \"name\": \"sub\", \"type\": "
    "      {"
    "        \"type\": \"record\","
    "        \"name\": \"subtest\","
    "        \"fields\": ["
    "          { \"name\": \"f\", \"type\": \"float\" },"
    "          { \"name\": \"l\", \"type\": \"long\" }"
    "        ]"
    "      }"
    "    },"
    "    { \"name\": \"nested\", \"type\": [\"null\", \"test\"] }"
    "  ]"
    "}"
  };
  jedex_schema *record_schema = NULL;
  jedex_schema_from_json_literal( json_schema, &record_schema );
  
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
