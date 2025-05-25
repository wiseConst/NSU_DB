cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

include(FetchContent)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

project(app LANGUAGES CXX)

add_subdirectory(app)