cmake_minimum_required(VERSION 3.18)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_DEBUG_POSTFIX "-d")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(build_version 2.1.2)

project(djson VERSION ${build_version})

set(is_root_project OFF)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
endif()

option(DJ_BUILD_TEST "Build djson tests" ${is_root_project})
option(DJ_INSTALL "Setup install and package steps" ${is_root_project})

include(GNUInstallDirs)
include(CheckPIESupported)

check_pie_supported()

configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp" @ONLY)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_sources(${PROJECT_NAME} PRIVATE
  include/djson/json.hpp

  src/compat/api.hpp

  src/detail/loc.hpp
  src/detail/scanner.hpp
  src/detail/token.hpp

  src/json.cpp
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(has_from_chars_d "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/str_to_d_fc.cpp")
try_compile(has_make_unique_for_overwrite "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/make_char_buf_mufo.cpp")
target_sources(${PROJECT_NAME} PRIVATE
  $<IF:$<BOOL:${has_from_chars_d}>,src/compat/str_to_d_fc.cpp,src/compat/str_to_d_stod.cpp>
  $<IF:$<BOOL:${has_make_unique_for_overwrite}>,src/compat/make_char_buf_mufo.cpp,src/compat/make_char_buf_mu.cpp>
)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type -Wunused
  )
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

if(DJ_BUILD_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()

if(DJ_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # install targets
  install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # install headers from include
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )

  # install generated headers
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )

  # install and export targets
  install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  # configure ${PROJECT_NAME}-config.cmake
  configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  # install ${PROJECT_NAME}-config.cmake
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  # configure ${PROJECT_NAME}-config-version.cmake
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${build_version}
    COMPATIBILITY AnyNewerVersion
  )

  # install ${PROJECT_NAME}-config-version.cmake
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
endif()
