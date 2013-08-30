/*
 * Physical machine control platform snmp
 *
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


#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <string.h>
#include "wrapper.h"
#include "log_writer.h"
#include <pthread.h>
#include "snmp.h"
#include "cesc.h"


static int MAX_REPETITIONS = 50;
static int NON_REPEATERS = 0;
static int SNMP_RETRIES = 3;
static int SNMP_TIMEOUT = 100000;

static const char *OID_IPADENTADDR = "ipAdEntAddr";
static const char *OID_IPADENTIFINDEX = "ipAdEntIfIndex";
static const char *OID_IFPHYSADDRESS = "ifPhysAddress";
static const char *OID_IFSPEED = "ifSpeed";
static const char *OID_IFINOCTETS = "ifInOctets";
static const char *OID_IFOUTOCTETS = "ifOutOctets";
static const char *OID_HRPROCESSORLOAD = "hrProcessorLoad";
static const char *OID_HRSTORAGEDESCR = "hrStorageDescr";
static const char *OID_HRSTORAGEALLOCATIONUNITS = "hrStorageAllocationUnits";
static const char *OID_HRSTORAGESIZE = "hrStorageSize";
static const char *OID_HRSTORAGEUSED = "hrStorageUsed";
static const char *OID_NSEXTENDOUTPUT1LINE = "nsExtendOutput1Line";

static u_char COMMUNITY[] = "public";
static const uint32_t LOOPBACK_IP_ADDRESS = 2130706433;
static const char *DESCR_PHYSICAL = "Physical memory";

typedef struct {
  long value;
  int index;
} data_integer;

typedef struct {
  uint64_t value;
  int index;
} data_uint64;

typedef struct {
  u_char *value;
  int index;
} data_string;


static uint32_t
ip_to_uint32( const u_char *ip ) {
  uint32_t ip_network_byte_order = ( ( uint32_t )ip[ 0 ] ) +
                                   ( ( uint32_t )ip[ 1 ] << 8 ) +
                                   ( ( uint32_t )ip[ 2 ] << 16 ) +
                                   ( ( uint32_t )ip[ 3 ] << 24 );
  return ntohl( ip_network_byte_order );
}


static uint64_t
string_to_uint64( const u_char *string ) {
  uint64_t num = 0;   
  while( *string != '\0' ){
    num += ( uint64_t )(*string) - '0';
    num *= 10;
    string++;
  }
  num /= 10;
  return num;
}

static size_t
strlen_unsigned( u_char str[] ) {
  size_t i = 0;
  for( i = 0; str[i] != '\0'; i++ ) {
    ;
  }
  return i;
}

static netsnmp_session *
create_session( uint32_t ip_address ) {
  netsnmp_session *session = malloc( sizeof( netsnmp_session ) );
  snmp_sess_init( session );
  session->version = SNMP_VERSION_2c;
  session->community = COMMUNITY;
  session->community_len = strlen_unsigned( session->community );
  struct in_addr addr;
  addr.s_addr = htonl( ip_address );
  char *ip = inet_ntoa( addr );
  session->peername = ip;
  session->retries = SNMP_RETRIES;
  session->timeout = SNMP_TIMEOUT;
  return session;
}

static int
get_bulk_snmp( uint32_t ip_address, const char *target_oid, u_char *type, list_element *values )
{
  int exitval = 0;

  netsnmp_session *session = create_session( ip_address );

  oid name[ MAX_OID_LEN ];
  size_t name_length = MAX_OID_LEN;
  if ( snmp_parse_oid( target_oid, name, &name_length ) == NULL ) {
    snmp_perror( target_oid );
    free( session );
    exit( 1 );
  }

  SOCK_STARTUP;

  netsnmp_session *ss = snmp_open( session );
  if ( ss == NULL ) {
    snmp_sess_perror( "snmpbulkwalk", session );
    SOCK_CLEANUP;
    free( session );
    exit( 1 );
  }

  netsnmp_pdu *pdu = snmp_pdu_create( SNMP_MSG_GETBULK );
  pdu->non_repeaters = NON_REPEATERS;
  pdu->max_repetitions = MAX_REPETITIONS;
  snmp_add_null_var( pdu, name, name_length );

  netsnmp_pdu *response;
  int status = snmp_synch_response( ss, pdu, &response );
  if ( status == STAT_SUCCESS ) {
    if ( response->errstat == SNMP_ERR_NOERROR ) {
      for ( netsnmp_variable_list *vars = response->variables; vars != NULL; vars = vars->next_variable ) {
        if ( ( vars->name_length < name_length ) || ( memcmp( name, vars->name, name_length * sizeof( oid ) ) != 0 ) ) {
          continue;
        }
        *type = vars->type;
        switch ( vars->type ) {
          case ASN_INTEGER:
          case ASN_GAUGE:
          case ASN_COUNTER:
          case ASN_TIMETICKS: {
            data_integer *d = malloc( sizeof( data_integer ) );
            d->value = *(vars->val.integer);
            d->index = ( int ) vars->name_loc[ vars->name_length - 1 ];
            append_to_tail( &values, d );
            break;
          }
          case ASN_COUNTER64: {
            u_long high = vars->val.counter64->high;
            u_long low = vars->val.counter64->low;
            data_uint64 *d = xmalloc( sizeof( data_uint64 ) );
            d->value = ( ( uint64_t ) high << 32 ) + ( uint64_t ) low;
            d->index = ( int ) vars->name_loc[ vars->name_length - 1 ];
            append_to_tail( &values, d );
            break;
          }
          case ASN_OCTET_STR: {
            data_string *d = xmalloc( sizeof( data_string ) );
            d->index = (int ) vars->name_loc[ vars->name_length - 1 ];
            u_char *value = ( u_char * )malloc( vars->val_len + 1 );
            memset( value, 0, vars->val_len + 1 );
            memcpy( value, vars->val.string, vars->val_len );
            d->value = value;
            append_to_tail( &values, d );
            break;
          }
          case ASN_IPADDRESS: {
            data_string *d = xmalloc( sizeof( data_string ) );
            d->index = ( int )vars->name_loc[ vars->name_length - 1 ];
            d->value = xmalloc( sizeof( u_char ) * vars->val_len );
            memset( d->value, 0, vars->val_len );
            memcpy( d->value, vars->val.string, sizeof( u_char ) * vars->val_len );
            append_to_tail( &values, d );
            break;
          }
        }
      }
    } else if ( response->errstat == SNMP_ERR_NOSUCHNAME ) {
      log_err( "End of MIB" );
    } else {
      log_err( "Error in packet ( Reason: %ld )", response->errstat );
      exitval = 2;
    }
  } else if ( status == STAT_TIMEOUT ) {
    log_warning( "Timeout: No Response from %s", session->peername );
    exitval = 1;
  } else {
    snmp_sess_perror( "snmpbulkget", ss );
    exitval = 1;
  }

  if ( response ) {
    snmp_free_pdu( response );
  }

  snmp_close( ss );
  free( session );

  SOCK_CLEANUP;

  return exitval;
}


static bool
check_port_is_data_plane( uint32_t control_plane_ip_address, port_status *status ) {
  uint32_t port_ip_address = status->ip_address;
  if ( port_ip_address == control_plane_ip_address ) {
    return false;
  }
  if ( port_ip_address == LOOPBACK_IP_ADDRESS ) {
    return false;
  }

  return true;
}


static data_integer *
lookup_data_integer( list_element *l, int index ) {
  for ( const list_element *e = l; e != NULL; e = e->next ) {
    data_integer *d = e->data;
    if ( d->index == index ) {
      return d;
    }
  }

  return NULL;
}

/*
static data_uint64 *
lookup_data_uint64( list *l, int index ) {
  for ( const list_element *e = l->head; e != NULL; e = e->next ) {
    data_uint64 *d = e->data;
    if ( d->index == index ) {
      return d;
    }
  }
  return NULL;
}
*/
static data_string *
lookup_data_string( list_element *l, int index ) {
  for ( const list_element *e = l; e != NULL; e = e->next ) {
    data_string *d = e->data;
    if ( d->index == index ) {
      return d;
    }
  }

  return NULL;
}


