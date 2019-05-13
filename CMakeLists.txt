project(TOFdata)
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
    set(CMAKE_MACOSX_RPATH 0)
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_MACOSX_RPATH 1)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(src)
