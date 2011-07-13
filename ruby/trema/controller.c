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


#include "buffer.h"
#include "controller.h"
#include "features_reply.h"
#include "logger.h"
#include "openflow.h"
#include "packet_in.h"
#include "flow_removed.h"
#include "switch_disconnected.h"
#include "port_status.h"
#include "stats_reply.h"
#include "trema.h"


VALUE mTrema;
VALUE cController;


static void
handle_timer_event( void *self ) {
  rb_funcall( ( VALUE ) self, rb_intern( "handle_timer_event" ), 0 );
}


static VALUE
controller_send_message( VALUE self, VALUE message, VALUE datapath_id ) {
  buffer *buf;
  Data_Get_Struct( message, buffer, buf );
  send_openflow_message( NUM2ULL( datapath_id ), buf );
  return self;
}

static void
form_actions( VALUE raction, openflow_actions *actions ) {
  VALUE *data_ptr;
  int len;
  int i;

  if ( raction != Qnil ) {
    switch ( TYPE( raction ) ) {
      case T_ARRAY:
        data_ptr = RARRAY_PTR( raction );
        len = RARRAY_LEN( raction );

        for ( i = 0; i < len; i++ ) {
          VALUE value = data_ptr[i];
          rb_funcall( value, rb_intern( "append" ), 1, Data_Wrap_Struct( cController, NULL, NULL, actions ) );
        }
        break;
      case T_OBJECT:
        rb_funcall( raction, rb_intern( "append" ), 1, Data_Wrap_Struct( cController, NULL, NULL, actions ) );
        break;
      default:
        rb_raise( rb_eTypeError, "actions argument must be an Array or an Action object" );
    }
  }
}

static VALUE
get_strict( int argc, VALUE *argv ) {
  VALUE datapath_id = Qnil;
  VALUE options = Qnil;
  VALUE strict = Qnil;

  rb_scan_args( argc, argv, "11", &datapath_id, &options );
  if ( options != Qnil ) {
    strict = rb_hash_aref( options, ID2SYM( rb_intern( "strict" ) ) );
  }
  return strict;
}

static VALUE
controller_send_flow_mod( uint16_t command, int argc, VALUE *argv, VALUE self ) {
  VALUE datapath_id = Qnil;
  VALUE options = Qnil;

  rb_scan_args( argc, argv, "11", &datapath_id, &options );

  struct ofp_match *match = malloc( sizeof( struct ofp_match ) );
  uint32_t buffer_id;
  openflow_actions *actions = create_actions();

  VALUE rmatch = Qnil;
  VALUE rbuffer_id = Qnil;
  VALUE raction = Qnil;

  if ( options != Qnil ) {
    rmatch = rb_hash_aref( options, ID2SYM( rb_intern( "match" ) ) );
    rbuffer_id = rb_hash_aref( options, ID2SYM( rb_intern( "buffer_id" ) ) );
    raction = rb_hash_aref( options, ID2SYM( rb_intern( "actions" ) ) );
  }

  if ( rmatch != Qnil ) {
    Data_Get_Struct( rmatch, struct ofp_match, match );
  }
  else {
    memset( match, 0, sizeof( struct ofp_match ) );
    match->wildcards = OFPFW_ALL;
  }

  if ( rbuffer_id == Qnil ) {
    buffer_id = UINT32_MAX;
  }
  else {
    buffer_id = NUM2ULONG( rbuffer_id );
  }

  if ( raction != Qnil ) {
    form_actions( raction, actions );
  }

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    *match,
    get_cookie(),
    command,
    60,
    0,
    UINT16_MAX,
    buffer_id,
    OFPP_NONE,
    0,
    actions
  );
  send_openflow_message( NUM2ULL( datapath_id ), flow_mod );

  free_buffer( flow_mod );
  delete_actions( actions );

  return self;
}

/*
 * call-seq:
 *   send_flow_mod_add(datapath_id, options={})
 *
 * Sends a flow_mod message to add a flow into the datapath.
 *
 *  # packet_in handler
 *  def packet_in message
 *    send_flow_mod_add(
 *      datapath_id,
 *      :match => Match.from(message),
 *      :buffer_id => message.buffer_id,
 *      :actions => ActionOutput.new( OFPP_FLOOD )
 *    )
 *  end
 *
 * Options:
 *
 * <code>match</code>::
 *   A {Match} object describing the fields of the
 *   flow. <em>(default=all fields are wildcarded)</em>
 *
 * <code>buffer_id</code>::
 *   The buffer ID assigned by the datapath of a buffered packet to
 *   apply the flow to. If 0xffffffff, no buffered packet is to be
 *   applied the flow actions. <em>(default=0xffffffff)</em>
 * 
 * <code>actions</code>::
 *   The sequence of actions specifying the actions to perform on the
 *   flow's packets. <em>(default=[])</em>
 */
