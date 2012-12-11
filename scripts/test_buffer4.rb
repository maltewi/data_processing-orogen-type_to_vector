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
    task.aggregator_max_latency = 1.0
    task.debug_buffer = true
    task.buffer_time = true
    task.buffer_size = 5
    task.buffer_new_only = false

    task.createInputPort("rbs1","/base/samples/RigidBodyState","position orientation",0)
    task.createInputPort("mc2d","/base/MotionCommand2D","",0)
    task.createInputPort("rbs2","/base/samples/RigidBodyState","orientation",1)

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
    w_mc2d = task.mc2d.writer
    r_b0 = task.debug_buffer_0.reader
    
    w_rbs1 = task.rbs2.writer
    r_b1 = task.debug_buffer_1.reader

    Readline.readline "Press enter to start task." do
    end

    task.configure
    task.start
    
    mc2d = w_mc2d.new_sample
    mc2d.translation = 10.0
    mc2d.rotation = -1.0
    
    rbs = w_rbs.new_sample
    rbs.position = Eigen::Vector3.new(1.0,2.0,3.0)
    rbs.orientation = Eigen::Quaternion.new(0.9,-0.1,-0.2,-0.3)
    rbs.time = Time.now

    (1..10).each do
        
        rbs.time = Time.now
        w_rbs.write(rbs)
        sleep 0.1
        w_mc2d.write(mc2d)

    
        w_rbs1.write(rbs)
        sleep 0.1

        if cv = r_b0.read()
            puts "--- debug buffer 0 ---"
            puts "time:"
            puts cv.time
            puts "data:"
            puts cv.data
        end
        if cv = r_b1.read()
            puts "--- debug buffer 1 ---"
            puts "time:"
            puts cv.time
            puts "data:"
            puts cv.data
        end
    end    
end
