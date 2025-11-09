# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(nsh LANGUAGES CXX)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (SRCS
    src/main.cpp
    src/execution/command_execution.cpp
    src/execution/job_control.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SRCS}
    README.md

)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "include/")
target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC "-ggdb" "-fsanitize=address" "-fsanitize=undefined" "-Wall" "-Wextra" "-Werror")
target_link_options(${CMAKE_PROJECT_NAME} PUBLIC "-ggdb" "-fsanitize=address" "-fsanitize=undefined" "-Wall" "-Wextra" "-Werror")


