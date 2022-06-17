target_sources(${PROJECT_NAME} PRIVATE
  detail/parser.hpp
  detail/scanner.hpp
  json.cpp
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(has_from_chars
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/detail/fc_test.cpp"
)

if(${has_from_chars})
  target_compile_definitions(${PROJECT_NAME} PUBLIC DJSON_HAS_FROM_CHARS_FLOAT)
endif()

configure_file(djson_version.hpp.in include/djson/djson_version.hpp)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")
target_sources(${PROJECT_NAME} PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/djson/djson_version.hpp>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/djson/djson_version.hpp>
)

if(DJSON_INSTALL)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/djson/djson_version.hpp" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/djson")
endif()
