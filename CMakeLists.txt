cmake_minimum_required(VERSION 3.1)

project(hash_table_main
    VERSION 1.0
    DESCRIPTION "Sockets"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Lista de archivos fuente
set(SOURCES
    src/ClientConnection.cpp
    src/ftp_server.cpp
    src/FTPServer.cpp)

# Agregar el ejecutable
add_executable(OUT ${SOURCES})

# Enlazar con la biblioteca pthread
target_link_libraries(OUT pthread)
