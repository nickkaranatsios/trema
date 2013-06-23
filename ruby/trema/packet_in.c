/*
 * Author: Yasuhito Takamiya <yasuhito@gmail.com>
 *
 * Copyright (C) 2008-2011 NEC Corporation
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


#include <string.h>
#include "buffer.h"
#include "ruby.h"
#include "trema.h"


extern VALUE mTrema;
VALUE cPacketIn;
static VALUE cPacketInfo;
static VALUE rb_packet_info;


#if 0
/*
 * @overload initialize()
 *   Allocates and wraps a {PacketIn} object to store the details of 
 *   the +OFPT_PACKET_IN+ message.
 *
 * @return [PacketIn] an object that encapsulates the +OPFT_PACKET_IN+ OpenFlow message. 
 */
static VALUE
packet_in_init( VALUE kclass ) {
  packet_in *_packet_in = xmalloc( sizeof( packet_in ) );
  return Data_Wrap_Struct( klass, 0, xfree, _packet_in );
}
#endif


#if 0
/*
 * @overload initialize()
 *   Allocates and wraps a {PacketInfo} object to store details about the user's
 *   data  packet included in the +OFPT_PACKET_IN+ message.
 *
 * @param [PacketIn] message
 *   the received packet in message.
 *
 * @return [PacketInfo] an object that encapsulates the user data packet.
 */ 
static VALUE
packet_info_init( VALUE kclass, VALUE message ) {
  packet_info _packet_info = get_packet_info( message.data );
  return Data_Wrap_Struct( cPacketInfo, 0, NULL, &_packet_info );
}
#endif


static VALUE
packet_in_alloc( VALUE klass ) {
  packet_in *_packet_in = xmalloc( sizeof( packet_in ) );
  return Data_Wrap_Struct( klass, 0, xfree, _packet_in );
}


static packet_in *
get_packet_in( VALUE self ) {
  packet_in *cpacket;
  Data_Get_Struct( self, packet_in, cpacket );
  return cpacket;
}


static packet_info *
get_pinfo( VALUE self ) {
  UNUSED( self );
  packet_info *_packet_info;
  Data_Get_Struct( rb_packet_info, packet_info, _packet_info );
  return _packet_info;
}


/*
 * Message originator identifier.
 *
 * @return [Number] the value of datapath_id.
 */
static VALUE
packet_in_datapath_id( VALUE self ) {
  return ULL2NUM( get_packet_in( self )->datapath_id );
}


/*
 * For this asynchronous message the transaction_id is set to zero.
 *
<<<<<<< HEAD
 * @return [Number] the value of transaction_id.
=======
 * @return [Number] the value of attribute transaction_id.
>>>>>>> 565363f0ea4d4edbfba7cfb3ac6d3bb37a0cf280
 */
static VALUE
packet_in_transaction_id( VALUE self ) {
  return ULONG2NUM( get_packet_in( self )->transaction_id );
}


/*
 * Buffer id value signifies if the entire frame (packet is not buffered) or 
 * portion of it (packet is buffered) is included in the data field of 
 * this +OFPT_PACKET_IN+ message.
 *
 * @return [Number] the value of buffer id.
 */
static VALUE
packet_in_buffer_id( VALUE self ) {
  return ULONG2NUM( get_packet_in( self )->buffer_id );
}


/*
 * A buffer_id value either than +UINT32_MAX+ marks the packet_in as buffered.
 *
 * @return [Boolean] true if packet_in is buffered otherwise false.
 */
static VALUE
packet_in_is_buffered( VALUE self ) {
  if ( get_packet_in( self )->buffer_id == UINT32_MAX ) {
    return Qfalse;
  }
  else {
    return Qtrue;
  }
}


/*
 * The port number from where the packet_in received.
 *
 * @return [Number] the value of in_port.
 */
static VALUE
packet_in_in_port( VALUE self ) {
  return INT2NUM( get_packet_in( self )->in_port );
}


/*
 * The full length of the received packet_in.
 *
 * @return [Number] the value of total_len.
 */
static VALUE
packet_in_total_len( VALUE self ) {
  return INT2NUM( get_packet_in( self )->total_len );
}


/*
 * A String that holds the entire or portion of the received frame.
 * Length of data, total_len - 20 bytes.
 *
 * @return [String] the value of data.
 */
static VALUE
packet_in_data( VALUE self ) {
  const buffer *buf = get_packet_in( self )->data;
  return rb_str_new( buf->data, ( long ) buf->length );
}


/*
 * The reason why the +OFPT_PACKET_IN+ message was sent.
 *
 * @return [Number] the value of reason.
 */
static VALUE
packet_in_reason( VALUE self ) {
  return INT2NUM( get_packet_in( self )->reason );
}