static data_integer *
lookup_if_index_data( list_element *if_indices, long index ) {
  for ( const list_element *e = if_indices; e != NULL; e = e->next ) {
    data_integer *d = e->data;
    if ( d->value == index ) {
      return d;
    }
  }

  return NULL;
}


static data_string *
lookup_ip_address_data( list_element *ip_addresses, uint32_t ip_address ) {
  for ( const list_element *e = ip_addresses; e != NULL; e = e->next ) {
    data_string *d = e->data;
    if ( ip_to_uint32( d->value ) == ip_address ) {
      return d;
    }
  }

  return NULL;
}


static void 
update_port_statuses( list_element *port_statuses,
                      list_element *if_indices,
                      list_element *ip_addresses,
                      list_element *if_speeds,
                      list_element *tx_bytes,
                      list_element *rx_bytes ) {
  list_element *e = port_statuses;
  while ( e != NULL ) {
    port_status *p = e->data;
    data_integer *if_index_data = lookup_if_index_data( if_indices, ( long ) p->if_index );
    if ( if_index_data == NULL ) {
      e = e->next;
      delete_element( &port_statuses, p );
      free( p );
    } else {
      data_integer *if_speed_data = lookup_data_integer( if_speeds, ( int ) p->if_index );
      p->if_speed = ( uint32_t )( if_speed_data->value );

      data_integer *tx_byte_data = lookup_data_integer( tx_bytes, ( int ) p->if_index );
      p->tx_byte = 0xffffffffUL & ( uint64_t )( tx_byte_data->value );

      data_integer *rx_byte_data = lookup_data_integer( rx_bytes, ( int ) p->if_index );
      p->rx_byte = 0xffffffffUL & ( uint64_t ) ( rx_byte_data->value );

      delete_element( &if_indices, if_index_data );
      data_string *ip_address_data = lookup_ip_address_data( ip_addresses, p->ip_address );
      delete_element( &ip_addresses, ip_address_data );
      e = e->next;
    }
  }
}


