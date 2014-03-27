include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(test_aziomq main.cpp)
target_link_libraries(test_aziomq ${Boost_LIBRARIES}
                                  ${ZeroMQ_LIBRARIES}
                                  aziomq)

add_test(basic_socket_functionality ${CMAKE_CURRENT_BINARY_DIR}/test_aziomq)


