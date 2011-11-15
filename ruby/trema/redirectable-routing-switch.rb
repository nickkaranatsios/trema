require "router"

class RedirectableRoutingSwitch < Controller
  include Router


  def start
puts "start is called"
    opts = RoutingSwitchOptions.parse( ARGV )
    extra_opts = RedirectableRoutingSwitchOptions.parse( ARGV )
    merged_opts = opts.merge( extra_opts )
    start_router( merged_opts )
  end



  def packet_in datapath_id, message
    return unless validate_in_port datapath_id, message.in_port
    return if message.macda.is_multicast?
    @fdb.learn message.macsa, message.in_port, datapath_id
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
      end.parse!
      self
    end


    def default_options
      {
        :authorized_host_db => "db_file"
      }
    end
  end
end
