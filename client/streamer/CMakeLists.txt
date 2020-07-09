cmake_minimum_required(VERSION 2.8.3)
project(ai_sail_stream_receiver)

find_package(catkin REQUIRED COMPONENTS
  rospy
  sensor_msgs
)

catkin_python_setup()
