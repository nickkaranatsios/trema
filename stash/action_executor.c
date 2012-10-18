/*
 * Functions for executing OpenFlow actions.
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "action_executor.h"
#include "port_manager.h"
#include "controller_manager.h"
#include "table_manager.h"
#include "packet_buffer_pool.h"
#include "packet_parser.h"
#include "utility.h"
#include "ether.h"
#include "ipv4.h"
#include "ipv6.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "ofdp.h"


/***************************************
  for unit test
***************************************/
#ifdef UNIT_TESTING
#include "../../unittests/test_mocks.h"
#endif /* UNIT_TESTING */


static void
set_ipv4_checksum( ipv4_header_t *header ) {
  header->csum = 0;
  header->csum = get_checksum( ( uint16_t * ) header, (uint32_t)sizeof( ipv4_header_t ) );
}


static uint32_t
get_sum( uint16_t *pos, size_t size ) {
  uint32_t sum = 0;

  for (; 2 <= size; pos++, size -= 2 ) {
    sum += *pos;
  }
  if ( size == 1 ) {
    sum += *( unsigned char * ) pos;
  }

  return sum;
}


static uint16_t
get_csum( uint32_t sum ) {
  while ( sum & 0xffff0000 ) {
    sum = ( sum & 0x0000ffff ) + ( sum >> 16 );
  }

  return ( uint16_t ) ~sum;
}


/**
 * get ipv4 pseudo header sum
 *
 * param ipv4 header
 * param protocol number
 * param payload size
 * @return ipv4 pseudo header sum
 */
uint32_t
get_ipv4_pseudo_header_sum( ipv4_header_t *ipv4_header, uint8_t protocol, size_t payload_size ) {
  uint32_t sum = 0;

  sum += get_sum( ( uint16_t * ) &ipv4_header->saddr, sizeof( ipv4_header->saddr ) );
  sum += get_sum( ( uint16_t * ) &ipv4_header->daddr, sizeof( ipv4_header->saddr ) );
  sum += ( ( uint32_t ) protocol ) << 8;
  sum += ( uint32_t )payload_size;

  return sum;
}


/**
 * get ipv6 pseudo header sum
 *
 * param ipv6 header
 * param protocol number
 * param payload size
 * @return ipv6 pseudo header sum
 */
uint32_t
get_ipv6_pseudo_header_sum( ipv6_header_t *ipv6_header, uint8_t protocol, size_t payload_size ) {
  uint32_t sum = 0;

  sum += get_sum( ( uint16_t * ) &ipv6_header->saddr[ 0 ], sizeof( ipv6_header->saddr ) );
  sum += get_sum( ( uint16_t * ) &ipv6_header->daddr[ 0 ], sizeof( ipv6_header->saddr ) );
  sum += ( uint32_t )payload_size;
  sum += ( ( uint32_t ) protocol ) << 8;
  return sum;
}


/**
 * get icmpv6 pseudo header sum
 *
 * param icmpv6 header
 * param payload size
 * @return ipv6 pseudo header sum
 */
uint32_t
get_icmpv6_pseudo_header_sum( ipv6_header_t *ipv6_header, size_t payload_size ) {
  uint32_t sum = 0;

  sum += get_sum( ( uint16_t * ) &ipv6_header->saddr[ 0 ], sizeof( ipv6_header->saddr ) );
  sum += get_sum( ( uint16_t * ) &ipv6_header->daddr[ 0 ], sizeof( ipv6_header->saddr ) );
  sum += ( uint32_t )( IPPROTO_ICMPV6 ) << 8;
  sum += ( uint32_t )payload_size;
  return sum;
}


static void
set_ipv4_udp_checksum( ipv4_header_t *ipv4_header, udp_header_t *udp_header, void *payload ) {
  uint32_t csum = 0;

  csum += get_ipv4_pseudo_header_sum( ipv4_header, IPPROTO_UDP, udp_header->len );
  udp_header->csum = 0;
  csum += get_sum( ( uint16_t * ) udp_header, sizeof( udp_header_t ) );
  csum += get_sum( payload, ntohs( udp_header->len ) - sizeof( udp_header_t ) );
  udp_header->csum = get_csum( csum );
}


static void
set_ipv6_udp_checksum( ipv6_header_t *ipv6_header, udp_header_t *udp_header, void *payload ) {
  uint32_t csum = 0;

  csum += get_ipv6_pseudo_header_sum( ipv6_header, IPPROTO_UDP, udp_header->len );
  udp_header->csum = 0;
  csum += get_sum( ( uint16_t * ) udp_header, sizeof( udp_header_t ) );
  csum += get_sum( payload, ntohs( udp_header->len ) - sizeof( udp_header_t ) );
  udp_header->csum = get_csum( csum );
}


static void
set_ipv4_tcp_checksum( ipv4_header_t *ipv4_header, tcp_header_t *tcp_header, void *payload, size_t payload_length ) {
  uint32_t csum = 0;

  csum += get_ipv4_pseudo_header_sum( ipv4_header, IPPROTO_TCP, payload_length );
  tcp_header->csum = 0;
  csum += get_sum( ( uint16_t * ) tcp_header, sizeof( tcp_header_t ) );
  csum += get_sum( payload, payload_length );
  tcp_header->csum = get_csum( csum );
}


static void
set_ipv6_tcp_checksum( ipv6_header_t *ipv6_header, tcp_header_t *tcp_header, void *payload, size_t payload_length ) {
  uint32_t csum = 0;

  csum += get_ipv6_pseudo_header_sum( ipv6_header, IPPROTO_TCP, payload_length );
  tcp_header->csum = 0;
  csum += get_sum( ( uint16_t * ) tcp_header, sizeof( tcp_header_t ) );
  csum += get_sum( payload, payload_length );
  tcp_header->csum = get_csum( csum );
}


static void
set_icmpv4_checksum( icmp_header_t *header, size_t length ) {
  header->csum = 0;
  header->csum = get_checksum( ( uint16_t * ) header, (uint32_t)length );
}


static void
set_icmpv6_checksum( ipv6_header_t *ipv6_header, icmp_header_t *icmp_header, size_t length ) {
  uint32_t csum = 0;

  csum += get_icmpv6_pseudo_header_sum( ipv6_header, length );
  icmp_header->csum = 0;
  csum += get_sum( ( uint16_t * ) icmp_header, length );
  icmp_header->csum = get_csum( csum );
}


