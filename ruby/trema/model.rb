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


    #
    # Creates an new instance of subclass Options to parse the options.
    #
    # @return [Options] any instance of subclass of Options.
    #
    def self.parse args
      new.parse!( args )
    end


    #
    # Initialize options with a hash of default options.
    #
    # @return [Hash] 
    #
    def initialize
      @options = default_options
    end


    #
    # Extends the arguments array in order to parse options.
    #
    # @return [void]
    #
    def parse! args
      args.extend( ::OptionParser::Arguable )
    end


    ################################################################################
    protected
    ################################################################################


    #
    # Options getter method.
    #
    # @api protected
    #
    def []( key )
      @options[ key ]
    end


    #
    # Options setter method.
    #
    # @api protected
    #
    def []=( value )
      @options[ key ] = value
    end
  end


  class PortDS
    TD_PORT_EXTERNAL = 1
    TD_LINK_UP = 1
    TD_PORT_UP = 1
    #
    # A port represents any configured virtual link or a physical port of 
    # an openflow switch.
    #
    # @return [Number]
    #
    attr_accessor :port_no
    #
    # A status attached to a port to classify it as an edge port or not.
    #
    # @return [Boolean]
    #
    attr_accessor :external_link
    #
    # A status attached to a port that indicates a virtual or a physical 
    # connection to another switch.
    #
    # @return [Boolean]
    #
    attr_accessor :switch_to_switch_link
    #
    # A status attached to a port that indicates a reverse link connection
    # from one switch to another.
    #
    # @return [Boolean]
    #
    attr_accessor :switch_to_switch_reverse_link


    #
    # Creates a new instance.
    #
    # @param [Number] port_no
    #   the port number to associate with this instance.
    #
    def initialize port_no
      @port_no = port_no
    end


    #
    # Sets the port's external link status to true or false depending on the
    # external status parameter. Edge ports have their external link status
    # set to true.
    #
    # @param [Number] external_status
    #   if it's equal to TD_PORT_EXTERNAL sets the port's external link
    #   status to true otherwise sets it to false.
    #
    # @return [Boolean] external_link
    #   true or false depending on the evaluation result.
    #
    def external_link=( external_status )
      @external_link = external_status == TD_PORT_EXTERNAL
    end


    #
    # Sets the port's switch to switch link status to true or false depending
    # on the given status parameter. Edge ports have their switch to
    # switch link status set to false.
    #
    # @param [Number] status
    #   the status value to check to set port's switch to switch link status to
    #   true if the status equals TD_LINK_UP or false if not.
    #
    # @return [Boolean]
    #
    def switch_to_switch_link=( status )
      @switch_to_switch_link = link_status( status )
    end


    #
    # Sets the port's switch to switch reverse link status to true or false
    # depending on the given status parameter. Edge ports have their switch to
    # switch reverse link status set to false.
    #
    # @param [Number] status
    #   the status value to check to set port's switch to switch reverse link
    #   status to true if status equals TD_LINK_UP or false if not.
    #
    # @return [Boolean]
    #
    def switch_to_switch_reverse_link=( status )
      @switch_to_switch_reverse_link = link_status( status )
    end


    #
    # Given a link status parameter returns true if it is equal to +TD_LINK_UP+
    # otherwise false.
    #
    # @return [Boolean]
    #
    def link_status status
      status == TD_LINK_UP
    end


    #
    # True if the given port number equals the receiver's class instance
    # variable port_no otherwise false.
    #
    # @return [Boolean]
    #
    def include? _port_no
      @port_no == _port_no
    end


    #
    # True if the port is a non-edge port otherwise false. An non-edge port
    # is a port whose its external link status is set to false and its switch
    # to switch and switch to switch reverse link status is set to true.
    #
    # @return [Boolean]
    #
    def forwarding_port?
      @external_link == false and ( @switch_to_switch_link  == true and @switch_to_switch_link_reverse_link == true )
    end


    #
    # True if a port is an action port otherwise false. An action port is an
    # port whose its external link status is set to true or its switch to switch
    # reverse link status is false.
    #
    # @return [Boolean]
    #
    def action_port?
      if @external_link == false or @switch_to_switch_reverse_link == true
        return false
      end
      true
    end
  end


  class SwitchDS
    #
    # Create a new instance of SwitchDS that maintains a hash for each 
    # switch keyed by a datapath identifier.
    #
    def initialize
      @switches ||= {}
    end


    #
    # Adds or deletes a port to a switch data source according to port's
    # status up or down.
    #
    # @param [TopologyPortStatus] message
    #   the message that encapsulates this class instance.
    #
    # @return [void]
    #
    def process_port_status message
      dpid = message.dpid
      port_no = message.port_no
      if message.status == PortDS::TD_PORT_UP 
        add_port_to_switch dpid, port_no, message.external
      else
        delete_port dpid, port_no
      end
    end


    #
    # Updates the port's switch to switch link status or switch to switch
    # reverse link status depending on the status message.
    #
    # @param [TopologyLinkStatus] message
    #   the message that encapsulates this class instance.
    #
    # @return [void]
    #
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


    #
    # Validates a switch's port. The port to be valid must already been
    # configured and must have its external link status set to true.
    #
    # @return [Boolean]
    #   true if a valid port false otherwise.
    #
    def validate_port dpid, port_no
      port = lookup_port( dpid, port_no )
      return false unless port
      res = true
      if port.external_link == false || port.switch_to_switch_reverse_link == true
        res = port.forwarding_port?
      end
      res
    end


    #
    # Looks up switch's data source to locate a given port number for a switch
    # identified by its datapath id.
    #
    # @return [PortDS] an instance of class PortDS matched.
    # @return [NilClass]  nil a non-existent key is used to locate switch's 
    #   data source.
    #
    def lookup_port dpid, port_no
      return nil unless @switches.has_key? dpid
      @switches[ dpid ].detect { | port | port.port_no == port_no }
    end


    #
    # Iterates through each switch and call the given block passing 
    # switch's datapath identifier and ports, an array of PortDS instances.
    #
    def each &block
      @switches.each do | dpid, ports |
        block.call dpid, ports
      end
    end



    ################################################################################
    private
    ################################################################################


    #
    # Adds a port to switch's data source. If a port already exists update
    # its external link status.
    #
    # @param [Number] datapath_id
    #   the datapath identifier to which this port is attached.
    # @param [Number] port
    #   the port number to add.
    # @param [Number] external_link
    #   for an edge port the external_link is set to 1 otherwise 0.
    #
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


    #
    # Deletes a given port from switch's data source keyed by the given dpid.
    #
    # @param [Number] dpid
    #   the datapath identifier a unique key to switch's data source.
    # @param [Number] port_no
    #   a port number to delete.
    #
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
