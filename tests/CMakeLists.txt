project(djson-test)

add_library(${PROJECT_NAME})
add_library(djson::test ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
  ../lib/src
  harness
)

target_sources(${PROJECT_NAME} PRIVATE
  harness/unit_test.hpp
  harness/unit_test.cpp
)

add_executable(${PROJECT_NAME}-main)

target_link_libraries(${PROJECT_NAME}-main PRIVATE
  djson::djson
  djson::test
)

file(GLOB_RECURSE sources LIST_DIRECTORIES false "*.cpp")
target_sources(${PROJECT_NAME}-main PRIVATE
  harness/unit_test_main.cpp
  ${sources}
)

add_test(NAME ${PROJECT_NAME}
  COMMAND ${PROJECT_NAME}-main
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
