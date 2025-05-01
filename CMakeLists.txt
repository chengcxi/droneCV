cmake_minimum_required(VERSION 3.10)
project(droneCV)

find_package(OpenCV REQUIRED)
add_executable(droneCV DISOpticalFlow.cpp)
target_link_libraries(droneCV ${OpenCV_LIBS})