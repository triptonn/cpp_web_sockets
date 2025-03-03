cmake_minimum_required(VERSION 3.10)
project(Server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(core
    core/logger.cpp
    core/http.cpp
    core/string_utils.cpp
)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /workspaces/web_sockets/build/server/)
add_executable(server
    server/myserver.cpp
)
target_link_libraries(server core)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /workspaces/web_sockets/build/client/)
add_executable(client
    client/myclient.cpp
)

find_package(Catch2 3 REQUIRED)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /workspaces/web_sockets/build/tests/)
add_executable(http_tests
    tests/http_tests.cpp
)
target_link_libraries(http_tests PRIVATE core Catch2::Catch2)
