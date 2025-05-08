cmake_minimum_required(VERSION 3.10)
project(droneCV)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# add_executable(droneCV DenseRLOFOpticalFlow.cpp)
add_executable(droneCV DISOpticalFlow.cpp)
#add_executable(droneCV DualTVL1OpticalFlow.cpp)
#add_executable(droneCV FarnebackOpticalFlow.cpp)

target_link_libraries(droneCV ${OpenCV_LIBS})