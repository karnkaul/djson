cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

include(version.cmake)

project(djson VERSION ${djson_version})

option(DJ_BUILD_TESTS "Build djson tests" ${PROJECT_IS_TOP_LEVEL})

configure_file(Doxyfile.in Doxyfile @ONLY)

add_subdirectory(lib)

if(DJ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