static bool
parse_frame( buffer *frame ) {
  assert( frame != NULL );

  bool ret = true;
  uint32_t temp_port_no = 0;
  uint64_t temp_metadata = 0;

  if ( frame->user_data != NULL ) {
    temp_port_no = ( ( packet_info * ) frame->user_data )->eth_in_port;
    temp_metadata = ( ( packet_info * ) frame->user_data )->metadata;
    free_packet_info( frame );
  }

  if ( !parse_packet( frame ) ) {
    error( "Failed to parse an Ethernet frame." );
    ret = false;
  }

  if ( ret ) {
      ( ( packet_info * ) frame->user_data )->eth_in_port = temp_port_no;
      ( ( packet_info * ) frame->user_data )->metadata = temp_metadata;
  }

  return ret;
}


static packet_info *
get_packet_info_data( const buffer *frame ) {
  assert( frame != NULL );
  return ( packet_info * ) frame->user_data;
}


static void
set_address( void *dst, match8 value[], size_t size ) {
  assert( dst != NULL );
  assert( value != NULL );
  uint8_t *tmp = ( uint8_t * ) dst;
  for ( size_t i = 0; i < size; i++ ) {
    tmp[ i ] = value[ i ].value;
  }
}


static void
set_dl_address( void *dst, match8 value[] ) {
  assert( dst != NULL );
  assert( value != NULL );
  set_address( dst, &value[ 0 ], ETH_ADDRLEN );
}


static void
set_ipv6_address( void *dst, match8 value[] ) {
  assert( dst != NULL );
  assert( value != NULL );
  set_address( dst, &value[ 0 ], IPV6_ADDRLEN );
}


static bool
set_dl_dst( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ether( frame ) ) {
    warn( "A non-ethernet frame type (%#x) found when trying to set the destination data link address.", info->format );
    return true;
  }

  ether_header_t *header = info->l2_header;
  set_dl_address( header->macda, value );

  return parse_frame( frame );
}


static bool
set_dl_src( buffer *frame, match8 value[]  ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ether( frame ) ) {
    warn( "A non-ethernet frame type (%#x) found when trying to set the source data link address.", info->format );
    return true;
  }

  ether_header_t *header = info->l2_header;
  set_dl_address( header->macsa, value );

  return parse_frame( frame );
}


static bool
set_dl_type( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ether( frame ) ) {
    warn( "A non-ethernet frame type (%#x) found when trying to set the data link type.", info->format );
    return true;
  }

  ether_header_t *header = info->l2_header;
  header->type = htons( value );

  return parse_frame( frame );
}


static bool
set_vlan_vid( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_vtag( frame ) ) {
    warn( "A non-vlan ethernet frame type (%#x) found when trying to set the vlan-id.", info->format );
    return true;
  }

  vlantag_header_t *header = info->l2_vlan_header;
  header->tci = ( uint16_t ) ( ( header->tci & htons( 0xf000 ) ) | value );

  return parse_frame( frame );
}


static bool
set_vlan_pcp( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_vtag( frame ) ) {
    warn( "A non-vlan ethernet frame type (%#x) found when trying to set the priority code point.", info->format );
    return true;
  }

  vlantag_header_t *header = info->l2_vlan_header;
  header->tci = ( uint16_t ) ( ( header->tci & htons( 0x1fff ) ) | ( ( value & 0x07 ) << 5 ) );

  return parse_frame( frame );
}


static bool
set_nw_dscp( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) ) {
    warn( "A non-ipv4 ethernet frame type (%#x) found when trying to set the dscp field.", info->format );
    return true;
  }

  ipv4_header_t *header = info->l3_header;
  header->tos = (uint8_t)(( header->tos & 0x03 ) | ( ( value << 2 ) & 0xFC ));
  set_ipv4_checksum( header );

  return parse_frame( frame );
}


static bool
set_nw_ecn( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) ) {
    warn( "A non-ipv4 ethernet frame type (%#x) found when trying to set the ecn field.", info->format );
    return true;
  }

  ipv4_header_t *header = info->l3_header;
  header->tos = (uint8_t)(( header->tos & 0xFC ) | ( value & 0x03 ));
  set_ipv4_checksum( header );

  return parse_frame( frame );
}


static bool
set_ip_proto( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) ) {
    warn( "A non-ipv4 ethernet frame type (%#x) found when trying to set the ip_proto field.", info->format );
    return true;
  }

  ipv4_header_t *header = info->l3_header;
  header->protocol = value;
  set_ipv4_checksum( header );

  return parse_frame( frame );
}


static bool
set_ipv4_src( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) ) {
    warn( "A non-ipv4 ethernet frame type (%#x) found when trying to set the source address.", info->format );
    return true;
  }

  ipv4_header_t *header = info->l3_header;
  header->saddr = htonl( value );
  set_ipv4_checksum( header );

  return parse_frame( frame );
}


static bool
set_ipv4_dst( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) ) {
    warn( "A non-ipv4 ethernet frame type (%#x) found when trying to set the destination address.", info->format );
    return true;
  }

  ipv4_header_t *header = info->l3_header;
  header->daddr  = htonl( value );
  set_ipv4_checksum( header );

  return parse_frame( frame );
}


static bool
set_tcp_src( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4_tcp( frame ) ) {
    warn( "A non-ipv4 tcp ethernet frame type (%#x) found when trying to set the tcp source port.", info->format );
    return true;
  }

  tcp_header_t *tcp_header = info->l4_header;
  tcp_header->src_port = htons( value );

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = info->l3_header;
    set_ipv4_tcp_checksum( ipv4_header, tcp_header, info->l4_payload, info->l4_payload_length );
  }
  else {
    ipv6_header_t *ipv6_header = info->l3_header;
    set_ipv6_tcp_checksum( ipv6_header, tcp_header, info->l4_payload, info->l4_payload_length );
  }

  return parse_frame( frame );
}


static bool
set_tcp_dst( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4_tcp( frame ) ) {
    warn( "A non-ipv4 tcp ethernet frame type (%#x) found when trying to set the tcp destination port.", info->format );
    return true;
  }

  tcp_header_t *tcp_header = info->l4_header;
  tcp_header->dst_port = htons( value );

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = info->l3_header;
    set_ipv4_tcp_checksum( ipv4_header, tcp_header, info->l4_payload, info->l4_payload_length );
  }
  else {
    ipv6_header_t *ipv6_header = info->l3_header;
    set_ipv6_tcp_checksum( ipv6_header, tcp_header, info->l4_payload, info->l4_payload_length );
  }

  return parse_frame( frame );
}


static bool
set_udp_src( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4_udp( frame ) ) {
    warn( "A non-ipv4 udp ethernet frame type (%#x) found when trying to set the udp source port.", info->format );
    return true;
  }
  udp_header_t *udp_header = info->l4_header;
  udp_header->src_port = htons( value );

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = info->l3_header;
    set_ipv4_udp_checksum( ipv4_header, udp_header, info->l4_payload );
  }
  else {
    ipv6_header_t *ipv6_header = info->l3_header;
    set_ipv6_udp_checksum( ipv6_header, udp_header, info->l4_payload );
  }

  return parse_frame( frame );
}


