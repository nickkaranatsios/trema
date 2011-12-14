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


if ENV.has_key? 'TREMA_APPS'
  trema_apps = ENV[ 'TREMA_APPS' ]
else
  trema_apps = "../apps_backup"
end


$LOAD_PATH << "#{ trema_apps }/topology"
$LOAD_PATH << "#{ trema_apps }/path_resolver_client"


require "observer"
require "trema/model"
require "topology_client"
require "path_resolver_client"


module Trema
  module Router
    include Observable, TopologyClient, PathResolverClient, Model


    #
    # Saves the options, initializes databases and starts the topology
    # application service. Registers the current instance to receive topology
    # update status messages.
    #
    # @param [Options] options
    #   a subclass instance of class Options.
    #
    # @return [void]
    #
    def start_router options
      @opts = options
      @model_ds = SwitchDS.new
      @fdb = FDB.new
      start_topology
      add_observer self
    end


    #
    # Starts the path resolver and topology application service.
    #
    # @return [void]
    #
    def start_topology
      init_path_resolver_client
      init_topology_client name
    end


    #
    # Finalizes the path resolver and topology application service.
    #
    # @return [void]
    #
    def stop_topology
      finalize_path_resolver_client
      finalize_topology_client
    end


    #
    # Processes a topology update status message. There are two kind of
    # supported messages link and port.
    #
    # @raise [Exception] message
    #   raises an exception message if an invalid kind of topology update message
    #   is received.
    #
    # @return [void]
    #
    def update message, kind
      case kind
      when :link_status
puts "TopologyLinkStatus #{message.from_dpid.to_hex} #{message.to_dpid.to_hex}, #{message.from_portno}, #{message.to_portno}, #{message.status}"
        process_link_status message
      when :port_status
puts "TopologyPortStatus #{message.dpid.to_hex} #{message.port_no}, #{message.status}, #{message.external}"
        process_port_status message
      else
        raise "An invalid topology change message received"
      end
    end


    #
    # Processes the packet in message. Validates the originator and learns
    # the source mac address. Finally it makes a path and forwards the
    # packet to destination if the destination mac address is known or floods
    # the packet on the network.
    #
    # @param [Number] datapath_id
    #   the datapath identifier of the received packet in message.
    # @param [PacketIn] message
    #   the packet in received message to process.
    #
    # @return [void]
    #
    def packet_in datapath_id, message
      # abort processing if in_port is not known.
      return unless validate_in_port datapath_id, message.in_port
      return if message.macda.is_multicast?
      @fdb.learn message.macsa, message.in_port, datapath_id
      if dest = @fdb.lookup( message.macda )
        make_path datapath_id, message, dest
      else
        flood_packet datapath_id, message
      end
    end


    #
    # Overrides the switch_ready handler to send a features request message.
    #
    # @param [Number] datapath_id
    #   the datapath identifier that become available.
    #
    # @return [void]
    #
    def switch_ready datapath_id
      send_message datapath_id, FeaturesRequest.new
    end


    #
    # Overrides the features_reply handler to send a set config request message.
    # Sets the miss_send_len to a maximum value of a short integer value so that 
    # the packet in's data portion includes a large portion of user data.
    #
    # @param [FeaturesReply] message
    #  the message that encapsulates the features reply.
    #
    # @return [void]
    #
    def features_reply message
      send_message message.datapath_id, SetConfigRequest.new( :miss_send_len => 2**16 -1 )
    end


    #
    # Validate packet in's input port.
    #
    # @param [Number] datapath_id
    #   the datapath identifier of the received packet in message.
    # @param [Number] port
    #   the input port on which the packet in is received.
    #
    # @return [Boolean] whether or not the +port+ is valid.
    #
    def validate_in_port datapath_id, port
      @model_ds.validate_port datapath_id, port
    end


    #
    # Processes the topology port status message.
    #
    # @param [TopologyPortStatus] message
    #   a message that represents an instance of the class TopologyPortStatus.
    #
    # @return [void]
    #
    def process_port_status message
      @model_ds.process_port_status message
    end


    #
    # Processes the topology link status message. Informs the path resolver
    # of the topology change.
    #
    # @param [TopologyLinkStatus] message
    #   a message that represents an instance of the class TopologyLinkStatus.
    #
    # @return [void]
    #
    def process_link_status message
      @model_ds.process_link_status message
      update_path message
    end


    #
    # Resolves a path and constructs flows to output the packet
    # otherwise discards the packet in.
    #
    # @param [Number] in_datapath_id
    #   the datapath identifier of the received packet in message.
    # @param {PacketIn] message
    #   the received packet in message.
    # @param [Array] dest
    #   an array of destination identification information 
    #   (datapath_id, out_port)
    #
    # @return [void]
    #
    def make_path in_datapath_id, message, dest
      out_datapath_id, out_port = *dest
      if hops = path_resolve( in_datapath_id, message.in_port, out_datapath_id, out_port )
        modify_flow_entry hops, message
        output_packet_from_last_switch hops.last, message
      else
        discard_packet_in in_datapath_id, message
      end
    end


    #
    # Sends a packet out to the last hop to output it on the specified output
    # port.
    #
    # @param [PathResolverHop] last_hop
    #   last hop path information dpid, output port number.
    # @param [PacketIn] message
    #   the packet in message to output.
    #
    # @return [void]
    #
    def output_packet_from_last_switch last_hop, message
      send_packet_out( last_hop.dpid,
        :packet_in => message,
        :actions => ActionOutput.new( :port => last_hop.out_port_no )
      )
    end


    #
    # Creates a flow entry to discard a packet in.
    #
    # @param [Number] datapath_id
    #   the datapath identifier that the flow entry would be created.
    # @param [PacketIn] message
    #   the received packet in message. Copy the packet in's match structure
    #   to the createda flow entry match structure.
    #
    # @return [void]
    #
    def discard_packet_in datapath_id, message
      send_flow_mod_add( datapath_id,
        :match => Match.from( message ),
        :hard_timeout => @opts.packet_in_discard_duration
      )
    end


    #
    # For each hop resolved creates a flow entry so that subsequent packets
    # can be forwarded according to flow entries without resulting in flow 
    # entry misses.
    #
    # @param [Array<PathResolverHop>] hops
    #   an array of PathResolverHop instances.
    # @param [message] message
    #   the received packet in message.
    #
    # @return [void]
    #
    def modify_flow_entry hops, message
      nr_hops = hops.length
      hops.each do | hop |
        idle_timeout = @opts.idle_timeout + nr_hops
        nr_hops -= 1
        send_flow_mod_add( hop.dpid,
          :match => Match.from( message ),
          :idle_timeout => idle_timeout,
          :actions => ActionOutput.new( :port => hop.out_port_no )
        )
      end
    end


    #
    # Floods the network by sending a packet out to every interconnected node 
    # and every port on the network except received packet in's input port
    #
    # @param [Number] datapath_id
    #   the datapath identifier of the received packet in message.
    # @param [PacketIn] message
    #   the received packet in message.
    #
    # @return [void]
    #
    def flood_packet datapath_id, message
      @model_ds.each do | dpid, ports |
        actions = []
        ports.each do | port |
          next if port.include?( message.in_port ) and dpid == datapath_id
          actions << ActionOutput.new( :port => port.port_no ) if port.action_port?
        end
        send_packet_out( dpid, :packet_in => message, :actions => actions ) if actions.length
      end
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
