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
VALUE cFlowRemoved;


/*
 * When a flow is deleted or expired a +OFPT_FLOW_REMOVED+ message is sent as long
 * as the +OFPFF_SEND_FLOW_REM+ bit is toggled in the +flags+ bitmap during
 * flow setup. A user would not explicitly instantiate a {FlowRemoved} object but
 * would be created while parsing the +OPPT_FLOW_REMOVED+ message.
 * Returns an object that encapsulates the +OPPT_FLOW_REMOVED+ openflow message.
 *
 * @overload initialize(options={})
 *   @example 
 *     FlowRemoved.new( 
 *       :datapath_id => 0xabc,
 *       :transaction_id => 0,
 *       :match => Match,
 *       :cookie => 123456789,
 *       :priority => 65535,
 *       :reason => 0,
 *       :duration_sec => 1,
 *       :duration_nsec => 783000000,
 *       :idle_timeout => 1,
 *       :packet_count => 1
 *       :byte_count=> 64
 *     )
 *
 *   @param [Hash] options the options hash.
 *
 *   @option options [Symbol] :datapath_id
 *     message originator identifier.
 *
 *   @option options [Symbol] :transaction_id
 *     unsolicited message transaction_id is zero.
 *
 *   @option options [Symbol] :match
 *     a {Match} object describing the flow fields copied from the corresponding 
 *     flow setup message.
 *
 *   @option options [Symbol] :cookie
 *     an opaque handle copied from the corresponding 
 *     flow setup message.
 *
 *   @option options [Symbol] :priority
 *     the priority level of the flow copied from the corresponding 
 *     flow setup message.
 *
 *   @option options [Symbol] :reason
 *     the reason why the flow is removed.
 *
 *   @option options [Symbol] :duration_sec
 *     the number of seconds the flow was active.
 *
 *   @option options [Symbol] :duration_nsec
 *     the number of nanoseconds the flow was active.
 *
 *   @option options [Symbol] :idle_timeout
 *     time elapsed in seconds before the flow is removed, copied from the 
 *     corresponding flow setup message.
 *
 *   @option options [Symbol] :packet_count
 *     a counter of the total number of packets.
 *
 *   @option options [Symbol] :byte_count
 *     a counter of the total number of bytes.
 *
 * @return [FlowRemoved] self
 */
static VALUE
flow_removed_init( VALUE self, VALUE options ) {
  rb_iv_set( self, "@attribute", options );
  return self;
}


/*
 * Message originator identifier.
 *
 * @return [Number] the value of attribute datapath_id.
 */
static VALUE
flow_removed_datapath_id( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "datapath_id" ) ) );
}


/*
 * For this asynchronous message the transaction_id is set to zero.
 *
 * @return [Number] the value of attribute transaction_id.
 */
static VALUE
flow_removed_transaction_id( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "transaction_id" ) ) );
}


/*
 * Flow fields matched.
 *
 * @return [Match] an object that encapsulates flow fields details.
 */
static VALUE
flow_removed_match( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "match" ) ) );
}


/*
 * An opaque handle copied from the corresponding flow setup message.
 *
 * @return [Number] the value of attribute cookie.
 */
static VALUE
flow_removed_cookie( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "cookie" ) ) );
}


/*
 * The priority level of the flow copied from the corresponding flow setup
 * message.
 *
 * @return [Number] the value of attribute priority.
 */
static VALUE
flow_removed_priority( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "priority" ) ) );
}


/*
 * The reason why the flow is removed.
 *
 * @return [Number] the value of attribute reason.
 */
static VALUE
flow_removed_reason( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "reason" ) ) );
}


/*
 * The number of seconds the flow was active.
 *
 * @return [Number] the value of attribute duration_sec.
 */
static VALUE
flow_removed_duration_sec( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "duration_sec" ) ) );
}


/*
 * The number of nanoseconds the flow was active.
 *
 * @return [Number] the value of attribute duration_nsec.
 */
static VALUE
flow_removed_duration_nsec( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "duration_nsec" ) ) );
}


