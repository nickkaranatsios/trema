module Trema
  module PathResolver
    def path_update message
      update_path message.from_dpid, message.to_dpid, message.from_portno, message.to_portno, message.status
    end
  end
end
