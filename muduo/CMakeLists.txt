cmake_minimum_required(VERSION 2.5)
project(muduocpp11)

# 最终编译为动态库，设置动态库路径，放在项目目录下的 lib  目录中
set(LIBRARY_OUTPUT_DIRECTORY  ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

aux_source_directory(. SRC_LIST)
add_library(muduocpp11 SHARED ${SRC_LIST})