/*
 * The MAC source address.
 *
 * @return [Trema::Mac]
 */
static VALUE
packet_info_macsa( VALUE self ) {
  VALUE macsa = ULL2NUM( mac_to_uint64( ( get_pinfo( self )->eth_macsa ) ) );
  return rb_funcall( rb_eval_string( "Trema::Mac" ), rb_intern( "new" ), 1, macsa );
}


/*
 * The MAC destination address.
 *
 * @return [Trema::Mac]
 */
static VALUE
packet_info_macda( VALUE self ) {
  VALUE macda = ULL2NUM( mac_to_uint64( ( get_pinfo( self )->eth_macda ) ) );
  return rb_funcall( rb_eval_string( "Trema::Mac" ), rb_intern( "new" ), 1, macda );
}


/*
 * The UDP source port number of the user's data packet.
 *
 * @return [Number]
 */
static VALUE
packet_info_udp_src_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->udp_src_port );
}


/*
 * The UDP destination port number of the user's data packet.
 *
 * @return [Number]
 */
static VALUE
packet_info_udp_dst_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->udp_dst_port );
}


/*
 * The TCP source port number of the user's data packet.
 *
 * @return [Number]
 */
static VALUE
packet_info_tcp_src_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->tcp_src_port );
}


/*
 * The TCP destination port number of the user's data packet.
 *
 * @return [Number]
 */
static VALUE
packet_info_tcp_dst_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->tcp_dst_port );
}


/*
 * true if the user data packet is an ARP packet otherwise false.
 *
 * @return [Boolean]
 */
static VALUE
packet_info_arp( VALUE self ) {
  return ( ( get_pinfo( self )->format & NW_ARP ) == NW_ARP ); 
}


void
Init_packet_in() {
  rb_require( "trema/mac" );
  cPacketIn = rb_define_class_under( mTrema, "PacketIn", rb_cObject );
  cPacketInfo = rb_define_class_under( mTrema, "PacketInfo", rb_cObject );
  rb_define_alloc_func( cPacketIn, packet_in_alloc );
#if 0
  /*
   * Don't remove this because it is to fake yard to create a constructor for 
   * PacketIn object.
   */
  rb_define_method( cPacketIn, "initialize", packet_in_init, 0 );
  /*
   * Don't remove this because it is to fake yard to create a constructor for
   * PacketInfo object.
   */
   rb_define_method( cPacketInfo, "initialize", packet_info_init, 0 );
  */
#endif

  rb_define_method( cPacketIn, "datapath_id", packet_in_datapath_id, 0 );
  rb_define_method( cPacketIn, "transaction_id", packet_in_transaction_id, 0 );  
  rb_define_method( cPacketIn, "buffer_id", packet_in_buffer_id, 0 );
  rb_define_method( cPacketIn, "buffered?", packet_in_is_buffered, 0 );
  rb_define_method( cPacketIn, "in_port", packet_in_in_port, 0 );
  rb_define_method( cPacketIn, "total_len", packet_in_total_len, 0 );
  rb_define_method( cPacketIn, "reason", packet_in_reason, 0 );
  rb_define_method( cPacketIn, "data", packet_in_data, 0 );

  rb_define_method( cPacketIn, "macsa", packet_info_macsa, 0 );
  rb_define_method( cPacketIn, "macda", packet_info_macda, 0 );
  rb_define_method( cPacketIn, "udp_src_port", packet_info_udp_src_port, 0 ); 
  rb_define_method( cPacketIn, "udp_dst_port", packet_info_udp_dst_port, 0 ); 
  rb_define_method( cPacketIn, "tcp_src_port", packet_info_tcp_src_port, 0 ); 
  rb_define_method( cPacketIn, "tcp_dst_port", packet_info_tcp_dst_port, 0 ); 
  rb_define_method( cPacketIn, "arp?", packet_info_arp, 0 );
}


/*
 * Handler called when +OFPT_PACKET_IN+ message is received.
 */
void
handle_packet_in( uint64_t datapath_id, packet_in message ) {
  VALUE controller = ( VALUE ) message.user_data;
  if ( rb_respond_to( controller, rb_intern( "packet_in" ) ) == Qfalse ) {
    return;
  }

  VALUE r_message = rb_funcall( cPacketIn, rb_intern( "new" ), 0 );
  packet_in *tmp = NULL;
  Data_Get_Struct( r_message, packet_in, tmp );
  memcpy( tmp, &message, sizeof( packet_in ) );

  packet_info _packet_info = get_packet_info( message.data );
  rb_packet_info = Data_Wrap_Struct( cPacketInfo, 0, NULL, &_packet_info );
  rb_funcall( controller, rb_intern( "packet_in" ), 2, ULL2NUM( datapath_id ), r_message );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
