cmake_minimum_required(VERSION 3.17)

project(djson-test)

if(NOT TARGET djson)
  find_package(djson REQUIRED)
endif()

add_executable(djson-test)
target_link_libraries(djson-test PRIVATE djson::djson)
target_sources(djson-test PRIVATE djson_test.cpp)

enable_testing()
add_test(djson-test djson-test)
