add_library(aziomq SHARED io_service.cpp error.cpp)

target_link_libraries(aziomq ${Boost_LIBRARIES}
                             ${ZeroMQ_LIBRARIES})

install(TARGETS aziomq LIBRARY DESTINATION lib)
