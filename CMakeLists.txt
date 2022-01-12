cmake_minimum_required(VERSION 3.14)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

set(project_name djson)
set(${project_name}_version 1.0.0)
set(cxx_standard cxx_std_17)

project(${project_name} VERSION ${${project_name}_version})
set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

# options
option(DJSON_INSTALL "Install djson" ${is_root_project})

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC ${cxx_standard})
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_include_directories(${PROJECT_NAME} PRIVATE include src)

target_sources(${PROJECT_NAME} PRIVATE
  include/dumb_json/json_type.hpp
  include/dumb_json/json.hpp
  include/dumb_json/serial_opts.hpp
)
add_subdirectory(src)
get_target_property(sources ${PROJECT_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wextra -Wall -Werror=return-type $<$<NOT:$<CONFIG:Debug>>:-Werror>)
endif()

if(DJSON_INSTALL)
  # cmake-utils
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    cmake-utils
    GIT_REPOSITORY https://github.com/karnkaul/cmake-utils
    GIT_TAG v1.0.1
  )
  FetchContent_MakeAvailable(cmake-utils)
  FetchContent_GetProperties(cmake-utils)

  include("${cmake-utils_SOURCE_DIR}/cmake-utils.cmake")
  install_and_export_target(TARGET ${PROJECT_NAME})
endif()
