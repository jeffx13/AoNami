cmake_minimum_required(VERSION 3.16)

project(Plugin1 VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Quick) # Adjust based on the actual Qt modules used by the plugin

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the plugin library
add_library(Plugin1 SHARED
    nivod.h
    nivod.cpp
)

target_link_libraries(Plugin1 PRIVATE Qt6::Core)

