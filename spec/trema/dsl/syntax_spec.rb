#
# Author: Yasuhito Takamiya <yasuhito@gmail.com>
#
# Copyright (C) 2008-2011 NEC Corporation
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


require File.join( File.dirname( __FILE__ ), "..", "..", "spec_helper" )
require "trema/dsl/syntax"
require "trema/packetin-filter"


describe Trema::DSL::Syntax do
  before( :each ) do
    @context = mock( "context", :port => 6633, :dump_to => nil )
    @syntax = Trema::DSL::Syntax.new( @context )
  end


  it "should recognize 'use_tremashark' directive" do
    @context.should_receive( :tremashark= ).with( an_instance_of( Tremashark ) ).once

    @syntax.instance_eval do
      use_tremashark
    end
  end


  it "should recognize 'port' directive" do
    @context.should_receive( :port= ).with( 1234 ).once

    @syntax.instance_eval do
      port 1234
    end
  end


  it "should recognize 'link' directive" do
    @context.stub!( :links ).and_return( [ mock( "link" ) ] )
    Trema::Link.should_receive( :add ).with( an_instance_of( Trema::Link ) ).once

    @syntax.instance_eval do
      link "PEER0", "PEER1"
    end
  end


  it "should recognize 'switch' directive" do
    Trema::Switch.should_receive( :add ).with( an_instance_of( OpenflowSwitch ) ).once

    @syntax.instance_eval do
      switch { }
    end
  end


  it "should recognize 'vswitch' directive" do
    Trema::Switch.should_receive( :add ).with( an_instance_of( OpenVswitch ) ).once

    @syntax.instance_eval do
      vswitch { }
    end
  end


  it "should recognize 'vhost' directive" do
    Trema::Host.should_receive( :add ).with( an_instance_of( Trema::Host ) ).once

    @syntax.instance_eval do
      vhost { }
    end
  end


  it "should recognize 'filter' directive" do
    Trema::PacketinFilter.should_receive( :add ).with( an_instance_of( Trema::PacketinFilter ) ).once

    @syntax.instance_eval do
      filter :lldp => "LLDP RULE", :packet_in => "PACKET-IN RULE"
    end
  end


  it "should recognize 'event' directive" do
    Trema::SwitchManager.should_receive( :add ).with( an_instance_of( Trema::SwitchManager ) ).once

    @syntax.instance_eval do
      event "RULE"
    end
  end


  it "should recognize 'app' directive" do
    Trema::App.should_receive( :add ).with( an_instance_of( Trema::App ) ).once

    @syntax.instance_eval do
      app { }
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:

