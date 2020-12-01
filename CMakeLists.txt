cmake_minimum_required(VERSION 3.14)

project(li)

set(SOURCE_FILES src/l1.cpp src/main.cpp)
set(CMAKE_CXX_STANDARD 20)

add_executable(li ${SOURCE_FILES})