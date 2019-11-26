add_executable(raw_decoder raw_decoder.cxx)
target_link_libraries(raw_decoder TOFdataRaw ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS raw_decoder RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(raw_checker raw_checker.cxx)
target_link_libraries(raw_checker TOFdataRaw ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS raw_checker RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(raw_adder raw_adder.cxx)
target_link_libraries(raw_adder ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS raw_adder RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(raw_stripper raw_stripper.cxx)
target_link_libraries(raw_stripper ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS raw_stripper RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(raw_mem raw_mem.cxx)
target_link_libraries(raw_mem ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS raw_mem RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(compressed_encoder compressed_encoder.cxx)
target_link_libraries(compressed_encoder TOFdataRaw TOFdataCompressed ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS compressed_encoder RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(compressed_decoder compressed_decoder.cxx)
target_link_libraries(compressed_decoder TOFdataCompressed ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS compressed_decoder RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)


if(HISTOGRAMMER)

find_package(ROOT)
include_directories(${ROOT_INCLUDE_DIRS})

add_executable(raw_histogrammer raw_histogrammer.cxx)
target_link_libraries(raw_histogrammer TOFdataRaw ${Boost_PROGRAM_OPTIONS_LIBRARY} ${ROOT_LIBRARIES})
install(TARGETS raw_histogrammer RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

add_executable(compressed_histogrammer compressed_histogrammer.cxx)
target_link_libraries(compressed_histogrammer TOFdataCompressed ${Boost_PROGRAM_OPTIONS_LIBRARY} ${ROOT_LIBRARIES})
install(TARGETS compressed_histogrammer RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)

endif()
