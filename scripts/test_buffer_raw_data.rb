#!/usr/bin/env ruby
require 'readline'
require 'orocos'
include Orocos
Orocos.initialize

ENV['BASE_LOG_LEVEL'] = 'DEBUG'
ENV['BASE_LOG_FORMAT'] = 'SHORT'
ENV['ORO_LOGLEVEL'] = '6'

#Orocos.run 'type_to_vector_test', 'valgrind' => true do |p|
Orocos.run 'type_to_vector_test' do |p|
    
    task = TaskContext.get 'TestBufferedTask'

    puts "--- Test port creation for buffered task ---"

    task.debug_conversion = true
    task.debug_buffer = true
    task.buffer_size = 3
    task.buffer_time = true

    puts "- add port rbs1 of type /base/samples/RigidBodyState"
    puts "  data are added to vextorIdx 0"
    
    rbs1_p = Types::TypeToVector::PortConfig.new
    rbs1_p.portname = "rbs1"
    rbs1_p.type = "/base/samples/RigidBodyState"
    rbs1_p.slice = "position orientation"
    rbs1_p.vectorIdx = 0
    rbs1_p.period = 0.1
    rbs1_p.useTimeNow = true

    task.addPort(rbs1_p)

    puts "- ports are created in the configureHook"
    
    task.configure

    w_rbs = task.rbs1_raw.writer
    r_b0 = task.debug_buffer_0.reader

    Readline.readline "Press enter to start task." do
    end

    task.start
    
    rbs = w_rbs.new_sample
    rbs.data = [1.0, 2.0, 3.0, 0.9, -0.1, -0.2, -0.3]

    (1..10).each do
        
        w_rbs.write(rbs)

        sleep 0.1

        if cv = r_b0.read()
            puts "--- debug buffer 0 ---"
            puts "time:"
            puts cv.time
            puts "data:"
            puts cv.data
        end
    end    
end

