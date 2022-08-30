cmake_minimum_required(VERSION 3.15)

add_executable(test test.cpp)

target_link_libraries(test Threads::Threads FileSignature)

install(TARGETS test DESTINATION bin)
install(FILES randomFile DESTINATION bin)