static void
insert_port_statuses( uint32_t control_plane_ip_address, 
                      list_element *port_statuses,
                      list_element *if_indices,
                      list_element *ip_addresses,
                      list_element *mac_addresses,
                      list_element *if_speeds,
                      list_element *tx_bytes,
                      list_element *rx_bytes ) {
  list_element *if_index_element = if_indices;
  list_element *ip_address_element = ip_addresses;
  while ( if_index_element != NULL ) {
    port_status *port = xmalloc( sizeof( port_status ) );
    data_integer *if_index_data = if_index_element->data;
    int index = ( int )if_index_data->value;
    data_string *ip_address_data = ip_address_element->data;
    port->if_index = ( uint32_t )index;
    port->ip_address = ip_to_uint32( ip_address_data->value );

    data_string *mac_address_data = lookup_data_string( mac_addresses, index );
    port->mac_address = mac_to_uint64( mac_address_data->value );

    data_integer *if_speed_data = lookup_data_integer( if_speeds, index );
    port->if_speed = ( uint32_t )( if_speed_data->value );

    data_integer *tx_byte_data = lookup_data_integer( tx_bytes, index );
    port->tx_byte = 0xffffffffUL & ( uint64_t )( tx_byte_data->value );

    data_integer *rx_byte_data = lookup_data_integer( rx_bytes, index );
    port->rx_byte = 0xffffffffUL & ( uint64_t )( rx_byte_data->value );

    bool port_is_data_plane = check_port_is_data_plane( control_plane_ip_address, port );
    if ( port_is_data_plane == true ) {
      append_to_tail( &port_statuses, port );
    } else {
      free( port );
    }
    ip_address_element = ip_address_element->next;
    if_index_element = if_index_element->next;
  }
}


int
get_port_statuses( uint32_t control_plane_ip_address, list_element *port_statuses ){
  u_char type = 0;

  list_element *if_indices;
  create_list( &if_indices );
  int ret = get_bulk_snmp( control_plane_ip_address, OID_IPADENTIFINDEX, &type, if_indices );

  list_element *ip_addresses;
  create_list( &ip_addresses );
  ret &= get_bulk_snmp( control_plane_ip_address, OID_IPADENTADDR, &type, ip_addresses );
  
  list_element *mac_addresses;
  create_list( &mac_addresses );
  ret &= get_bulk_snmp( control_plane_ip_address, OID_IFPHYSADDRESS, &type, mac_addresses );

  list_element *if_speeds;
  create_list( &if_speeds );
  ret &= get_bulk_snmp( control_plane_ip_address, OID_IFSPEED, &type, if_speeds );

  list_element *tx_bytes;
  create_list( &tx_bytes );
  ret &= get_bulk_snmp( control_plane_ip_address, OID_IFOUTOCTETS, &type, tx_bytes );

  list_element *rx_bytes;
  create_list( &rx_bytes );
  ret &= get_bulk_snmp( control_plane_ip_address, OID_IFINOCTETS, &type, rx_bytes );
  if ( ret != RET_OK ) {
    return RET_NG;
  }

  update_port_statuses( port_statuses, if_indices, ip_addresses, if_speeds, tx_bytes, rx_bytes );
  insert_port_statuses( control_plane_ip_address, port_statuses, if_indices, ip_addresses, mac_addresses, if_speeds, tx_bytes, rx_bytes );
  
  delete_list( if_indices );
  delete_list( ip_addresses );
  delete_list( mac_addresses );
  delete_list( if_speeds );
  delete_list( tx_bytes );
  delete_list( rx_bytes );

  return RET_OK;
}


