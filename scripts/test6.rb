require 'readline'
require 'orocos'
include Orocos
Orocos.initialize

ENV['BASE_LOG_LEVEL'] = 'DEBUG'
ENV['BASE_LOG_FORMAT'] = 'SHORT'
ENV['ORO_LOGLEVEL'] = '5'

# Testing the debug port creation

#Orocos.run 'general_processing_test', 'valgrind' => true do |p|
Orocos.run 'general_processing_test' do |p|

    task = TaskContext.get 'TestBaseTask'

    task.debug_conversion = true
    task.create_places = false
    task.aggregator_max_latency = 0.01

    task.createInputPort("rbs1","/base/samples/RigidBodyState","position orientation",0)

    task.createInputPort("mc2d","/base/MotionCommand2D","",0)

    task.createInputPort("rbs2","/base/samples/RigidBodyState","orientation",2)
    
    task.createInputPort("laser1","/base/samples/LaserScan",
                         "minRange maxRange ranges.[1-100:20]",2)

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
    w_rbs2 = task.rbs2.writer
    w_ls1 = task.laser1.writer
    r_d0 = task.debug_0.reader
    r_d2 = task.debug_2.reader

    Readline.readline "Press enter to start task." do
    end

    task.configure
    task.start

    rbs = w_rbs.new_sample
    rbs.position = Eigen::Vector3.new(1.0,2.0,3.0)
    rbs.orientation = Eigen::Quaternion.new(0.9,-0.1,-0.2,-0.3)
    rbs.time = Time.now

    ls1 = w_ls1.new_sample
    ls1.time = Time.now
    ls1.minRange = 2
    ls1.maxRange = 200
    for v in 4..140 do
        ls1.ranges << v
    end

    mc2d = w_mc2d.new_sample
    mc2d.translation = 10.0
    mc2d.rotation = -1.0
    
    rbs2 = w_rbs2.new_sample
    rbs2.position = Eigen::Vector3.new(11.0,-2.0,3.0)
    rbs2.cov_position.data = [0.4,0.0,0.0,0.0,0.4,0.0,0.0,0.0,0.4]
    rbs2.velocity = Eigen::Vector3.new(-0.1,0.1,0.2)
    rbs2.cov_velocity.data = [0.5,0.0,0.0,0.0,0.5,0.0,0.0,0.0,0.5]
    rbs2.time = rbs.time
    rbs2.orientation = Eigen::Quaternion.new(1.0,0.0,0.0,0.0)
    rbs2.cov_orientation.data = [0.6,0.0,0.0,0.0,0.6,0.0,0.0,0.0,0.6]
    rbs2.angular_velocity = Eigen::Vector3.new(10.0,20.0,30.0)
    rbs2.cov_angular_velocity.data = [0.7,0.0,0.0,0.0,0.7,0.0,0.0,0.0,0.7]

    w_rbs.write(rbs)
    w_ls1.write(ls1)
    w_mc2d.write(mc2d)
    w_rbs2.write(rbs2)

    Readline.readline "Press enter to read data" do
    end

    if cv = r_d0.read()
        puts "--- debug 0 ---"
        puts "time:"
        puts cv.time
        puts "data:"
        puts cv.data
        puts "places:"
        puts cv.places
    end
    
    if cv = r_d2.read()
        puts "--- debug 2 ---"
        puts "time:"
        puts cv.time
        puts "data:"
        puts cv.data
        puts "places:"
        puts cv.places
    end

    Readline.readline "Press enter to quit" do
    end
end
