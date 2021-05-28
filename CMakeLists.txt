cmake_minimum_required(VERSION 3.3)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

set(DJSON_VERSION 0.1.0.0 CACHE STRING "DJSON version" FORCE)
project(djson VERSION ${DJSON_VERSION})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${INCLUDES} ${SOURCES})

add_library(djson ${INCLUDES} ${SOURCES})
add_library(djson::djson ALIAS djson)
target_compile_features(djson PUBLIC cxx_std_17)
target_include_directories(djson PUBLIC include PRIVATE src)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(djson PRIVATE -Wextra -Wall -Werror=return-type $<$<NOT:$<CONFIG:Debug>>:-Werror>)
endif()
