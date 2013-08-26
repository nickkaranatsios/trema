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


#include "emirates.h"
#include "jedex_iface.h"
#include "checks.h"
#include "wrapper.h"
#include "service_names.h"


typedef struct responder {
  emirates_iface *emirates;
  // top main schema
  jedex_schema *schema;
  /*
   * schema used to parse the request. In this example no data element
   * associated with.
   */
  jedex_schema *request_schema;
  // this is the schema to be used to create the reply.
  jedex_schema *reply_schema;
} responder;


static void
set_pm_status( jedex_value *array ) {
  jedex_value element;
  jedex_value_append( array, &element, NULL );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( &element, "pm_ip_address", &field, &index );
  jedex_value_set_int( &field, ( int32_t ) 2886730754 );

  jedex_value_get_by_name( &element, "port_count", &field, &index );
  jedex_value_set_int( &field, 2 );

  jedex_value_get_by_name( &element, "cpu_count", &field, &index );
  jedex_value_set_int( &field, 8 );

  jedex_value_get_by_name( &element, "vm_count", &field, &index );
  jedex_value_set_int( &field, 0 );

  jedex_value_get_by_name( &element, "memory_size", &field, &index );
  jedex_value_set_long( &field, 33554432 );

  jedex_value_get_by_name( &element, "available_memory", &field, &index );
  jedex_value_set_long( &field, 31457280 );

  jedex_value_append( array, &element, NULL );
  jedex_value_get_by_name( &element, "pm_ip_address", &field, &index );
  jedex_value_set_int( &field, ( int32_t ) 2886730755 );
  
  jedex_value_get_by_name( &element, "port_count", &field, &index );
  jedex_value_set_int( &field, 2 );

  jedex_value_get_by_name( &element, "cpu_count", &field, &index );
  jedex_value_set_int( &field, 4 );

  jedex_value_get_by_name( &element, "vm_count", &field, &index );
  jedex_value_set_int( &field, 0 );

  jedex_value_get_by_name( &element, "memory_size", &field, &index );
  jedex_value_set_long( &field, 6634864640 );

  jedex_value_get_by_name( &element, "available_memory", &field, &index );
  jedex_value_set_long( &field, 64424509440 );
}


static void
set_pm_cpu_status( jedex_value *array ) {
  jedex_value element;
  jedex_value_append( array, &element, NULL );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( &element, "pm_ip_address", &field, &index );
  jedex_value_set_int( &field, ( int32_t ) 2886730754 );

  jedex_value_get_by_name( &element, "id", &field, &index );
  jedex_value_set_int( &field, 1 );

  jedex_value_get_by_name( &element, "load", &field, &index );
  double cpu_load = 100.0 / 8.0;
  jedex_value_set_double( &field, cpu_load );

  jedex_value_append( array, &element, NULL );

  jedex_value_get_by_name( &element, "pm_ip_address", &field, &index );
  jedex_value_set_int( &field, ( int32_t ) 2886730754 );

  jedex_value_get_by_name( &element, "id", &field, &index );
  jedex_value_set_int( &field, 2 );

  jedex_value_get_by_name( &element, "load", &field, &index );
  cpu_load = 100.0 / ( 8.0 * 2 );
  jedex_value_set_double( &field, cpu_load );
}


static void
set_pm_port_status( jedex_value *array ) {
  jedex_value element;
  jedex_value_append( array, &element, NULL );

  jedex_value field;
  size_t index;
  jedex_value_get_by_name( &element, "pm_ip_address", &field, &index );
  jedex_value_set_int( &field, ( int32_t ) 2886730754 );

  jedex_value_get_by_name( &element, "if_index", &field, &index );
  jedex_value_set_int( &field, 1 );

  jedex_value_get_by_name( &element, "tx_bytes", &field, &index );
  jedex_value_set_long( &field, 344775858 );
}


static void
stats_collect_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( val );
  UNUSED( client_id );

  assert( user_data );
  responder *self = user_data;

  jedex_value_iface *union_class = jedex_generic_class_from_schema( self->reply_schema );
  jedex_value uval;
  jedex_generic_value_new( union_class, &uval );

  jedex_value top;
  size_t index;
  jedex_value_get_by_name( &uval, "statistics_status_reply", &top, &index );

  jedex_value array_0;
  jedex_value_get_by_name( &top, "pm_stats", &array_0, &index );
  set_pm_status( &array_0 );

  jedex_value array_1;
  jedex_value_get_by_name( &top, "pm_cpu_stats", &array_1, &index );
  set_pm_cpu_status( &array_1 );

  jedex_value array_2;
  jedex_value_get_by_name( &top, "pm_port_stats", &array_2, &index );
  set_pm_port_status( &array_2 );

  jedex_value array_3;
  jedex_value_get_by_name( &top, "vm_stats", &array_3, &index );

  jedex_value array_4;
  jedex_value_get_by_name( &top, "vm_port_stats", &array_4, &index );

  jedex_value array_5;
  jedex_value_get_by_name( &top, "vm_cpu_stats", &array_5, &index );

  jedex_value array_6;
  jedex_value_get_by_name( &top, "service_stats", &array_6, &index );

  printf( "stats collect callback called data %s\n", json );
  self->emirates->send_reply( self->emirates, STATS_COLLECT, &top );
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );
  responder *self = xmalloc( sizeof( *self ) );
  self->schema = jedex_initialize( "../../lib/schema/ipc" );
  self->request_schema = jedex_schema_get_subschema( self->schema, "nc_statistics_status_request" );
  self->reply_schema = self->schema;


  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, RESPONDER ) );
  if ( self->emirates != NULL ) {
    self->emirates->set_service_request( self->emirates, STATS_COLLECT, self->request_schema, self, stats_collect_callback );
    set_ready( self->emirates );

    emirates_loop( self->emirates );
    emirates_finalize( &self->emirates );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