static bool
set_udp_dst( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4_udp( frame ) ) {
    warn( "A non-ipv4 udp ethernet frame type (%#x) found when trying to set the udp destination port.", info->format );
    return true;
  }
  udp_header_t *udp_header = info->l4_header;
  udp_header->dst_port = htons( value );

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = info->l3_header;
    set_ipv4_udp_checksum( ipv4_header, udp_header, info->l4_payload );
  }
  else {
    ipv6_header_t *ipv6_header = info->l3_header;
    set_ipv6_udp_checksum( ipv6_header, udp_header, info->l4_payload );
  }

  return parse_frame( frame );
}


static bool
set_icmpv4_type( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv4( frame ) ) {
    warn( "A non-icmpv4 frame type (%#x) found when trying to set the type field.", info->format );
    return true;
  }

  icmp_header_t *icmp_header = info->l4_header;
  icmp_header->type = value;

  set_icmpv4_checksum( icmp_header, info->l4_payload_length );

  return parse_frame( frame );
}


static bool
set_icmpv4_code( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv4( frame ) ) {
    warn( "A non-icmpv4 frame type (%#x) found when trying to set the code field.", info->format );
    return true;
  }

  icmp_header_t *icmp_header = info->l4_header;
  icmp_header->code = value;

  set_icmpv4_checksum( icmp_header, info->l4_payload_length );

  return parse_frame( frame );
}


static bool
set_arp_op( buffer *frame, uint16_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_arp( frame ) ) {
    warn( "A non-arpv4 frame type (%#x) found when trying to set the opcode field.", info->format );
    return true;
  }

  arp_header_t *header = info->l3_header;
  header->ar_op = htons( value );

  return parse_frame( frame );
}


// TODO should rename set_arp_spa => set_arp_sip
static bool
set_arp_spa( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_arp( frame ) ) {
    warn( "A non-arpv4 frame type (%#x) found when trying to set the source ip address.", info->format );
    return true;
  }

  arp_header_t *header = info->l3_header;
  header->sip  = htonl( value );

  return parse_frame( frame );
}

// TODO should rename set_arp_tpa => set_arp_tip
static bool
set_arp_tpa( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_arp( frame ) ) {
    warn( "A non-arpv4 frame type (%#x) found when trying to set the destination (target) ip address.", info->format );
    return true;
  }

  arp_header_t *header = info->l3_header;
  header->tip = htonl( value );

  return parse_frame( frame );
}


static bool
set_arp_sha( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_arp( frame ) ) {
    warn( "A non-arpv4 frame type (%#x) found when trying to set the source mac address.", info->format );
    return true;
  }

  arp_header_t *header = info->l3_header;
  set_dl_address( header->sha, value );

  return parse_frame( frame );
}


static bool
set_arp_tha( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_arp( frame ) ) {
    warn( "A non-arpv4 frame type (%#x) found when trying to set the destination (target) mac address.", info->format );
    return true;
  }

  arp_header_t *header = info->l3_header;
  set_dl_address( header->tha, value );

  return parse_frame( frame );
}


static bool
set_ipv6_src( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv6 frame type (%#x) found when trying to set the source ip address.", info->format );
    return true;
  }

  ipv6_header_t *header = info->l3_header;
  set_ipv6_address( header->saddr, value );

  return parse_frame( frame );
}


static bool
set_ipv6_dst( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv6 frame type (%#x) found when trying to set the destination ip address.", info->format );
    return true;
  }

  ipv6_header_t *header = info->l3_header;
  set_ipv6_address( header->daddr, value );

  return parse_frame( frame );
}


static bool
set_ipv6_flabel( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv6 frame type (%#x) found when trying to set the flow label.", info->format );
    return true;
  }

  ipv6_header_t *header = info->l3_header;
  header->hdrctl = ( header->hdrctl & htonl( 0xfff00000 ) ) | ( htonl( value ) & htonl( 0x000fffff ) );

  return parse_frame( frame );
}


static bool
set_icmpv6_type( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv6( frame ) && !packet_type_icmpv4( frame ) ) {
    warn( "A non-icmpv6 frame type (%#x) found when trying to set the type field.", info->format );
    return true;
  }

  icmp_header_t *icmp_header = info->l4_header;
  icmp_header->type = value;
  ipv6_header_t *ipv6_header = info->l3_header;
  set_icmpv6_checksum( ipv6_header, icmp_header, info->l4_payload_length );

  return parse_frame( frame );
}


static bool
set_icmpv6_code( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv6( frame ) && !packet_type_icmpv4( frame ) ) {
    warn( "A non-icmpv6 frame type (%#x) found when trying to set the code field.", info->format );
    return true;
  }

  icmp_header_t *icmp_header = info->l4_header;
  icmp_header->code = value;

  ipv6_header_t *ipv6_header = info->l3_header;
  set_icmpv6_checksum( ipv6_header, icmp_header, info->l4_payload_length );

  return parse_frame( frame );
}


static bool
set_ipv6_nd_target( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv6( frame ) ) {
    warn( "A non-icmpv6 frame type (%#x) found when trying to set the neighbor target address.", info->format );
    return true;
  }

  if ( ( info->icmpv6_type != ICMPV6_TYPE_NEIGHBOR_SOL )
     && ( info->icmpv6_type != ICMPV6_TYPE_NEIGHBOR_ADV ) ) {
    warn( "Unsupported icmpv6 type (%#x) found when trying to set the neighbor target address.", info->icmpv6_type );
    return true;
  }

  icmpv6_header_t *header = info->l3_payload;
  icmpv6data_ndp_t *icmpv6data_ndp = ( icmpv6data_ndp_t * ) header->data;
  set_ipv6_address( icmpv6data_ndp->nd_target, &value[ 0 ] );

  return parse_frame( frame );
}


static bool
set_ipv6_nd_sll( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );

  if ( !packet_type_icmpv6( frame ) ) {
    warn( "A non-icmpv6 frame type (%#x) found when trying to set the source link-layer address.", info->format );
    return true;
  }

  if ( info->icmpv6_type != ICMPV6_TYPE_NEIGHBOR_SOL ) {
    warn( "Unsupported icmpv6 type (%#x) found when trying to set the source link-layer address.", info->icmpv6_type );
    return true;
  }

  icmpv6_header_t *header = info->l3_payload;
  icmpv6data_ndp_t *icmpv6data_ndp = ( icmpv6data_ndp_t * ) header->data;

  if ( icmpv6data_ndp->ll_type != ICMPV6_ND_SOURCE_LINK_LAYER_ADDRESS ) {
    warn( "Incorrect link-layer address type (%#x) found when trying to set the source link-layer address.", icmpv6data_ndp->ll_type );
    return true;
  }
  set_dl_address( icmpv6data_ndp->ll_addr, &value[ 0 ] );

  return parse_frame( frame );
}