static cpu_status *
lookup_cpu_status( list_element *cpu_statuses, uint32_t id ) {
  for ( const list_element *e = cpu_statuses; e != NULL; e = e->next ) {
    cpu_status *c = e->data;
    if ( c->id == id ) {
      return c;
    }
  }

  return NULL;
}   


int
get_cpu_statuses( uint32_t ip_address, list_element *cpu_statuses ) {
  u_char type = 0;
  list_element *loads;
  create_list( &loads );

  int ret = get_bulk_snmp( ip_address, OID_HRPROCESSORLOAD, &type, loads );
  if ( ret != RET_OK ) {
     return RET_NG;
  }
  
  for ( const list_element *e = loads; e != NULL; e = e->next ) {
    data_integer *d = e->data;
    cpu_status *cpu = lookup_cpu_status( cpu_statuses, ( uint32_t ) d->index );
    if ( cpu == NULL ) {
      cpu = malloc( sizeof( cpu_status ) );
      cpu->id = ( uint32_t ) d->index;
      cpu->load = ( ( double ) d->value ) / 100;
      append_to_tail( &cpu_statuses, cpu );
    } else {
      cpu->id = ( uint32_t ) d->index;
      cpu->load = ( ( double ) d->value ) / 100;
    }
  }
  delete_list( loads );

  return RET_OK;
}


int
get_memory_status( uint32_t ip_address, memory_status *memory ) {
  u_char type = 0;
  list_element *descrs;
  create_list( &descrs );
  int ret = get_bulk_snmp( ip_address, OID_HRSTORAGEDESCR, &type, descrs );

  list_element *allocation_units;
  create_list( &allocation_units );
  ret &= get_bulk_snmp( ip_address, OID_HRSTORAGEALLOCATIONUNITS, &type, allocation_units );

  list_element *sizes;
  create_list( &sizes );
  ret &= get_bulk_snmp( ip_address, OID_HRSTORAGESIZE, &type, sizes );

  list_element *useds;
  create_list( &useds );
  ret &= get_bulk_snmp( ip_address, OID_HRSTORAGEUSED, &type, useds );
  if ( ret != RET_OK ) {
    return RET_NG;
  }

  for ( const list_element *e = descrs; e != NULL; e = e->next ) {
    data_string *d = e->data;
    char descr[100];
    memcpy( descr, d->value, sizeof( u_char ) * strlen( DESCR_PHYSICAL ) );
    if ( strncmp( descr, DESCR_PHYSICAL, strlen( DESCR_PHYSICAL ) )  == 0 ) {
      int index = d->index;
      data_integer *allocation_unit_data = lookup_data_integer( allocation_units, index );
      data_integer *size_data = lookup_data_integer( sizes, index );
      data_integer *used_data = lookup_data_integer( useds, index );
      memory->total = ( uint64_t ) ( size_data->value * ( allocation_unit_data->value / 1024 ) );
      memory->available = ( uint64_t ) ( ( size_data->value - used_data->value ) * ( allocation_unit_data->value / 1024 ) );
      return RET_OK;
    }
  }

  return RET_NG;
}


static bool
check_list_is_empty( list_element *l ) {
  uint32_t lsize = list_length_of( l );
  if ( !lsize ) {
    return true;
  } else {
    return false;
  }
}


int
get_service_module_statuses( uint32_t ip_address, list_element *service_module_statuses ) {
  u_char type = 0;

  list_element *extends;
  create_list( &extends );
  int ret = get_bulk_snmp( ip_address, OID_NSEXTENDOUTPUT1LINE, &type, extends );
  bool list_is_empty = check_list_is_empty( service_module_statuses );
  
  if ( list_is_empty == true ) {
    for ( const list_element *e = extends; e != NULL; e = e->next ) {
      data_string *d = e->data;
      uint64_t *service_module_status_uint64 = malloc( sizeof( uint64_t ) );
      *service_module_status_uint64 = string_to_uint64( d->value );
      append_to_tail( &service_module_statuses, service_module_status_uint64 );
    }
  } else {
    list_element *se = service_module_statuses;
    for ( const list_element *e = extends; e != NULL; e = e->next ) {
      data_string *d = e->data;
      uint64_t *service_module_status_uint64 = se->data;
      *service_module_status_uint64 = string_to_uint64( d->value );
    }
  }
  
  delete_list( extends );
  if ( ret != RET_OK ) {
     return RET_NG;
  }

  return RET_OK;
}


void
initialize_snmp(){
  init_snmp( "SNMP" );
}


void
finalize_snmp() {
  // do nothing
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
