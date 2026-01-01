include(version.cmake)

configure_file(Doxyfile.in "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile" @ONLY)

set(flags "")

if(QUIET)
  set(flags "-q")
endif()

execute_process(COMMAND
  doxygen ${flags} Doxyfile
  COMMAND_ERROR_IS_FATAL ANY
)
