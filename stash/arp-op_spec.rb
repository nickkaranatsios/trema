#
# Author: Nick Karanatsios <nickkaranatsios@gmail.com>
#
# Copyright (C) 2008-2013 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#


require File.join( File.dirname( __FILE__ ), "..", "spec_helper" )
require "trema"


describe ArpOp, "new( VALID OPTIONS )" do
  subject { ArpOp.new arp_op: 1 }
  let( :arp_op ) { 1 }
  its ( :arp_op ) { should == 1 }
end


describe ArpOp, ".new( MANDADORY OPTION MISSING ) - arp op" do
  subject { ArpOp.new }
  it "should raise ArgumentError" do
    expect { subject }.to raise_error( ArgumentError, /Required option arp_op missing/ )
  end
end


describe ArpOp, ".new( VALID OPTIONS )" do
  context "when sending #flow_mod(add) with a match field set to ArpOp" do
    it "should respond to #pack_arp_op" do
      conf_blk = Proc.new {
        trema_switch( "lsw" ) { datapath_id 0xabc }
        vhost( "host1" ) {
          ip "192.168.0.1"
          netmask "255.255.255.0"
          mac "00:00:00:00:00:01"
        }
        vhost( "host2" ) {
          ip "192.168.0.2"
          netmask "255.255.255.0"
          mac "00:00:00:00:00:02"
        }
        link "host1", "lsw:1"
        link "host2", "lsw:2"
      }
      ControllerMock.start( :switch_ready, conf_blk ) do | controller, datapath_id |
        match_field = ArpOp.new( arp_op: 1 ) 
        match_field.should_receive( :pack_arp_op )
        controller.send_flow_mod_add( datapath_id,
                                      cookie: 1111,
                                      match: match_field )
      end
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
