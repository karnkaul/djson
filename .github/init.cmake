cmake_minimum_required(VERSION 4.3)

if(CMAKE_HOST_UNIX)
  execute_process(COMMAND
    ".github/init_ubuntu_26.04.sh"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()
