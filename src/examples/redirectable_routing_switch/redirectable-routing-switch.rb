$LOAD_PATH << "./src/examples/authenticator/"
$LOAD_PATH << "./src/examples/redirector/"


require "trema/router"
require "authenticator"
require "redirector"


class RedirectableRoutingSwitch < Trema::Controller
  include Router


  def start
    opts = RedirectableRoutingSwitchOptions.parse( ARGV )
    if  @authenticator = Authenticator.new( opts[ :authorized_host_db ] )
      @redirector = Redirector.new
      start_router( opts )
    end
  end


  def packet_in datapath_id, message
    return unless validate_in_port datapath_id, message.in_port
    return if message.macda.is_multicast?
    @fdb.learn message.macsa, message.in_port, datapath_id
puts message.macsa
    if !@authenticator.authenticate_mac( message.macsa )
      # if the array list is empty call redirect otherwise skip redirection
      filtered = AuthenticationFilter.apply( message )
puts "calling redirect #{filtered.length}"
      if filtered.length == 0
        @redirector.redirect( datapath_id, message )
      end
    else
      if dest = @fdb.lookup( message.macda )
puts "make_path"
        make_path datapath_id, message, dest
      else
puts "flood packet"
        flood_packet datapath_id, message
      end
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
            @options[ :idle_timeout ] = t.to_i
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
      attr_reader :packet_in
      attr_reader :packet_in_methods


      def inherited subclass
        @subclasses << subclass
      end


      def apply packet_in
        @packet_in_methods = packet_in.class.instance_methods( false )
        @packet_in = packet_in
        @subclasses.select { | subclass | subclass.new.allow? }
      end
    end


    def filter_attributes *attributes
      attributes.each_index do | i |
        attribute = attributes[ i ].to_s
        match = AuthenticationFilter.packet_in_methods.select { | v | v == attribute or v == attribute + "?" }
        if match.length == 1
          instance_variable_set("@#{ attributes[ i ] }", AuthenticationFilter.packet_in.send( match.to_s ) )
          instance_variable_get "@#{ attributes[ i ] }"
        end
      end
    end
  end



  class DhcpBootpFilter < AuthenticationFilter
    def allow?
      filter_attributes :udp_src_port, :udp_dst_port
puts @udp_src_port, @udp_dst_port
      return @udp_src_port == 67 || @udp_src_port == 68 || @udp_dst_port == 67 || @udp_dst_port == 68
    end
  end


  class DnsUdpFilter < AuthenticationFilter
     def allow?
       filter_attributes :udp_src_port, :udp_dst_port
puts @udp_src_port, @udp_dst_port
       return @udp_src_port == 53 || @udp_dst_port == 53
     end
  end


  class DnsTcpFilter < AuthenticationFilter
    def allow?
      filter_attributes :tcp_src_port, :tcp_dst_port
puts @tcp_src_port, @tcp_dst_port
      return @tcp_src_port == 53 || @tcp_dst_port == 53
    end
  end


  class ArpFilter < AuthenticationFilter
    def allow?
      filter_attributes :arp
puts @arp
      return @arp
    end
  end
end