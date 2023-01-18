cmake_minimum_required(VERSION 3.0.0)
project(controller_server VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    udp_client.cpp
    uinput_controller.cpp
)

add_executable(controller_server ${SOURCES})