static bool
set_ipv6_nd_tll( buffer *frame, match8 value[] ) {
  assert( frame != NULL );
  assert( value != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_icmpv6( frame ) ) {
    warn( "A non-icmpv6 frame type (%#x) found when trying to set the target link-layer address.", info->format );
    return true;
  }

  if ( info->icmpv6_type != ICMPV6_TYPE_NEIGHBOR_ADV ) {
    warn( "Unsupported icmpv6 type (%#x) found when trying to set the target link-layer address.", info->icmpv6_type );
    return true;
  }

  icmpv6_header_t *header = info->l3_payload;
  icmpv6data_ndp_t *icmpv6data_ndp = ( icmpv6data_ndp_t * ) header->data;

  if ( icmpv6data_ndp->ll_type != ICMPV6_ND_TARGET_LINK_LAYER_ADDRESS ) {
    warn( "Incorrect link-layer address type (%#x) found when trying to set the target link-layer address.", icmpv6data_ndp->ll_type );
    return true;
  }
  set_dl_address( icmpv6data_ndp->ll_addr, &value[ 0 ] );

  return parse_frame( frame );
}


static bool
set_mpls_label( buffer *frame, uint32_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to set the mpls label.", info->format );
    return true;
  }

  uint32_t *mpls = get_packet_info_data( frame )->l2_mpls_header;
  *mpls = ( *mpls & htonl( 0x00000fff ) ) | htonl( ( value & 0x000fffff ) << 12 );

  return parse_frame( frame );
}


static bool
set_mpls_tc( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to set the traffic class.", info->format );
    return true;
  }

  uint32_t *mpls = get_packet_info_data( frame )->l2_mpls_header;
  *mpls = ( *mpls & htonl( 0xFFFFF1FF ) ) | htonl( ( uint32_t )( value & 0x07 ) << 9 );

  return parse_frame( frame );
}


static bool
set_mpls_bos( buffer *frame, uint8_t value ) {
  assert( frame != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to set the bottom of stack bit.", info->format );
    return true;
  }

  uint32_t *mpls = get_packet_info_data( frame )->l2_mpls_header;
  *mpls = ( *mpls & htonl( 0xFFFFFEFF ) ) | htonl( ( uint32_t )( value & 0x01 ) << 8 );

  return parse_frame( frame );
}


static void
push_linklayer_tag( buffer *frame, void *head, size_t tag_size ) {
  assert( frame != NULL );
  assert( head != NULL );

  char *tail = ( char * ) head + tag_size;
  size_t length = frame->length - ( size_t ) ( ( char * ) head - ( char * ) frame->data );
  append_back_buffer( frame, tag_size );
  memmove( tail, head, length );
  memset( head, 0, tag_size );
}


static void
pop_linklayer_tag( buffer *frame, void *head, size_t tag_size ) {
  assert( frame != NULL );
  assert( head != NULL );

  char *tail = ( char * ) head + tag_size;
  size_t length = frame->length - ( ( size_t )( ( char * ) head - ( char * ) frame->data ) + tag_size );
  memmove( head, tail, length );
  frame->length -= tag_size;
}


static void
push_vlan_tag( buffer *frame, void *head ) {
  assert( frame != NULL );
  assert( head != NULL );

  push_linklayer_tag( frame, head, sizeof( vlantag_header_t ) );
}


static void
pop_vlan_tag( buffer *frame, void *head ) {
  assert( frame != NULL );
  assert( head != NULL );

  pop_linklayer_tag( frame, head, sizeof( vlantag_header_t ) );
}


static void
push_mpls_tag( buffer *frame, void *head ) {
  assert( frame != NULL );
  assert( head != NULL );

  push_linklayer_tag( frame, head, sizeof( uint32_t ) );
}


static void
pop_mpls_tag( buffer *frame, void *head ) {
  assert( frame != NULL );
  assert( head != NULL );

  pop_linklayer_tag( frame, head, sizeof( uint32_t ) );
}


static bool
decrement_ttl( uint8_t *ttl ) {
  assert( ttl != NULL );

  if ( *ttl == 0 ) {
    return false;
  }

    ( *ttl )--;

  return true;
}


static bool
execute_action_copy_ttl_in( buffer *frame, action *copy_ttl_in ) {
  assert( frame != NULL );
  assert( copy_ttl_in != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to retrieve the ttl field.", info->format );
    return true;
  }


  if ( !packet_type_ipv4( frame ) && !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv4/6 frame type (%#x) found when trying to set the ttl field.", info->format );
    return true;
  }

  uint32_t mpls = *( uint32_t * ) info->l2_mpls_header;
  uint8_t ttl = ( uint8_t )( mpls >> 24 );

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = ( ipv4_header_t * ) info->l3_header;
    ipv4_header->ttl = ttl;
    set_ipv4_checksum( ipv4_header );
  }
  else {
    ipv6_header_t *ipv6_header = ( ipv6_header_t * ) info->l3_header;
    ipv6_header->hoplimit = ttl;
  }

  return parse_frame( frame );
}


static bool
execute_action_pop_mpls( buffer *frame, action *pop_mpls ) {
  assert( frame != NULL );
  assert( pop_mpls != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to retrieve the mpls label.", info->format );
    return true;
  }

  pop_mpls_tag( frame, info->l2_mpls_header );

  uint16_t next_type = 0;
  if ( packet_type_ipv4( frame ) ) {
    next_type = ETH_ETHTYPE_IPV4;
  }
  else {
    next_type = ETH_ETHTYPE_IPV6;
  }

  ether_header_t *ether_header = ( ether_header_t * ) frame->data;
  ether_header->type = htons( next_type );

  return parse_frame( frame );
}


static bool
execute_action_pop_vlan( buffer *frame, action *pop_vlan ) {
  assert( frame != NULL );
  assert( pop_vlan != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_vtag( frame ) ) {
    warn( "A non-vlan frame type (%#x) found when trying to pop a vlan tag.", info->format );
    return true;
  }

  pop_vlan_tag( frame, info->l2_vlan_header );

  uint16_t next_type = 0;
  if ( packet_type_ipv4( frame ) ) {
    next_type = ETH_ETHTYPE_IPV4;
  }
  else {
    next_type = ETH_ETHTYPE_IPV6;
  }

  ether_header_t *ether_header = ( ether_header_t * ) frame->data;
  ether_header->type = htons( next_type );

  return parse_frame( frame );
}


