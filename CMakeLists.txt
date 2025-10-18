# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(nsh LANGUAGES CXX)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-ggdb -fsanitize=address")


set (SRCS
    src/main.cpp
)


add_executable(${CMAKE_PROJECT_NAME} ${SRCS}
    include/tokens.hpp
    include/command_struct.hpp
    include/parse_input.hpp
    include/word_control.hpp
    src/execution/command_execution.cpp include/execution/command_execution.hpp
    include/builtin.hpp
    README.md
    include/system_envs.hpp
    include/c_array.hpp
    include/execution/job_control.hpp src/execution/job_control.cpp
    include/execution/internal/job_control_impl.hpp)
