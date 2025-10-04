if("${PRESET}" STREQUAL "")
  message(FATAL_ERROR "Invalid PRESET: ${PRESET}")
endif()

if("${BUILD_DIR}" STREQUAL "")
  message(FATAL_ERROR "Invalid BUILD_DIR: ${BUILD_DIR}")
endif()

message(STATUS "djson install test")

execute_process(
  COMMAND ${CMAKE_COMMAND} --install ${BUILD_DIR} --config=Debug --prefix=install_test/packages
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${CMAKE_COMMAND} --install ${BUILD_DIR} --config=Release --prefix=install_test/packages
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND cp -f CMakePresets.json install_test/exe
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${CMAKE_COMMAND} -S install_test/exe --preset=${PRESET} -B=install_test/build -DCMAKE_INSTALL_PREFIX=install_test/packages
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)

execute_process(
  COMMAND ${CMAKE_COMMAND} --build install_test/build --config=Debug
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${CMAKE_COMMAND} --build install_test/build --config=Release
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND install_test/build/Debug/exe
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND install_test/build/Release/exe
  COMMAND_ERROR_IS_FATAL ANY
)