static bool
execute_action_push_mpls( buffer *frame, action *push_mpls ) {
  assert( frame != NULL );
  assert( push_mpls != NULL );

  packet_info *info = get_packet_info_data( frame );
  void *start = info->l2_mpls_header;
  if ( start == NULL ) {
    start = info->l2_payload;
  }

  push_mpls_tag( frame, start );
  ether_header_t *ether_header = ( ether_header_t * ) frame->data;
  ether_header->type = htons( push_mpls->ethertype );

  uint32_t *mpls = ( uint32_t * ) start;
  *mpls = *mpls | htonl( 0x00000100 );

  return parse_frame( frame );
}


static bool
execute_action_push_vlan( buffer *frame, action *push_vlan ) {
  assert( frame != NULL );
  assert( push_vlan != NULL );

  packet_info *info = get_packet_info_data( frame );
  void *start = info->l2_vlan_header;
  if ( start == NULL ) {
    start = info->l2_payload;
  }

  push_vlan_tag( frame, start );
  ether_header_t *ether_header = ( ether_header_t * ) frame->data;
  ether_header->type = htons( push_vlan->ethertype );
  vlantag_header_t *vlan_header = ( vlantag_header_t * ) start;

  uint16_t next_type = 0;
  if ( packet_type_ipv4( frame ) ) {
    next_type = ETH_ETHTYPE_IPV4;
  }
  else {
    next_type = ETH_ETHTYPE_IPV6;
  }

  vlan_header->type = htons( next_type );

  return parse_frame( frame );
}


static bool
execute_action_copy_ttl_out( buffer *frame, action *copy_ttl_out ) {
  assert( frame != NULL );
  assert( copy_ttl_out != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to replace the mpls ttl.", info->format );
    return true;
  }

  if ( !packet_type_ipv4( frame ) && !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv4/6 frame type (%#x) found when trying to set the ttl field.", info->format );
    return true;
  }

  uint8_t ttl = 0;

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *ipv4_header = ( ipv4_header_t * ) info->l3_header;
    ttl = ipv4_header->ttl;
  }
  else {
    ipv6_header_t *ipv6_header = ( ipv6_header_t * ) info->l3_header;
    ttl = ipv6_header->hoplimit;
  }

  uint8_t *mpls_ttl = ( ( uint8_t * ) info->l2_mpls_header ) + 3;
  *mpls_ttl = ttl;

  return parse_frame( frame );
}


static bool
execute_action_dec_mpls_ttl( buffer *frame, action *dec_mpls_ttl ) {
  assert( frame != NULL );
  assert( dec_mpls_ttl != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to decrement the mpls ttl.", info->format );
    return true;
  }

  uint8_t *ttl = ( uint8_t * ) info->l2_mpls_header + 3;

  if ( !decrement_ttl( ttl ) ) {
    notify_parameter_packet_in parameter;
    parameter.cookie = dec_mpls_ttl->entry->cookie;
    parameter.reason = OFPR_INVALID_TTL;
    parameter.max_len = MISS_SEND_LEN;
    parameter.packet = frame;
    parameter.table_id = dec_mpls_ttl->entry->table_id;
    memcpy( &parameter.match, dec_mpls_ttl->entry->p_match, sizeof( parameter.match ) );
    notify_packet_in( &parameter );
  }

  return parse_frame( frame );
}


static bool
execute_action_dec_nw_ttl( buffer *frame, action *dec_nw_ttl ) {
  assert( frame != NULL );
  assert( dec_nw_ttl != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) && !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv4/6 frame type (%#x) found when trying to decrement the ttl field.", info->format );
    return true;
  }

  uint8_t *ttl = NULL;
  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *header = ( ipv4_header_t * ) info->l3_header;
    ttl = &header->ttl;
    set_ipv4_checksum( header );
  }
  else {
    ipv6_header_t *header = ( ipv6_header_t * ) info->l3_header;
    ttl = &header->hoplimit;
  }

  if ( !decrement_ttl( ttl ) ) {
    notify_parameter_packet_in parameter;
    parameter.cookie = dec_nw_ttl->entry->cookie;
    parameter.reason = OFPR_INVALID_TTL;
    parameter.max_len = MISS_SEND_LEN;
    parameter.packet = frame;
    parameter.table_id = dec_nw_ttl->entry->table_id;
    memcpy( &parameter.match, dec_nw_ttl->entry->p_match, sizeof( parameter.match ) );
    notify_packet_in( &parameter );
  }

  return parse_frame( frame );
}


static bool
execute_action_set_mpls_ttl( buffer *frame, action *set_mpls_ttl ) {
  assert( frame != NULL );
  assert( set_mpls_ttl != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_eth_mpls( frame ) ) {
    warn( "A non-mpls frame type (%#x) found when trying to set the mpls ttl.", info->format );
    return true;
  }

  uint8_t *ttl = ( uint8_t * ) info->l2_mpls_header + 3;
  *ttl = set_mpls_ttl->mpls_ttl;

  return parse_frame( frame );
}


static bool
execute_action_set_nw_ttl( buffer *frame, action *set_nw_ttl ) {
  assert( frame != NULL );
  assert( set_nw_ttl != NULL );

  packet_info *info = get_packet_info_data( frame );
  if ( !packet_type_ipv4( frame ) && !packet_type_ipv6( frame ) ) {
    warn( "A non-ipv4/6 frame type (%#x) found when trying to set the ttl field.", info->format );
    return true;
  }

  if ( packet_type_ipv4( frame ) ) {
    ipv4_header_t *header = ( ipv4_header_t * ) info->l3_header;
    header->ttl = set_nw_ttl->nw_ttl;
    set_ipv4_checksum( header );
  }
  else {
    ipv6_header_t *header = ( ipv6_header_t * ) info->l3_header;
    header->hoplimit = set_nw_ttl->nw_ttl;
  }

  return parse_frame( frame );
}