static VALUE
controller_send_flow_mod_add( int argc, VALUE *argv, VALUE self ) {
	uint16_t command = OFPFC_ADD;

	return controller_send_flow_mod( command, argc, argv, self );
}

/*
 * call-seq :
 * send_flow_mod_modify(datapath, options={})
 * 
 * Sends a flow_mod message to either modify or modify strict a flow from the datapath.
 * The flow_mod modify/modify_strict command would be used to modify flow actions.
 *
 * Options:
 *
 * <code>strict</code>::
 * The strict option if set to true modify_strict command is invoked.
 * If not set or set to false modify command is invoked.
 */
static VALUE
controller_send_flow_mod_modify( int argc, VALUE *argv, VALUE self ) {
  uint16_t command = OFPFC_MODIFY;

  if ( get_strict( argc, argv ) == Qtrue ) {
    command = OFPFC_MODIFY_STRICT;
  }
  return controller_send_flow_mod( command, argc, argv, self );
}

static VALUE
controller_send_flow_mod_delete( int argc, VALUE *argv, VALUE self ) {
  uint16_t command = OFPFC_DELETE;

  if ( get_strict( argc, argv ) == Qtrue ) {
    command = OFPFC_DELETE_STRICT;
  }
  return controller_send_flow_mod( command, argc, argv, self );
}

static VALUE
controller_send_packet_out( int argc, VALUE *argv, VALUE self ) {
  VALUE first_arg = Qnil;
  VALUE second_arg = Qnil;

  rb_scan_args( argc, argv, "11", &first_arg, &second_arg );

  if ( rb_funcall( first_arg, rb_intern( "is_a?" ), 1, cPacketIn ) ) {
    packet_in *cpacket_in;
    Data_Get_Struct( first_arg, packet_in, cpacket_in );

    VALUE raction = second_arg;
    openflow_actions *actions = create_actions();
    if ( raction != Qnil ) {
      form_actions( raction, actions );
    }

    buffer *packet_out = create_packet_out(
      get_transaction_id(),
      cpacket_in->buffer_id,
      cpacket_in->in_port,
      actions,
      cpacket_in->buffer_id == UINT32_MAX ? cpacket_in->data : NULL
    );

    send_openflow_message( cpacket_in->datapath_id, packet_out );
    free_buffer( packet_out );

    delete_actions( actions );
  }
  else {
    uint32_t buffer_id;

    VALUE datapath_id = first_arg;
    VALUE options = second_arg;

    VALUE rbuffer_id = Qnil;
    VALUE rin_port = Qnil;
    VALUE raction = Qnil;
    VALUE rdata = Qnil;

    if ( options != Qnil ) {
      rbuffer_id = rb_hash_aref( options, ID2SYM( rb_intern( "buffer_id" ) ) );
      rin_port = rb_hash_aref( options, ID2SYM( rb_intern( "in_port" ) ) );
      raction = rb_hash_aref( options, ID2SYM( rb_intern( "actions" ) ) );
      rdata = rb_hash_aref( options, ID2SYM( rb_intern( "data" ) ) );
    }

    if ( rbuffer_id == Qnil ) {
      buffer_id = UINT32_MAX;
    }
    else {
      buffer_id = NUM2ULONG( rbuffer_id );
    }

    openflow_actions *actions = create_actions();
    if ( raction != Qnil ) {
      form_actions( raction, actions );
    }

    buffer *packet_out;
    if ( rdata == Qnil ) {
      packet_out = create_packet_out(
        get_transaction_id(),
        buffer_id,
        NUM2INT( rin_port ),
        actions,
        NULL
      );
    }
    else {
      buffer *cbuffer;
      Data_Get_Struct( rdata, buffer, cbuffer );

      packet_out = create_packet_out(
        get_transaction_id(),
        buffer_id,
        NUM2INT( rin_port ),
        actions,
        cbuffer
      );
    }

    send_openflow_message( NUM2ULL( datapath_id ), packet_out );
    free_buffer( packet_out );

    delete_actions( actions );
  }
  return self;
}


/*
 * call-seq:
 *   run()  => self
 *
 * Starts this controller. Usually you do not need to invoke
 * explicitly, because this is called implicitly by "trema run"
 * command.
 */
