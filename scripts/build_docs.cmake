include(version.cmake)

configure_file(Doxyfile.in "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile" @ONLY)

execute_process(COMMAND
  doxygen Doxyfile
  COMMAND_ERROR_IS_FATAL ANY
)