static bool
execute_action_set_field( buffer *frame, action *set_field ) {
  assert( frame != NULL );
  assert( set_field != NULL );

  match *p_match = set_field->p_match;

  if ( p_match->eth_dst[ 0 ].valid ) {
    if ( !set_dl_dst( frame, p_match->eth_dst ) ) {
      return false;
    }
  }

  if ( p_match->eth_src[ 0 ].valid ) {
    if ( !set_dl_src( frame, p_match->eth_src ) ) {
      return false;
    }
  }

  if ( p_match->eth_type.valid ) {
    if ( !set_dl_type( frame, p_match->eth_type.value ) ) {
      return false;
    }
  }

  if ( p_match->vlan_vid.valid ) {
    if ( !set_vlan_vid( frame, p_match->vlan_vid.value ) ) {
      return false;
    }
  }

  if ( p_match->vlan_pcp.valid ) {
    if ( !set_vlan_pcp( frame, p_match->vlan_pcp.value ) ) {
      return false;
    }
  }

  if ( p_match->ip_dscp.valid ) {
    if ( !set_nw_dscp( frame, p_match->ip_dscp.value ) ) {
      return false;
    }
  }

  if ( p_match->ip_ecn.valid ) {
    if ( !set_nw_ecn( frame, p_match->ip_ecn.value ) ) {
      return false;
    }
  }

  if ( p_match->ip_proto.valid ) {
    if ( !set_ip_proto( frame, p_match->ip_proto.value ) ) {
      return false;
    }
  }

  if ( p_match->ipv4_src.valid ) {
    if ( !set_ipv4_src( frame, p_match->ipv4_src.value ) ) {
      return false;
    }
  }

  if ( p_match->ipv4_dst.valid ) {
    if ( !set_ipv4_dst( frame, p_match->ipv4_dst.value ) ) {
      return false;
    }
  }

  if ( p_match->tcp_src.valid ) {
    if ( !set_tcp_src( frame, p_match->tcp_src.value ) ) {
      return false;
    }
  }

  if ( p_match->tcp_dst.valid ) {
    if ( !set_tcp_dst( frame, p_match->tcp_dst.value ) ) {
      return false;
    }
  }

  if ( p_match->udp_src.valid ) {
    if ( !set_udp_src( frame, p_match->udp_src.value ) ) {
      return false;
    }
  }

  if ( p_match->udp_dst.valid ) {
    if ( !set_udp_dst( frame, p_match->udp_dst.value ) ) {
      return false;
    }
  }

  if ( p_match->icmpv4_type.valid ) {
    if ( !set_icmpv4_type( frame, p_match->icmpv4_type.value ) ) {
      return false;
    }
  }

  if ( p_match->icmpv4_code.valid ) {
    if ( !set_icmpv4_code( frame, p_match->icmpv4_code.value ) ) {
      return false;
    }
  }

  if ( p_match->arp_op.valid ) {
    if ( !set_arp_op( frame, p_match->arp_op.value ) ) {
      return false;
    }
  }

  if ( p_match->arp_spa.valid ) {
    if ( !set_arp_spa( frame, p_match->arp_spa.value ) ) {
      return false;
    }
  }

  if ( p_match->arp_tpa.valid ) {
    if ( !set_arp_tpa( frame, p_match->arp_tpa.value ) ) {
      return false;
    }
  }

  if ( p_match->arp_sha[ 0 ].valid ) {
    if ( !set_arp_sha( frame, &p_match->arp_sha[ 0 ] ) ) {
      return false;
    }
  }

  if ( p_match->arp_tha[ 0 ].valid ) {
    if ( !set_arp_tha( frame, &p_match->arp_tha[ 0 ] ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_src[ 0 ].valid ) {
    if ( !set_ipv6_src( frame, &p_match->ipv6_src[ 0 ] ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_dst[ 0 ].valid ) {
    if ( !set_ipv6_dst( frame, &p_match->ipv6_dst[ 0 ] ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_flabel.valid ) {
    if ( !set_ipv6_flabel( frame, p_match->ipv6_flabel.value ) ) {
      return false;
    }
  }

  if ( p_match->icmpv6_type.valid ) {
    if ( !set_icmpv6_type( frame, p_match->icmpv6_type.value ) ) {
      return false;
    }
  }

  if ( p_match->icmpv6_code.valid ) {
    if ( !set_icmpv6_code( frame, p_match->icmpv6_code.value ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_nd_target[ 0 ].valid ) {
    if ( !set_ipv6_nd_target( frame, p_match->ipv6_nd_target ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_nd_sll[ 0 ].valid ) {
    if ( !set_ipv6_nd_sll( frame, p_match->ipv6_nd_sll ) ) {
      return false;
    }
  }

  if ( p_match->ipv6_nd_tll[ 0 ].valid ) {
    if ( !set_ipv6_nd_tll( frame, p_match->ipv6_nd_tll ) ) {
      return false;
    }
  }

  if ( p_match->mpls_label.valid ) {
    if ( !set_mpls_label( frame, p_match->mpls_label.value ) ) {
      return false;
    }
  }

  if ( p_match->mpls_tc.valid ) {
    if ( !set_mpls_tc( frame, p_match->mpls_tc.value ) ) {
      return false;
    }
  }

  if ( p_match->mpls_bos.valid ) {
    if ( !set_mpls_bos( frame, p_match->mpls_bos.value ) ) {
      return false;
    }
  }

  return true;
}


static bool
execute_group_all( buffer *frame, bucket_list *buckets ) {
  assert( frame != NULL );
  assert( buckets != NULL );

  bucket_list *bucket_element = ( bucket_list * ) get_last_element( ( dlist_element * ) buckets );

  while ( bucket_element != NULL && bucket_element->node != NULL ) {
    bucket_element->node->packet_count++;
    bucket_element->node->byte_count += frame->length;
    action_list *action_element = ( action_list * ) get_last_element( ( dlist_element * ) bucket_element->node->actions );
    if ( execute_action_list( action_element, frame ) != OFDPE_SUCCESS ) {
      return false;
    }
    bucket_element = bucket_element->prev;
  }

  return true;
}


static bool
check_backet( action_list *acions ) {
  assert( acions != NULL );

  action_list *action_element = ( action_list * ) get_last_element( ( dlist_element * ) acions );
  while ( action_element != NULL && action_element->node != NULL ) {
    if ( action_element->node->type == OFPAT_OUTPUT ) {
      if ( !is_device_linkup( action_element->node->port ) ) {
        return false;
      }
    }
    action_element = action_element->prev;
  }

  return true;
}


static bool
execute_group_select( buffer *frame, bucket_list *buckets ) {
  assert( frame != NULL );
  assert( buckets != NULL );

  list_element *cantdidate;
  create_list( &cantdidate );

  bucket_list *bucket_element = ( bucket_list * ) get_last_element( ( dlist_element * ) buckets );

  while ( bucket_element != NULL && bucket_element->node != NULL ) {
    if ( !check_backet( bucket_element->node->actions ) ) {
      continue;
    }
    append_to_tail( &cantdidate, bucket_element );
    bucket_element = bucket_element->prev;
  }

  unsigned int length_of_cantdidate = list_length_of( cantdidate );
  if ( length_of_cantdidate == 0 ) {
    delete_list( cantdidate );
    return true;
  }

  struct timeval tv;
  gettimeofday( &tv, NULL );
  unsigned int cantdidate_index = (unsigned int)tv.tv_sec % length_of_cantdidate;
  debug( "execute group select. bucket=%d(/%d)", cantdidate_index, length_of_cantdidate );
  list_element *target = cantdidate;
  unsigned int i = 0;
  while ( target != NULL ) {
    if ( i == cantdidate_index ) {
      break;
    }
    target = target->next;
    i++;
  }

    ( ( bucket_list * ) target->data )->node->packet_count++;
    ( ( bucket_list * ) target->data )->node->byte_count += frame->length;

  if ( execute_action_list( ( ( bucket_list * ) ( target->data ) )->node->actions, frame ) != OFDPE_SUCCESS ) {
    delete_list( cantdidate );
    return false;
  }


  delete_list( cantdidate );
  return true;
}


static bool
execute_group_indirect( buffer *frame, action_list *actions ) {
  assert( frame != NULL );
  assert( actions != NULL );

  if ( execute_action_list( actions, frame ) != OFDPE_SUCCESS ) {
    return false;
  }
  return true;
}


static bool
execute_action_group( buffer *frame, action *group ) {
  assert( frame != NULL );
  assert( group != NULL );

  group_entry *entry = lookup_group_entry( group->group_id );
  if ( entry == NULL ) {
    return true;
  }

  entry->packet_count++;
  entry->byte_count += frame->length;

  bool ret = true;

  switch ( entry->type ) {
  case OFPGT_ALL:
    debug( "execute action group.(OFPGT_ALL)" );
    ret = execute_group_all( frame, entry->p_bucket );
    break;

  case OFPGT_SELECT:
    debug( "execute action group.(OFPGT_SELECT)" );
    ret = execute_group_select( frame, entry->p_bucket );
    break;

  case OFPGT_INDIRECT:
    debug( "execute action group.(OFPGT_INDIRECT)" );
    {
      bucket_list *bucket_element = ( bucket_list * ) get_last_element( ( dlist_element * ) entry->p_bucket );
      bucket_element->node->packet_count++;
      bucket_element->node->byte_count += frame->length;
      action_list *action_element = ( action_list * ) get_last_element( ( dlist_element * ) bucket_element->node->actions );
      ret = execute_group_indirect( frame, action_element );
    }
    break;

  case OFPGT_FF:
    debug( "execute action group.(OFPGT_FF)" );
    break;

  default:
    break;
  }


  return ret;
}


static bool
execute_action_output( buffer *frame, action *output ) {
  assert( frame != NULL );
  assert( output != NULL );

  bool ret = true;

  if ( output->port == OFPP_CONTROLLER ) {
    notify_parameter_packet_in parameter;
    parameter.cookie = output->entry->cookie;
    parameter.reason = OFPR_ACTION;
    parameter.max_len = output->max_len;
    parameter.packet = frame;
    parameter.table_id = output->entry->table_id;
    memcpy( &parameter.match, output->entry->p_match, sizeof( parameter.match ) );
    notify_packet_in( &parameter );
  }
  else {
    if ( send_data( output->port, frame ) != OFDPE_SUCCESS ) {
      ret = false;
    }
  }

  return ret;
}


/**
 * initialize action executor
 *
 * param nothing
 * @return success/failed
 */
OFDPE
init_action_executor( void ) {
  return OFDPE_SUCCESS;
}


/**
 * finalize action executor
 *
 * param nothing
 * @return success/failed
 */
OFDPE
finalize_action_executor( void ) {
  return OFDPE_SUCCESS;
}


/**
 * execute action list
 *
 * param action list
 * param target buffer
 * @return success/failed
 */
static OFDPE
_execute_action_list( action_list *p_action_list, buffer *frame ) {
  assert( p_action_list != NULL );
  assert( frame != NULL );

  action_list *action_element = ( action_list * ) get_last_element( ( dlist_element * ) p_action_list );

  while ( action_element != NULL && action_element->node != NULL ) {
    bool ret = true;
    action *action = action_element->node;

    switch ( action->type ) {
    case OFPAT_OUTPUT:
      debug( "execute action list.(OFPAT_OUTPUT): port = %u, maxlen = %u", action->port, action->max_len );
      ret = execute_action_output( frame, action );
      break;

    case OFPAT_COPY_TTL_OUT:
      debug( "execute action list.(OFPAT_COPY_TTL_OUT)" );
      ret = execute_action_copy_ttl_out( frame, action );
      break;

    case OFPAT_COPY_TTL_IN:
      debug( "execute action list.(OFPAT_COPY_TTL_IN)" );
      ret = execute_action_copy_ttl_in( frame, action );
      break;

    case OFPAT_SET_MPLS_TTL:
      debug( "execute action list.(OFPAT_SET_MPLS_TTL): ttl = %u", action->mpls_ttl );
      ret = execute_action_set_mpls_ttl( frame, action );
      break;

    case OFPAT_DEC_MPLS_TTL:
      debug( "execute action list.(OFPAT_DEC_MPLS_TTL)" );
      ret = execute_action_dec_mpls_ttl( frame, action );
      break;

    case OFPAT_PUSH_VLAN:
      debug( "execute action list.(OFPAT_PUSH_VLAN)" );
      ret = execute_action_push_vlan( frame, action );
      break;

    case OFPAT_POP_VLAN:
      debug( "execute action list.(OFPAT_POP_VLAN)" );
      ret = execute_action_pop_vlan( frame, action );
      break;

    case OFPAT_PUSH_MPLS:
      debug( "execute action list.(OFPAT_PUSH_MPLS)" );
      ret = execute_action_push_mpls( frame, action );
      break;

    case OFPAT_POP_MPLS:
      debug( "execute action list.(OFPAT_POP_MPLS)" );
      ret = execute_action_pop_mpls( frame, action );
      break;

    case OFPAT_SET_QUEUE:
      debug( "execute action list.(OFPAT_SET_QUEUE)" );
      warn( "Don't supported action type(=OFPAT_SET_QUEUE)" );
      return OFDPE_FAILED;

    case OFPAT_GROUP:
      debug( "execute action list.(OFPAT_GROUP)" );
      ret = execute_action_group( frame, action );
      break;

    case OFPAT_SET_NW_TTL:
      debug( "execute action list.(OFPAT_SET_NW_TTL): ttl = %u", action->nw_ttl );
      ret = execute_action_set_nw_ttl( frame, action );
      break;

    case OFPAT_DEC_NW_TTL:
      debug( "execute action list.(OFPAT_DEC_NW_TTL)" );
      ret = execute_action_dec_nw_ttl( frame, action );
      break;

    case OFPAT_SET_FIELD:
      debug( "execute action list.(OFPAT_SET_FIELD)" );
      ret = execute_action_set_field( frame, action );
      break;

    case OFPAT_PUSH_PBB:
      debug( "execute action list.(OFPAT_PUSH_PBB)" );
      warn( "Unsupported action type(=OFPAT_PUSH_PBB)" );
      return OFDPE_FAILED;

      break;

    case OFPAT_POP_PBB:
      debug( "execute action list.(OFPAT_POP_PBB)" );
      warn( "Unsupported action type(=OFPAT_POP_PBB)" );
      return OFDPE_FAILED;

      break;

    case OFPAT_EXPERIMENTER:
      debug( "execute action list.(OFPAT_EXPERIMENTER)" );
      warn( "Unsupported action type(=OFPAT_SET_QUEUE)" );
      return OFDPE_FAILED;

    default:
      warn( "Illegal action type(=%04x)", action->type );
      return OFDPE_FAILED;
    }

    if ( !ret ) {
      return OFDPE_FAILED;
    }

    action_element = action_element->prev;
  }

  return OFDPE_SUCCESS;
}
OFDPE ( *execute_action_list )( action_list *p_action_list, buffer *frame ) = _execute_action_list;


/**
 * execute action set
 *
 * param action list
 * param target buffer
 * @return success/failed
 */
static OFDPE
_execute_action_set( action_set *action_set, buffer *frame ) {
  assert( action_set != NULL );
  assert( frame != NULL );

  if ( action_set->copy_ttl_in != NULL ) {
    debug( "execute action set.(OFPAT_COPY_TTL_IN)" );
    if ( !execute_action_copy_ttl_in( frame, action_set->copy_ttl_in ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->pop_mpls != NULL ) {
    debug( "execute action set.(OFPAT_POP_MPLS)" );
    if ( !execute_action_pop_mpls( frame, action_set->pop_mpls ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->pop_pbb != NULL ) {
    debug( "execute action set.(OFPAT_POP_PBB)" );
    warn( "OFPAT_POP_PBB is not supported" );
    return OFDPE_FAILED;
  }

  if ( action_set->pop_vlan != NULL ) {
    debug( "execute action set.(OFPAT_POP_VLAN)" );
    if ( !execute_action_pop_vlan( frame, action_set->pop_vlan ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->push_mpls != NULL ) {
    debug( "execute action set.(OFPAT_PUSH_MPLS)" );
    if ( !execute_action_push_mpls( frame, action_set->push_mpls ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->push_pbb != NULL ) {
    debug( "execute action set.(OFPAT_PUSH_PBB)" );
    warn( "OFPAT_PUSH_PBB is not supported" );
    return OFDPE_FAILED;
  }

  if ( action_set->push_vlan != NULL ) {
    debug( "execute action set.(OFPAT_PUSH_VLAN)" );
    if ( !execute_action_push_vlan( frame, action_set->push_vlan ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->copy_ttl_out != NULL ) {
    debug( "execute action set.(OFPAT_COPY_TTL_OUT)" );
    if ( !execute_action_copy_ttl_out( frame, action_set->copy_ttl_out ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->dec_mpls_ttl != NULL ) {
    debug( "execute action set.(OFPAT_DEC_MPLS_TTL)" );
    if ( !execute_action_dec_mpls_ttl( frame, action_set->dec_mpls_ttl ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->dec_nw_ttl != NULL ) {
    debug( "execute action set.(OFPAT_DEC_NW_TTL)" );
    if ( !execute_action_dec_nw_ttl( frame, action_set->dec_nw_ttl ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->set_mpls_ttl != NULL ) {
    debug( "execute action set.(OFPAT_SET_MPLS_TTL)" );
    if ( !execute_action_set_mpls_ttl( frame, action_set->set_mpls_ttl ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->set_nw_ttl != NULL ) {
    debug( "execute action set.(OFPAT_SET_NW_TTL)" );
    if ( !execute_action_set_nw_ttl( frame, action_set->set_nw_ttl ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->set_field != NULL ) {
    debug( "execute action set.(OFPAT_SET_FIELD)" );
    if ( !execute_action_set_field( frame, action_set->set_field ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( action_set->set_queue != NULL ) {
    debug( "execute action set.(OFPAT_SET_QUEUE)" );
    warn( "OFPAT_SET_QUEUE is not supported" );
    return OFDPE_FAILED;
  }

  if ( action_set->group != NULL ) {
    debug( "execute action set.(OFPAT_GROUP)" );
    if ( !execute_action_group( frame, action_set->group ) ) {
      return OFDPE_FAILED;
    }
  }

  if ( ( action_set->group == NULL )
     && ( action_set->output != NULL ) ) {
    debug( "execute action set.(OFPAT_OUTPUT)" );
    if ( !execute_action_output( frame, action_set->output ) ) {
      return OFDPE_FAILED;
    }
  }

  return OFDPE_SUCCESS;
}
OFDPE ( *execute_action_set )( action_set *action_set, buffer *frame ) = _execute_action_set;


/**
 * execute action packet out
 *
 * param packet_in buffer id
 * param recveive switch port number
 * param action list
 * param target buffer
 * return success/failed
 */
static OFDPE
_execute_action_packet_out( uint32_t buffer_id, uint32_t in_port, action_list *action_list, buffer *frame ) {
  assert( action_list != NULL );

  buffer *target = NULL;

  if ( buffer_id != OFP_NO_BUFFER ) {
    if ( get_packet_buffer( buffer_id, &target ) != OFDPE_SUCCESS ) {
      return OFDPE_FAILED;
    }
  }
  else {
    target = duplicate_packet_buffer_pool_entry( frame );
    if ( target == NULL ) {
      return OFDPE_FAILED;
    }
  }

  if ( !parse_packet( target ) ) {
    free_packet_buffer_pool_entry( target );
    return OFDPE_FAILED;
  }

    ( ( packet_info * ) target->user_data )->eth_in_port = in_port;

  if ( !lock_pipeline() ) {
    free_packet_buffer_pool_entry( target );
    return OFDPE_FAILED;
  }

  OFDPE ret = execute_action_list( action_list, target );
  unlock_pipeline();

  if ( target->user_data != NULL ) {
    free_packet_info( target );
  }

  free_packet_buffer_pool_entry( target );

  return ret;
}
OFDPE ( *execute_action_packet_out )( uint32_t buffer_id, uint32_t in_port, action_list *action_list, buffer *frame ) = _execute_action_packet_out;


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
*/
