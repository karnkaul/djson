cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(dj-test)

if(NOT TARGET djson)
  find_package(djson CONFIG REQUIRED)
endif()

add_library(dj-test)
add_library(dj::test ALIAS dj-test)
target_link_libraries(dj-test PUBLIC djson::djson)
target_include_directories(dj-test PUBLIC .)
target_sources(dj-test PRIVATE test/test.cpp test/test.hpp)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(dj-test PUBLIC
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type -Wunused
  )
endif()

add_executable(dj-test-core)
target_sources(dj-test-core PRIVATE core.cpp)
target_link_libraries(dj-test-core PRIVATE dj::test)

add_executable(dj-test-marshall)
target_sources(dj-test-marshall PRIVATE marshall.cpp)
target_link_libraries(dj-test-marshall PRIVATE dj::test)

add_executable(dj-test-read)
target_sources(dj-test-read PRIVATE read.cpp)
target_link_libraries(dj-test-read PRIVATE dj::test)

add_test(djson-core dj-test-core)
add_test(djson-marshall dj-test-marshall)
add_test(djson-read dj-test-read WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
