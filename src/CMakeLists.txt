project(TOFdata)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(SET CMP0042 NEW)

find_package(Boost REQUIRED)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/. ${Boost_INCLUDE_DIR})

add_subdirectory(Raw)
add_subdirectory(Compressed)
add_subdirectory(Utils)