static VALUE
controller_run( VALUE self ) {
  setenv( "TREMA_HOME", STR2CSTR( rb_funcall( mTrema, rb_intern( "home" ), 0 ) ), 1 );

  VALUE name = rb_funcall( self, rb_intern( "name" ), 0 );
  rb_gv_set( "$PROGRAM_NAME", name );

  int argc = 3;
  char **argv = malloc( sizeof( char * ) * ( argc + 1 ) );
  argv[ 0 ] = STR2CSTR( name );
  argv[ 1 ] = "--name";
  argv[ 2 ] = STR2CSTR( name );
  argv[ 3 ] = NULL;
  init_trema( &argc, &argv );

  set_switch_ready_handler( handle_switch_ready, ( void * ) self );
  set_features_reply_handler( handle_features_reply, ( void * ) self );
  set_packet_in_handler( handle_packet_in, ( void * ) self );
  set_flow_removed_handler( handle_flow_removed, ( void * ) self );
  set_switch_disconnected_handler( handle_switch_disconnected, ( void * ) self );
  set_port_status_handler( handle_port_status, ( void * ) self );
  set_stats_reply_handler( handle_stats_reply, ( void * ) self );

  struct itimerspec interval;
  interval.it_interval.tv_sec = 1;
  interval.it_interval.tv_nsec = 0;
  interval.it_value.tv_sec = 0;
  interval.it_value.tv_nsec = 0;
  add_timer_event_callback( &interval, handle_timer_event, ( void * ) self );

  rb_funcall( self, rb_intern( "start" ), 0 );

  rb_funcall( self, rb_intern( "start_trema" ), 0 );

  return self;
}


static VALUE
controller_shutdown( VALUE self ) {
  stop_trema();
  return self;
}


static void
thread_pass( void *user_data ) {
	UNUSED( user_data );
  rb_funcall( rb_cThread, rb_intern( "pass" ), 0 );
}

static VALUE
controller_start_trema( VALUE self ) {
  struct itimerspec interval;
  interval.it_interval.tv_sec = 0;
  interval.it_interval.tv_nsec = 1000000;
  interval.it_value.tv_sec = 0;
  interval.it_value.tv_nsec = 0;
  add_timer_event_callback( &interval, thread_pass, NULL );

  start_trema();

  return self;
}


/********************************************************************************
 * Handlers.
 ********************************************************************************/

// Override me if necessary.
static VALUE
controller_start( VALUE self ) {
  return self;
}


// Override me if necessary.
static VALUE
controller_switch_ready( VALUE self, VALUE datapath_id ) {
	UNUSED( datapath_id );
  return self;
}


// Override me if necessary.
static VALUE
controller_features_reply( VALUE self, VALUE message ) {
	UNUSED( message );
  return self;
}


/*
 * call-seq:
 *   packet_in(message)
 *
 * Handle the reception of a {PacketIn} message.
 */
static VALUE
controller_packet_in( VALUE self, VALUE packet_in ) {
	UNUSED( packet_in );
  return self;
}

// Override me if necessary.
static VALUE
controller_flow_removed( VALUE self, VALUE message ) {
	UNUSED( message );
  return self;
}

// Override me if necessary.
static VALUE
controller_switch_disconnected( VALUE self, VALUE datapath_id ) {
	UNUSED( datapath_id );
  return self;
}

// Override me if necessary.
static VALUE
controller_port_status( VALUE self, VALUE port_status ) {
	UNUSED( port_status );
  return self;
}

// Override me if necessary.
static VALUE
controller_stats_reply( VALUE self, VALUE stats_reply ) {
	UNUSED( stats_reply );
  return self;
}
/********************************************************************************
 * Init Controller module.
 ********************************************************************************/

void
Init_controller() {
  mTrema = rb_define_module( "Trema" );

  rb_require( "trema/controller" );

  cController = rb_eval_string( "Trema::Controller" );
  rb_include_module( cController, mLogger );
  rb_define_const( cController, "OFPP_FLOOD", INT2NUM( OFPP_FLOOD ) );

  rb_define_method( cController, "send_message", controller_send_message, 2 );
  rb_define_method( cController, "send_flow_mod_add", controller_send_flow_mod_add, -1 );
  rb_define_method( cController, "send_flow_mod_modify", controller_send_flow_mod_modify, -1 );
  rb_define_method( cController, "send_flow_mod_delete", controller_send_flow_mod_delete, -1 );
  rb_define_method( cController, "send_packet_out", controller_send_packet_out, -1 );

  rb_define_method( cController, "run!", controller_run, 0 );
  rb_define_method( cController, "shutdown!", controller_shutdown, 0 );

  // Handlers
  rb_define_method( cController, "start", controller_start, 0 );
  rb_define_method( cController, "switch_ready", controller_switch_ready, 1 );
  rb_define_method( cController, "features_reply", controller_features_reply, 1 );
  rb_define_method( cController, "packet_in", controller_packet_in, 1 );
  rb_define_method( cController, "flow_removed", controller_flow_removed, 1 );
  rb_define_method( cController, "switch_disconnected", controller_switch_disconnected, 1 );
  rb_define_method( cController, "port_status", controller_port_status, 1 );
  rb_define_method( cController, "stats_reply", controller_stats_reply, 1 );

  // Private
  rb_define_private_method( cController, "start_trema", controller_start_trema, 0 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

