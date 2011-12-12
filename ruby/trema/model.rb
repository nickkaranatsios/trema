#
# Author: Nick Karanatsios <nickkaranatsios@gmail.com>
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


require "trema/fdb"


module Model
  class Options
    attr_reader :options


    def self.parse args
      new.parse!( args )
    end


    def initialize
      @options = default_options
    end


    def parse! args
      args.extend( ::OptionParser::Arguable )
    end


    ################################################################################
    protected
    ################################################################################


    #
    # Options protected getter method.
    #
    def []( key )
      @options[ key ]
    end


    #
    # Options protected setter method.
    #
    def []=( value )
      @options[ key ] = value
    end
  end


  class PortDS
    TD_PORT_EXTERNAL = 1
    TD_LINK_UP = 1
    TD_PORT_UP = 1
    attr_accessor :port_no, :external_link, :switch_to_switch_link, :switch_to_switch_reverse_link


    #
    # Creates a port data source instance.
    #
    # @param [Number] port_no
    #   the port number to save to into its corresponding attribute.
    #
    def initialize port_no
      @port_no = port_no
    end


    def update_link status, which_link = 0
      if !which_link
        @switch_to_switch_link = status 
      else
        @switch_to_switch_reverse_link = status
      end
    end


    #
    # Sets the port's external link status to true or false depending on the
    # value parameter.
    #
    # @param [Number] value
    #   the value to check to set port's external link status to true
    #   if value equals TD_PORT_EXTERNAL or false if not.
    #
    #
    def external_link=( value )
      @external_link = value == TD_PORT_EXTERNAL
    end


    #
    # Sets the port's switch to switch link status to true or false depending 
    # on the given status parameter.
    #
    # @param [Number] status
    #   the status value to check to set port's switch to switch link status to
    #   true if status equals TD_LINK_UP or false if not.
    #
    def switch_to_switch_link=( status )
      @switch_to_switch_link = link_status( status )
    end


    #
    # Sets the port's switch to switch reverse link status to true or false 
    # depending on the given status parameter.
    #
    # @param [Number] status
    #   the status value to check to set port's switch to switch reverse link
    #   status to true if status equals TD_LINK_UP or false if not.
    #
    def switch_to_switch_reverse_link=( status )
      @switch_to_switch_reverse_link = link_status( status )
    end


    #
    # Checks the link status parameter.
    #
    # @return [Boolean] true if status is TD_LINK_UP otherwise false.
    #
    def link_status status
      status == TD_LINK_UP
    end


    # FIXME maybe is better to convert this method to equal.
    def include? other
      @port_no == other
    end


    #
    # @return [Boolean]
    #   true if the port is a non-edge port which means the external link 
    #   status is set to false and its switch to switch and switch to switch 
    #   reverse link status is set to true otherwise it returns false.
    #
    def forwarding_port?
      @external_link == false and ( @switch_to_switch_link  == true and @switch_to_switch_link_reverse_link == true )
    end


    def action_port?
      if @external_link == false or @switch_to_switch_reverse_link == true
        return false
      end
      true
    end
  end


  class SwitchDS
    def initialize
      @switches ||= {}
    end


    def process_port_status message
      dpid = message.dpid
      port_no = message.port_no
      if message.status == PortDS::TD_PORT_UP 
        add_port_to_switch dpid, port_no, message.external
      else
        delete_port dpid, port_no
      end
    end


    def process_link_status message
      if port = lookup_port( message.from_dpid, message.from_portno )
        port.switch_to_switch_link = message.status
      end
      if message.to_portno
        if port = lookup_port( message.to_dpid, message.to_portno )
          port.switch_to_switch_reverse_link =  message.status
        end
      end
    end


    def validate_port dpid, port_no
      port = lookup_port( dpid, port_no )
      return unless port
      res = true
      if port.external_link == false || port.switch_to_switch_reverse_link == true
        res = port.forwarding_port?
      end
      res
    end


    def lookup_port dpid, port_no
      return nil unless @switches.has_key? dpid
      @switches[ dpid ].detect { | port | port.port_no == port_no }
    end


    def update_link dpid, port_no, status, which_link = 0
      if port = lookup_port( dpid, port_no )
        port.update_link status, which_link
      end
    end


    def each &block
      @switches.each do | dpid, ports |
        block.call dpid, ports
      end
    end



    ################################################################################
    private
    ################################################################################


    def add_port_to_switch dpid, port, external_link
      new_port = PortDS.new( port )
      new_port.external_link = external_link
      if @switches.has_key? dpid
        ports = @switches[ dpid ]
        if port = lookup_port( dpid, port )
          port.external_link = external_link
        else
          ports << new_port
        end
      else
        ports = [ new_port ]
      end
      @switches[ dpid ] = ports
    end


    def delete_port dpid, port_no
      if port = lookup_port( dpid, port_no )
        @switches[ dpid ].delete port
      end
    end


  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
