set(SOURCES Decoder.cxx)
	
add_library(TOFdataRaw SHARED ${SOURCES})
target_link_libraries(TOFdataRaw)
install(TARGETS TOFdataRaw LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/lib)
