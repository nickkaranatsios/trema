class PacketInfo
  attr_accessor :receiver
  def initialize packet_in
    @receiver = packet_in.packet_info( self.class )
  end

  def udp_src_port
    packet_info_udp_src_port @receiver
  end
end
