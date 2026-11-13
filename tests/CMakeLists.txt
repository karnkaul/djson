project(djson-test)

if (NOT TARGET klib::klib-test-main)
  include(FetchContent)
  FetchContent_Declare(klib
    GIT_REPOSITORY https://github.com/karnkaul/klib
    GIT_TAG feature/modules
    GIT_SHALLOW TRUE
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/ext/klib"
  )
  set(KLIB_UNIT_TEST ON)
  FetchContent_MakeAvailable(klib)
endif()

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
  ../lib/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  djson::djson
  klib::klib
  klib::klib-test-main
)

file(GLOB_RECURSE sources LIST_DIRECTORIES false "test_*.cpp")
target_sources(${PROJECT_NAME} PRIVATE
  ${sources}
)

add_test(NAME ${PROJECT_NAME}
  COMMAND ${PROJECT_NAME}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
