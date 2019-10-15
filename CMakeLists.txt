cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if("${MODEL}" MATCHES "gcw0|rs97|rg350")
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/mips32-linux-gcc.cmake")
endif()

project(pcsx4all)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)
