include_directories(${CMAKE_CURRENT_SOURCE_DIR}/. ${Boost_INCLUDE_DIR})

add_subdirectory(Raw)
add_subdirectory(Compressed)
add_subdirectory(Utils)
