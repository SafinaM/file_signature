cmake_minimum_required(VERSION 3.15)
project(FileSignature)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_INSTALL_PREFIX install)

set(SOURCES ChunkProcessor.cpp Hasher.cpp ArgParser.cpp FileReader.cpp FileWriter.cpp)

add_executable(file_signature ${SOURCES} main.cpp)

include_directories(../include)

target_link_libraries(file_signature Threads::Threads)

if(WITH_TESTS)

    add_library(FileSignature STATIC ${SOURCES})
    add_subdirectory(tests)

    install(DIRECTORY ../include DESTINATION include)
    install(TARGETS FileSignature DESTINATION lib)

endif()

install(TARGETS file_signature DESTINATION bin)