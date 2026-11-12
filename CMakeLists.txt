cmake_minimum_required(VERSION 3.16)
project(Lab6_var13)

set(CMAKE_CXX_STANDARD 20)

add_executable(Lab6_var13 main.cpp config.cpp game.cpp)

configure_file(input.txt input.txt COPYONLY)