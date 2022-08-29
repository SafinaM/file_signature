cmake_minimum_required(VERSION 3.15)
project(threads)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(SOURCES thread_pool.cpp Hasher.cpp ArgParser.cpp FileReader.cpp FileWriter.cpp)

add_executable(thread_pool ${SOURCES})

include_directories(../include)

target_link_libraries(thread_pool Threads::Threads)
