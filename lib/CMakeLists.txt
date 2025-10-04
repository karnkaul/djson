set(version_file "${CMAKE_CURRENT_BINARY_DIR}/include/djson/build_version.hpp")
configure_file(src/build_version.hpp.in "${version_file}" @ONLY)

add_library(${PROJECT_NAME})
add_library(djson::djson ALIAS ${PROJECT_NAME})

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/djson/*.hpp")
target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS include
  FILES ${headers}
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include"
  FILES "${version_file}"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
)

file(GLOB_RECURSE sources LIST_DIRECTORIES false "src/*.[hc]pp")
target_sources(${PROJECT_NAME} PRIVATE
  ${sources}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<BOOL:${MINGW}>:stdc++exp>
)
