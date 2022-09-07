cmake_minimum_required(VERSION 3.15)

add_executable(test test.cpp)

target_link_libraries(test Threads::Threads FileSignature)

if(Boost_FOUND AND WITH_BOOST)
    message("Building tests with BOOST!")
    target_link_libraries(test ${Boost_SYSTEM_LIBRARY})
endif()

install(TARGETS test DESTINATION bin)
install(FILES randomFile DESTINATION bin)