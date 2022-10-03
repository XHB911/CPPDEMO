cmake_minimum_required(VERSION 2.6)
project(leptjson_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g ")

add_library(leptjson leptjson.cpp)
add_executable(leptjson_test test.cpp)
target_link_libraries(leptjson_test leptjson)
