set(version_module src/build_version.cppm)
configure_file(private/build_version.cppm.in ${version_module} @ONLY)

add_library(${PROJECT_NAME})
add_library(djson::djson ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES
  BASE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"

  FILES
  public/types.cppm
  public/djson.cppm
  private/detail.cppm
  "${CMAKE_CURRENT_BINARY_DIR}/${version_module}"
)

target_sources(${PROJECT_NAME} PRIVATE
  src/types.cpp
  src/detail.cpp
  src/djson.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<BOOL:${MINGW}>:stdc++exp>
)
