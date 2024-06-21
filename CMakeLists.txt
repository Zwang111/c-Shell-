cmake_minimum_required(VERSION 3.5)

project(shell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(shell main.cpp
)
configure_file(${CMAKE_SOURCE_DIR}/script0 ${CMAKE_BINARY_DIR}/script0)

install(TARGETS shell
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
