/*
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
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


#include "trema.h"
#include "ruby.h"


extern VALUE mTrema;
VALUE cPacketInfo;


static VALUE
packet_info_init( VALUE self, VALUE packet_in ) {
  VALUE packet_info = rb_funcall( packet_in, rb_intern( "packet_info" ), 1, rb_eval_string( "self.class") );
  rb_iv_set( self, "@info", packet_info );
  return self;
}


packet_info *
get_pinfo( VALUE self ) {
  packet_info *_packet_info;
  Data_Get_Struct( rb_iv_get( self, "@info" ), packet_info, _packet_info );
  return _packet_info;
}


static VALUE 
packet_info_udp_src_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->udp_src_port );
}


static VALUE
packet_info_udp_dst_port( VALUE self ) {
  return UINT2NUM( get_pinfo( self )->udp_dst_port );
}


void
Init_packet_info() {
  VALUE module_defined = rb_eval_string( "Object.constants.include?( \"mTrema\")" );
  if ( module_defined == Qfalse ) {
    mTrema = rb_define_module( "Trema" );
  }
  cPacketInfo = rb_define_class_under( mTrema,  "PacketInfo", rb_cObject );
  rb_define_method( cPacketInfo, "initialize", packet_info_init, 1 );
  rb_define_method( cPacketInfo, "udp_src_port", packet_info_udp_src_port, 0 );
  rb_define_method( cPacketInfo, "udp_dst_port", packet_info_udp_dst_port, 0 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
