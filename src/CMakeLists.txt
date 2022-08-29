cmake_minimum_required(VERSION 3.15)
project(FileSignature)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(SOURCES ChunkProcessor.cpp Hasher.cpp ArgParser.cpp FileReader.cpp FileWriter.cpp main.cpp )

add_executable(file_signature ${SOURCES})

include_directories(../include)

target_link_libraries(file_signature Threads::Threads)