/*
 * Time elapsed in seconds before the flow is removed.
 *
 * @return [Number] the value of attribute idle_timeout.
 */
static VALUE
flow_removed_idle_timeout( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "idle_timeout" ) ) );
}


/*
 * A counter of the total number of packets.
 *
 * @return [Number] the value of attribute packet_count.
 */
static VALUE
flow_removed_packet_count( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "packet_count" ) ) );
}


/*
 * A counter of the total number of bytes.
 *
 * @return [Number] the value of attribute byte_count.
 */
static VALUE
flow_removed_byte_count( VALUE self ) {
  return rb_hash_aref( rb_iv_get( self, "@attribute" ), ID2SYM( rb_intern( "byte_count" ) ) );
}


void
Init_flow_removed() {
  cFlowRemoved = rb_define_class_under( mTrema, "FlowRemoved", rb_cObject );
  rb_define_method( cFlowRemoved, "initialize", flow_removed_init, 1 );
  rb_define_method( cFlowRemoved, "datapath_id", flow_removed_datapath_id, 0 );
  rb_define_method( cFlowRemoved, "transaction_id", flow_removed_transaction_id, 0 );
  rb_define_method( cFlowRemoved, "match", flow_removed_match, 0 );
  rb_define_method( cFlowRemoved, "cookie", flow_removed_cookie, 0 );
  rb_define_method( cFlowRemoved, "priority", flow_removed_priority, 0 );
  rb_define_method( cFlowRemoved, "reason", flow_removed_reason, 0 );
  rb_define_method( cFlowRemoved, "duration_sec", flow_removed_duration_sec, 0 );
  rb_define_method( cFlowRemoved, "duration_nsec", flow_removed_duration_nsec, 0 );
  rb_define_method( cFlowRemoved, "idle_timeout", flow_removed_idle_timeout, 0 );
  rb_define_method( cFlowRemoved, "packet_count", flow_removed_packet_count, 0 );
  rb_define_method( cFlowRemoved, "byte_count", flow_removed_byte_count, 0 );
}


void
handle_flow_removed(
  uint64_t datapath_id,
  uint32_t transaction_id,
  struct ofp_match match,
  uint64_t cookie,
  uint16_t priority,
  uint8_t reason,
  uint32_t duration_sec,
  uint32_t duration_nsec,
  uint16_t idle_timeout,
  uint64_t packet_count,
  uint64_t byte_count,
  void *user_data
) {
  VALUE controller = ( VALUE ) user_data;
  if ( rb_respond_to( controller, rb_intern( "flow_removed" ) ) == Qfalse ) {
    return;
  }
  VALUE attributes = rb_hash_new();

  rb_hash_aset( attributes, ID2SYM( rb_intern( "datapath_id" ) ), ULL2NUM( datapath_id ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "transaction_id" ) ), UINT2NUM( transaction_id ) );

  VALUE match_obj = rb_eval_string( "Match.new" );
  rb_funcall( match_obj, rb_intern( "replace" ), 1, Data_Wrap_Struct( cFlowRemoved, NULL, NULL, &match ) );

  rb_hash_aset( attributes, ID2SYM( rb_intern( "match" ) ), match_obj );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "cookie" ) ), ULL2NUM( cookie ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "priority" ) ), UINT2NUM( priority ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "reason" ) ), UINT2NUM( reason ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "duration_sec" ) ), UINT2NUM( duration_sec ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "duration_nsec" ) ), UINT2NUM( duration_nsec ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "idle_timeout" ) ), UINT2NUM( idle_timeout ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "packet_count" ) ), ULL2NUM( packet_count ) );
  rb_hash_aset( attributes, ID2SYM( rb_intern( "byte_count" ) ), ULL2NUM( byte_count ) );

  VALUE flow_removed = rb_funcall( cFlowRemoved, rb_intern( "new" ), 1, attributes );
  rb_funcall( controller, rb_intern( "flow_removed" ), 1, flow_removed );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
