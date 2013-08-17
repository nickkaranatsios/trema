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


typedef struct test_info {
  emirates_iface *iface;
  jedex_schema *add_service_schema;
  jedex_schema *add_service_reply_schema;
} test_info;


static void
set_add_service_reply( const char *status, jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "status", &field, &index ); 
  jedex_value_set_string( &field, status );
}


static void
request_add_service_srm_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( client_id );
  assert( user_data );
  test_info *test = user_data;

  jedex_value_iface *add_service_reply_class = jedex_generic_class_from_schema( test->add_service_reply_schema );
  jedex_value reply_val;
  jedex_generic_value_new( add_service_reply_class, &reply_val );
  if ( val ) {
    printf( "request_add_service_srm_callback called data %s\n", json );
    set_add_service_reply( "success", &reply_val );
    test->iface->send_reply( test->iface, "add_service", &reply_val );
  }
  else {
    test->iface->send_reply( test->iface, "add_service", &reply_val );
  }
  jedex_value_decref( &reply_val );
}


static void
request_add_service_sm_callback( jedex_value *val, const char *json, const char *client_id, void *user_data ) {
  UNUSED( client_id );
  assert( user_data );
  test_info *test = user_data;

  jedex_value_iface *add_service_reply_class = jedex_generic_class_from_schema( test->add_service_reply_schema );
  jedex_value reply_val;
  jedex_generic_value_new( add_service_reply_class, &reply_val );
  if ( val ) {
    printf( "request_add_service_sm_callback called data %s\n", json );
    set_add_service_reply( "success", &reply_val );
    test->iface->send_reply( test->iface, "add_service", &reply_val );
  }
  else {
    test->iface->send_reply( test->iface, "add_service", &reply_val );
  }
  jedex_value_decref( &reply_val );
}


/*
 * This test program receives the add_service_srm and add_service_sm requests
 * message from add_service_requester and sends a reply back to originator.
 */
int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );

  test_info test;
  jedex_schema *schema = jedex_initialize( "add_service" );
  const char *sub_schema[] = { "add_service", "add_service_reply", NULL };
  test.add_service_schema = jedex_schema_get_subschema( schema, sub_schema[ 0 ] );
  test.add_service_reply_schema = jedex_schema_get_subschema( schema, sub_schema[ 1 ] );

  
  int flag = 0;
  test.iface = emirates_initialize_only( ENTITY_SET( flag, RESPONDER ) );
  if ( test.iface != NULL ) {
    test.iface->set_service_request( test.iface, "add_service_srm", test.add_service_schema, &test, request_add_service_srm_callback );
    test.iface->set_service_request( test.iface, "add_service_sm", test.add_service_schema, &test, request_add_service_sm_callback );
    set_ready( test.iface );

    emirates_loop( test.iface );
    jedex_finalize( &schema );
    emirates_finalize( &test.iface );
  }

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
