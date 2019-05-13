set(SOURCES Encoder.cxx Decoder.cxx)
	
add_library(TOFdataCompressed SHARED ${SOURCES})
target_link_libraries(TOFdataCompressed)
install(TARGETS TOFdataCompressed LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/lib)
