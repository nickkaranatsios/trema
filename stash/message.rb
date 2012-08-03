require "bindata"
require "stringio"

module Trema
  module Message


    AttributeSpec = Struct.new( :name, :klass, :type )


    class << self
      def headers
        @headers ||= []
        @headers
      end


      def header( name, klass )
        name = name.to_sym
        raise "#{ name } already defined" if method_defined?( :name )
        self.headers << name
        define_method( name ) do
          header = instance_variable_get( "@#{ name }" )
          unless header
            header = klass.new
            instance_variable_set( "@#{ name }", header )
          end
          header
        end
        define_method( "#{ name }=" ) do | val |
          instance_varaible_set( "@#{ name }", val )
        end
      end
    end
  end
end


module Trema
  module Message
    class Enqueue < BinData::Record
      endian :big
      uint16 :port_number, :initial_value => 0
      uint32 :queue_id, :initial_value => 0
    end
    eq = Enqueue.new( :port_number => 5555, :queue_id => 12345 )
puts eq.queue_id.to_binary_s
  end
end


module Trema
  class EnqueueMessage < Trema::Message::Enqueue
    Trema::Message.header :enqueue_header, Trema::Message::Enqueue
  end
end

module Daemon
  module ClassMethods
    def commands
      @commands ||= []
    end

    def log_files
      @log_files ||= []
    end
  end
  class << self
    def append_features klass
      super
      return if klass.instance_of? Module
      klass.extend ClassMethods
      klass.extend self
    end
  end

  def command &block
    commands << block
  end
  
  def print_command
    puts commands.inspect
  end
end

class Test
  include Daemon
  command { |x| puts "the value of x is #{x}" }
  
  def show_command
    self.class.print_command
  end
end


class Y
  include Daemon
  command { |x| puts "the value of x is #{x}" }
end
  

x = Test.new
x.show_command
puts Y.print_command
