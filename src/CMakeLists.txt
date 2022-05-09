target_sources(${PROJECT_NAME} PRIVATE
  detail/parser.hpp
  detail/scanner.hpp
  json.cpp
)

configure_file(djson_version.hpp.in include/djson/djson_version.hpp)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")
target_sources(${PROJECT_NAME} PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/djson/djson_version.hpp>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/djson/djson_version.hpp>
)

if(DJSON_INSTALL)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/djson/djson_version.hpp" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/djson)
endif()
