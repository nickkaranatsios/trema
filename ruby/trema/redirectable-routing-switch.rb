require "router"
require "packet_info"

class RedirectableRoutingSwitch < Trema::Controller
  include Router


  def start
puts "start is called"
    opts = RedirectableRoutingSwitchOptions.parse( ARGV )
    if  Authenticator.init_authenticator( opts[ :authorized_host_db ] )
      start_router( opts )
    end
  end



  def packet_in datapath_id, message
    return unless validate_in_port datapath_id, message.in_port
    return if message.macda.is_multicast?
    @fdb.learn message.macsa, message.in_port, datapath_id
    packet_info = PacketInfo.new( message )
    puts packet_info.udp_src_port
exit
    if !Authenticator.authenticate_mac( message.macsa )
      # if the array list is empty call redirect otherwise skip redirection
      authentication_exempted = AuthenticationFilter.apply( message.packet_info )
    end
  end


  class RedirectableRoutingSwitchOptions < Options
    def parse!( args ) 
      super
      args.options do | opts |
        opts.on( "-a",
          "--authorized-host-db  DB_FILE",
          "Authorized host database file" ) do | file |
            @options[ :authorized_host_db ] = file
        end
        opts.on( "-i",
          "--idle-timeout TIMEOUT",
          "Idle timeout value for flow entry" ) do | t |
        end
      end.parse!
      self
    end


    def default_options
      {
        :authorized_host_db => "db_file"
      }
    end
  end


  class AuthenticationFilter
    @subclasses = []


    class << self
      attr_reader :packet_info
      attr_reader :packet_info_methods


      def inherited(subclass)
        @subclasses << subclass
      end


      def apply_filter packet_info
        @packet_info_methods = packet_info.class.instance_methods( false )
        @packet_info = packet_info
        @subclasses.select { | subclass | subclass.new.allow? }
      end
    end


    def filter_attributes *attributes
      attributes.each_index do | i |
        attribute = attributes[ i ].to_s
        match = AuthenticationFilter.packet_info_methods.select { | v | v == attribute or v == attribute + "?" }
        if match.length == 1
          instance_variable_set("@#{ attributes[ i ] }", AuthenticationFilter.packet_info.send( match.to_s ) )
          instance_variable_get "@#{ attributes[ i ] }"
        end
      end
    end
  end



  class DhcpBootpFilter < AuthenticationFilter
    def allow?
      filter_attributes :udp_src_port, :udp_dst_port
      return @udp_src_port == 67 || @udp_src_port == 68 || @udp_dst_port == 67 || @udp_dst_port == 68
    end
  end


  class DnsUdpFilter < AuthenticationFilter
     def allow?
       filter_attributes :udp_src_port, :udp_dst_port
       return @udp_src_port == 53 || @udp_dst_port == 53
     end
  end


  class DnsTcpFilter < AuthenticationFilter
    def allow?
      filter_attributes :tcp_src_port, :tcp_dst_port
      return @tcp_src_port == 53 || @tcp_dst_port == 53
    end
  end


  class ArpFilter < AuthenticationFilter
    def allow?
      filter_attributes :arp
      return @arp
    end
  end
end
