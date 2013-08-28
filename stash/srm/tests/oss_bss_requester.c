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
#include "common_defs.h"


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
} requester;



static void
add_service_handler( uint32_t tx_id, jedex_value *val, const char *json, void *user_data ) {
  UNUSED( tx_id );
  UNUSED( val );
  UNUSED( user_data );
  printf( "add_service_handler called %s\n", json );
}


void
test_add_service( requester *self ) {
  jedex_value_iface *record_class = jedex_generic_class_from_schema( self->request_schema );
  jedex_value rval;
  jedex_generic_value_new( record_class, &rval );
  
  size_t index;
  jedex_value field;
  jedex_value_get_by_name( &rval, "service_name", &field, &index );
  jedex_value_set_string( &field, INTERNET_ACCESS_SERVICE );

  jedex_value_get_by_name( &rval, "bandwidth", &field, &index );
  double bandwidth = 0.0;
  jedex_value_set_double( &field, bandwidth );

  jedex_value_get_by_name( &rval, "nr_subscribers", &field, &index );
  jedex_value_set_long( &field, 100 );
  self->emirates->send_request( self->emirates, OSS_BSS_ADD_SERVICE, &rval, self->reply_schema );
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );

  requester *self = xmalloc( sizeof( *self ) );
  self->schema = jedex_initialize( "../../lib/schema/ipc" );
  self->request_schema = jedex_schema_get_subschema( self->schema, "oss_bss_add_service_request" );
  self->reply_schema = jedex_schema_get_subschema( self->schema, "common_reply" );

  int flag = 0;
  self->emirates = emirates_initialize_only( ENTITY_SET( flag, REQUESTER ) );
  if ( self->emirates != NULL ) {
    int32_t msecs = 60 * 1000; 
    self->emirates->set_request_expiry( self->emirates, OSS_BSS_ADD_SERVICE, msecs );
    self->emirates->set_service_reply( self->emirates, OSS_BSS_ADD_SERVICE, self, add_service_handler );
    test_add_service( self );
    emirates_loop( self->emirates );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
