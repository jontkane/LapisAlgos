cmake_minimum_required(VERSION 3.24)
project(LapisAlgos VERSION 0.1)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

include(LapisAlgos.cmake)
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
  include(LapisAlgosTest.cmake)
endif()