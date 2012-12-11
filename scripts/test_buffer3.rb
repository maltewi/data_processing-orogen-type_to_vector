require 'readline'
require 'orocos'
include Orocos
Orocos.initialize

ENV['BASE_LOG_LEVEL'] = 'DEBUG'
ENV['BASE_LOG_FORMAT'] = 'SHORT'
ENV['ORO_LOGLEVEL'] = '6'

# Testing the debug port creation

#Orocos.run 'general_processing_test', 'valgrind' => true do |p|
Orocos.run 'general_processing_test' do |p|

    task = TaskContext.get 'TestBufferedTask'

    task.debug_conversion = false
    task.create_places = false
    task.aggregator_max_latency = 0.01
    task.debug_buffer = true
    task.buffer_time = true
    task.buffer_size = 5
    task.buffer_new_only = true

    task.createInputPort("rbs1","/base/samples/RigidBodyState","position orientation",0)


    puts "Task  Type  Orocos_Type"
    puts "--- Input ports ---"
    task.each_input_port do |p|
        puts "#{p.name}  #{p.type_name}  #{p.orocos_type_name}"
    end
    
    puts "\n--- Output ports ---"
    task.each_output_port do |p|
        puts "#{p.name}  #{p.type_name}  #{p.orocos_type_name}"
    end

    w_rbs = task.rbs1.writer
    r_b0 = task.debug_buffer_0.reader

    Readline.readline "Press enter to start task." do
    end

    task.configure
    task.start
    
    rbs = w_rbs.new_sample
    rbs.position = Eigen::Vector3.new(1.0,2.0,3.0)
    rbs.orientation = Eigen::Quaternion.new(0.9,-0.1,-0.2,-0.3)
    rbs.time = Time.now

    (1..10).each do
        
        rbs.time = Time.now
        w_rbs.write(rbs)

        sleep 0.2

        if cv = r_b0.read()
            puts "--- debug buffer 0 ---"
            puts "time:"
            puts cv.time
            puts "data:"
            puts cv.data
        end
    end    
end
