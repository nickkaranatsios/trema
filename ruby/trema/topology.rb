require "observer"

module Trema
  module Topology
    include Observable
    def topology_notifier message, kind
      changed
      notify_observers message, kind
    end
  end
end
