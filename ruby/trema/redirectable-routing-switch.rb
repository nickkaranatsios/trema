require "router"

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
    if !Authenticator.authenticate_mac( message.macsa )
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


  class ExemptionFilter
    def initialize packet_info
      @udp_src_port = packet_info.udp_src_port
      @udp_dst_port = packet_info.udp_dst_port
      @tcp_src_port = packet_info.tcp_src_port
      @tcp_dst_port = packet_info.tcp_dst_port
      @arp = packet_info.arp?
    end
  end
end
