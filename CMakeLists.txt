cmake_minimum_required(VERSION 3.3)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

project(djson)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

add_library(djson ${SOURCES})
target_compile_features(djson PUBLIC cxx_std_17)
target_include_directories(djson PUBLIC include)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_options(djson PRIVATE -fPIC)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(djson PRIVATE /MP)
	target_compile_definitions(djson PUBLIC _UNICODE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(djson PRIVATE -Wextra -Wall -Werror=return-type $<$<NOT:$<CONFIG:Debug>>:-Werror>)
endif()
