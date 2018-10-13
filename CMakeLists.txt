cmake_minimum_required(VERSION 3.7)
project(zs-apc-spdu-ctl)

set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for binaries")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(NetSNMP REQUIRED)
find_package(LowlevelZS REQUIRED)

include(CheckIncludeFileCXX)
check_include_file_cxx("string_view" HAVE_CXXH_STRING_VIEW)
check_include_file_cxx("experimental/string_view" HAVE_CXXH_EX_STRING_VIEW)

foreach(i config.h)
  configure_file("${i}.in" "${PROJECT_BINARY_DIR}/${i}")
endforeach(i)

add_subdirectory(src)
