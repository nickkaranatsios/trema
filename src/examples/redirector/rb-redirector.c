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


#include "redirector.h"
#include "ruby.h"
#include "trema.h"


VALUE cRedirector;
static uint8_t initialized = 0;


/*
 * Routes non-authorized packets via a tun device to a destination host.
 * If initializer called multiple times an already initialized instance
 * is returned.
 *
 * @overload initialize()
 *
 *   @example Redirector.new
 *
 *   @raise [RuntimeError] if failed to initialize the tun device.
 *
 *   @return [Redirector] self
 *     an object that encapsulates this instance.
 */
static VALUE
rb_init_redirector( VALUE self ) {
  if ( initialized ) {
    return cRedirector;
  }
  if ( !init_redirector() ) {
    rb_raise( rb_eRuntimeError, "Failed to initialize redirector" );
  }
  initialized = 1;
  return self;
}


static VALUE
rb_redirect( VALUE self, VALUE datapath_id, VALUE message ) {
  packet_in *_packet_in;
  Data_Get_Struct( message, packet_in, _packet_in );
  redirect( NUM2ULL( datapath_id ), _packet_in->in_port, _packet_in->data );
  return self;
}


void 
Init_redirector() {
  cRedirector = rb_define_class( "Redirector", rb_cObject );
  rb_define_method( cRedirector, "initialize", rb_init_redirector, 0 );
  rb_define_method( cRedirector, "redirect", rb_redirect, 2 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */