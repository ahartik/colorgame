cmake_minimum_required(VERSION 2.6)

project(colorgame_ai)

add_definitions("-Wall -O3 ")

set(sources ColorAI.cpp ColorBoard.cpp)

add_executable(colorgame ${sources})
