cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(exe)

add_executable(${PROJECT_NAME})

find_package(djson CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE djson::djson)

target_sources(${PROJECT_NAME} PRIVATE main.cpp)
