#
# Copyright (C) 2008-2013 NEC Corporation
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


require_relative "mapping"


module Trema
  #
  # A module for defining user defined like accessors.
  #
  module Accessor
    def self.included klass
      klass.extend ClassMethods
      klass.instance_eval do
        include InstanceMethods
      end
      klass.setup_accessor_methods
puts klass.methods.sort
    end


    module ClassMethods
      # mixin mapping of ofp type to implementation class.
      include Mapping
      USER_DEFINED_TYPES = %w( ip_addr mac match packet_info array string bool )


      class AttributeProperty
        attr_reader :required, :default, :alias


        def initialize
          @required ||= []
          @default ||= {}
          @alias ||= {}
        end


        def attributes
          @attributes ||= AttributeProperty.new
        end
      end


      def search type, sub_key
        key = "#{ type }_#{ sub_key }"
        retrieve key
      end


      def setup_accessor_methods
        #map_ofp_type klass
        primitive_sizes.each do | each |
          define_accessor_meth :"unsigned_int#{ each }"
          define_method :"check_unsigned_int#{ each }" do | number, name |
            begin
              unless number.send( "unsigned_#{ each }bit?" )
                raise ArgumentError, "#{ name } must be an unsigned #{ each }-bit integer."
              end
            rescue NoMethodError
              raise TypeError, "#{ name } must be a Number."
            end
          end
        end
        USER_DEFINED_TYPES.each{ | meth | define_accessor_meth meth }
      end


      ############################################################################
      private
      ############################################################################


      def primitive_sizes
        ( 8..64 ).step( 8 ).select{ | i | i.to_s( 2 ).count( '1' ) == 1 }
      end


      def define_accessor_meth meth
        class_eval do
          define_method :"#{ meth }" do | *args |
            attrs = args
            opts = extract_options!( args )
            check_args args
            attrs.delete( opts ) unless opts.empty?
            opts.store :attributes, attrs
            opts.store :validate_with, "check_#{ meth }" if meth.to_s[ /unsigned_int\d\d/ ]
            attrs.each do | attr_name |
              define_accessor attr_name, opts
              attributes.required << attr_name if opts.has_key? :presence
              attributes.default[ attr_name ] = opts[ :default ] if opts.has_key? :default
            end
          end
        end
      end


      def define_accessor attr_name, opts
        class_eval do
          define_method attr_name do
            instance_variable_get "@#{ attr_name }"
          end

          define_method "#{ attr_name }=" do | v |
            if opts.has_key? :presence
              if opts[ :presence ] == true
                if v.nil?
                  raise ArgumentError, "#{ attr_name } is a mandatory option."
                end
              end
            end
            validation_methods = opts.select { | key, _ | key == :within or key == :validate_with }
            validation_methods.each { | _, meth | __send__( meth, v, attr_name ) }
            instance_variable_set "@#{ attr_name }", v
          end
        end
      end


      def extract_options! args
        if args.last.is_a?( Hash ) && args.last.instance_of?( Hash )
          args.pop
        else
          {}
        end
      end


      def check_args args
        raise ArgumentError, "You need at least one attribute" if args.empty?
      end
    end


    module InstanceMethods
      def initialize options=nil
        setters = self.class.instance_methods.select{ | i | i.to_s =~ /[a-z].+=$/ }.delete_if{ | i | i.to_s =~ /required_attributes=/ }
        required = self.class.attributes.required
        if required.empty?
          required = self.class.superclass.attributes.required
        end

        set_default setters
        case options
        when Hash
          setters.each do | each |
            opt_key = each.to_s.sub( '=', '' ).to_sym
            if options.has_key? opt_key
              public_send each, options[ opt_key ]
            else
              raise ArgumentError, "Required option #{ opt_key } is missing for #{ self.class.name }" if required.include? opt_key
            end
          end
        when Integer, String
          unless setters.empty?
            public_send setters[ 0 ], options
          else
            raise ArgumentError, "#{ self.class.name } accepts no options"
          end
          else
            raise ArgumentError, "Required option #{ required_attributes.first } missing for #{ self.class.name }" unless required_attributes.empty?
          end
        end
      end


      def set_default setters
        default = self.class.attributes.default
        setters.each do | each |
        opt_key = each.to_s.sub( '=', '' ).to_sym
        if default.has_key? opt_key
          if default[ opt_key ].respond_to? :call
            public_send each, default[ opt_key ].call
          else
            public_send each, default[ opt_key ]
          end
        end
      end
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End: