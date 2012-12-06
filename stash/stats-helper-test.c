/*
 * Copyright (C) 2008-2012 NEC Corporation
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
#include <string.h>
#include "cmockery_trema.h"
#include "openflow.h"
#include "wrapper.h"
#include "checks.h"
#include "ofdp_error.h"
#include "port_manager.h"
#include "switch_port.h"
#include "controller_manager.h"
#include "stats-helper.h"


#define SELECT_TIMEOUT_USEC 100000 
#define MAX_SEND_QUEUE 512
#define MAX_RECV_QUEUE 512
#define PORT_NO 2


static const uint8_t HW_ADDR[ OFP_ETH_ALEN ] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
static const char *DEV_NAME = "test_veth";


#undef get_ofp_port_structure
#define get_ofp_port_structure mock_get_ofp_port_structure
OFDPE get_ofp_port_structure( uint32_t port_no, ofp_port *out_port );


bool
mock_send_error_message( uint32_t transaction_id, uint16_t type, uint16_t code ) {
  check_expected( transaction_id );
  check_expected( type );
  check_expected( code );
  return true;
}

int
mock_time_now( struct timespec *now ) {
  UNUSED( now );
  return 1;
}


ether_device *
mock_create_ether_device( const char *name, const size_t max_send_queue, const size_t max_recv_queue ) {
  check_expected( name );
  check_expected( max_send_queue );
  check_expected( max_recv_queue );
  return ( ether_device * ) ( ( uint32_t ) mock() );
}


bool
mock_set_frame_received_handler( ether_device *device, frame_received_handler callback, void *user_data ) {
  UNUSED( device );
  UNUSED( callback );
  UNUSED( user_data );
  return true;
}


OFDPE
mock_send_for_notify_port_config( uint32_t port_no, uint8_t reason ) {
  check_expected( port_no );
  check_expected( reason );
  return OFDPE_SUCCESS;
}


void
mock_delete_ether_device( ether_device * device ) {
  UNUSED( device );
  return;
}


static void
create_port_desc( void **state ) {
  UNUSED( state );

  int ret = init_port_manager( SELECT_TIMEOUT_USEC, MAX_SEND_QUEUE, MAX_RECV_QUEUE );
  if ( ret != OFDPE_SUCCESS ) {
   return;
  }

  expect_string( mock_create_ether_device, name, DEV_NAME );
  expect_value( mock_create_ether_device, max_send_queue, MAX_SEND_QUEUE );
  expect_value( mock_create_ether_device, max_recv_queue, MAX_RECV_QUEUE );
  switch_port *port = ( switch_port * ) xmalloc( sizeof( switch_port ) );
  port->device = ( ether_device * ) xmalloc( sizeof( ether_device ) );
  memcpy( port->device->name, DEV_NAME, sizeof( port->device->name ) );
  memcpy( port->device->hw_addr, HW_ADDR, sizeof( port->device->hw_addr ) );
  will_return( mock_create_ether_device, port->device );

  expect_value( mock_send_for_notify_port_config, port_no, PORT_NO );
  expect_value( mock_send_for_notify_port_config, reason, OFPPR_ADD );

  char *device_name = xstrdup( DEV_NAME );
  ret = add_port( PORT_NO, device_name );
  xfree( device_name );
}


static void
create_group_features( void **state ) {
  init_group_table();
  struct ofp_group_features *features = ( struct ofp_group_features * ) xmalloc( sizeof( struct ofp_group_features ) );
  features->types = OFPGT_ALL;
  features->capabilities = OFPGFC_SELECT_WEIGHT | OFPGFC_SELECT_LIVENESS;
  features->max_groups[ OFPGT_ALL ] = 128;
  features->max_groups[ OFPGT_SELECT ] = 256;
  features->max_groups[ OFPGT_INDIRECT ] = 512;
  features->max_groups[ OFPGT_FF ] = 640;
  features->actions[ OFPGT_ALL ] = OFPAT_OUTPUT;
  features->actions[ OFPGT_SELECT ] = OFPAT_COPY_TTL_OUT;
  features->actions[ OFPGT_INDIRECT ] = OFPAT_PUSH_VLAN;
  features->actions[ OFPGT_FF ] = OFPAT_GROUP;
  set_group_features( features );
  *state = ( void * ) features;
}


static void
destroy_port_desc( void **state ) {
  UNUSED( state );
  finalize_port_manager();
}


static void
destroy_group_features( void **state ) {
  UNUSED( state );
  finalize_group_table();
}


static void
test_request_port_desc( void **state ) {
  UNUSED( state );

  list_element *list = request_port_desc();
  assert_true( list );
  struct ofp_port *ofp_port = list->data;
  assert_int_equal( ofp_port->port_no, PORT_NO );
  assert_string_equal( ofp_port->name, DEV_NAME );
  assert_memory_equal( ofp_port->hw_addr, HW_ADDR, sizeof( ofp_port->hw_addr ) );
}


static void
test_request_group_features( void **state ) {
  struct ofp_group_features *set_features = *state;
  struct ofp_group_features *features;
  
  features = request_group_features();
  assert_true( features );
  assert_memory_equal( features, set_features, sizeof( *features ) );
}


int
main() {
  const UnitTest tests[] = {
    unit_test_setup_teardown( test_request_port_desc, create_port_desc, destroy_port_desc ),
    unit_test_setup_teardown( test_request_group_features, create_group_features, destroy_group_features )
  };
  return run_tests( tests );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
