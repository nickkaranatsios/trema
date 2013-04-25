#
# Author: Yasuhito Takamiya <yasuhito@gmail.com>
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


$LOAD_PATH << File.join( File.dirname( __FILE__ ), "..", "ruby" )
$LOAD_PATH.unshift File.expand_path( File.join File.dirname( __FILE__ ), "..", "vendor", "ruby-ifconfig-1.2", "lib" )


require "rubygems"

require "rspec"
require "trema"
require "trema/dsl/configuration"
require "trema/dsl/context"
require "trema/util"


require "coveralls"
Coveralls.wear!


RSpec.configure do | config |
  config.expect_with :rspec do | c |
    # Ensure that 'expect' is used and disable 'should' for consistency
    c.syntax = :expect
  end
end


# Requires supporting files with custom matchers and macros, etc,
# in ./support/ and its subdirectories.
Dir[ "#{ File.dirname( __FILE__ ) }/support/**/*.rb" ].each do | each |
  require File.expand_path( each )
end


include Trema


def controller name
  Trema::App[ name ]
end


def vswitch name
  Trema::OpenflowSwitch[ name ]
end


def vhost name
  Trema::Host[ name ]
end


def send_packets source, dest, options = {}
  Trema::Shell.send_packets source, dest, options
end


include Trema::Util


class Network
  def initialize &block
    @context = Trema::DSL::Parser.new.eval( &block )
  end


  def run controller_class, &test
    begin
      trema_run controller_class
      test.call
    ensure
      trema_kill
    end
  end


  ################################################################################
  private
  ################################################################################


  def trema_run controller_class
    controller = controller_class.new
    if not controller.is_a?( Trema::Controller )
      raise "#{ controller_class } is not a subclass of Trema::Controller"
    end
    Trema::DSL::Context.new( @context ).dump

    app_name = controller.name
    rule = { :port_status => app_name, :packet_in => app_name, :state_notify => app_name }
    sm = SwitchManager.new( rule, @context.port )
    sm.no_flow_cleanup = true
    sm.run!

    @context.links.each do | name, each |
      each.add!
    end
    @context.hosts.each do | name, each |
      each.run!
    end
    @context.trema_switches.each do | name, each |
      each.run!
    end
    @context.links.each do | name, each |
      each.up!
    end
    @context.hosts.each do | name, each |
      each.add_arp_entry @context.hosts.values - [ each ]
    end

    @th_controller = Thread.start do
      controller.run!
    end
    sleep 2  # FIXME: wait until controller.up?
  end


  def trema_kill
    cleanup_current_session
    @th_controller.join if @th_controller
    sleep 2  # FIXME: wait until switch_manager.down?
  end
end


def network &block
  Network.new &block
end

class ControllerMock < Controller
  def initialize handlers, conf_blk, &callback_blk
    @handlers = handlers
    @conf_blk = conf_blk
    @callback_blk = callback_blk
    @context = Trema::DSL::Parser.new.eval( &conf_blk )
  end


  def self.start handler, conf_blk, &callback_blk
    new( handler, conf_blk, callback_blk ) unless conf_blk.nil?
  end


  def start_handlers
    if @conf_blk.respond_to? :call
      begin
        trema_run
      ensure
        trema_kill
      end
    end
  end


  def switch_ready datapath_id
    @callback_blk.call( self, datapath_id )
   end


  def packet_in datapath_id, message
    @callback_blk.call( self, datapath_id, message )
  end


  def yield_handler options
     if @handlers.include? method
      yield options
    end
  end


  def trema_run
    controller = self
    if not controller.is_a?( Trema::Controller )
      raise "#{ controller_class } is not a subclass of Trema::Controller"
    end
    Trema::DSL::Context.new( @context ).dump

    app_name = controller.name
    rule = { :port_status => app_name, :packet_in => app_name, :state_notify => app_name }
    sm = SwitchManager.new( rule, @context.port )
    sm.no_flow_cleanup = true
    sm.run!

    @context.links.each do | name, each |
      each.add!
    end
    @context.hosts.each do | name, each |
      each.run!
    end
    @context.trema_switches.each do | name, each |
      each.run!
    end
    @context.links.each do | name, each |
      each.up!
    end
    @context.hosts.each do | name, each |
      each.add_arp_entry @context.hosts.values - [ each ]
    end

    @th_controller = Thread.start do
      controller.run!
    end
    sleep 2  # FIXME: wait until controller.up?
  end


  def trema_kill
    cleanup_current_session
    @th_controller.join if @th_controller
    sleep 2  # FIXME: wait until switch_manager.down?
  end
end

def controller_mock handlers, conf_blk, &callback_blk
  ControllerMock.new( handlers, conf_blk, &callback_blk ).start_handlers
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
