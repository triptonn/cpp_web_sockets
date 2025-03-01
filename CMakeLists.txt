cmake_minimum_required(VERSION 3.10)
project(Server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(core
    core/logger.cpp
)

add_executable(server
    server/myserver.cpp
)

target_link_libraries(server core)

add_executable(client
    client/myclient.cpp
)
