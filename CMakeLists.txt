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

add_executable(http_request_tests
    tests/http_request_tests.cpp
)

add_executable(http_response_tests
    tests/http_response_tests.cpp
)

add_executable(http_server_tests
    tests/http_server_tests.cpp
)

add_executable(http_client_tests
    tests/http_client_tests.cpp
)

target_link_libraries(http_request_tests PRIVATE core Catch2::Catch2)
target_link_libraries(http_response_tests PRIVATE core Catch2::Catch2)
target_link_libraries(http_server_tests PRIVATE core Catch2::Catch2)
target_link_libraries(http_client_tests PRIVATE core Catch2::Catch2)

add_custom_target(run_all_tests
    COMMAND http_request_tests
    COMMAND http_response_tests
    COMMAND http_server_tests
    COMMAND http_client_tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
