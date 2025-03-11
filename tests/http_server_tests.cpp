// Copyright [2025] <Nicolas Selig>
//
//

#define CATCH_CONFIG_MAIN
#include <set>
#include <stdexcept>
#include "../core/http.hpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

TEST_CASE("HttpServer - Basic Setup", "[server]") {
    SECTION("Server initialization with valid port") {
        REQUIRE_NOTHROW(HttpServer(8080));
    }

    SECTION("Server initialization with invalid port") {
        REQUIRE_THROWS_AS(HttpServer(80), std::runtime_error);
    }

    SECTION("Server initialization with already in use port") {
        HttpServer blocking_server(8081);
        REQUIRE_THROWS_AS(HttpServer(8081), std::runtime_error);
    }
}

TEST_CASE("HttpServer - File Descriptor Management",
          "[server][fd_management]") {
    SECTION("File Descriptor Set Management") {
        FdSet fd_set;
        int test_fd = 3;

        fd_set.add(test_fd);
        REQUIRE(fd_set.is_set(test_fd));

        fd_set.remove(test_fd);
        REQUIRE_FALSE(fd_set.is_set(test_fd));
    }

    SECTION("Multiple FD Management") {
        FdSet fd_set;
        fd_set.add(3);
        fd_set.add(5);
        fd_set.add(7);

        REQUIRE(fd_set.get_max_fd() == 7);

        fd_set.remove(7);
        REQUIRE(fd_set.get_max_fd() == 5);
    }

    SECTION("Clear operation") {
        FdSet fd_set;
        fd_set.add(3);
        fd_set.add(5);

        fd_set.clear();
        REQUIRE_FALSE(fd_set.is_set(3));
        REQUIRE_FALSE(fd_set.is_set(5));
        REQUIRE(fd_set.get_max_fd() == 0);
    }
}

TEST_CASE("HttpServer - Component Organisation", "[server][constructors]") {
    SECTION("Client Session Management") {
        ClientSession session(5, "127.0.0.1");
        REQUIRE(session.get_fd() == 5);
        REQUIRE(session.get_ip() == "127.0.0.1");
        REQUIRE(session.is_active());
    }

    SECTION("Server Event Loop Components") {
        ServerEventLoop loop;

        ServerEventLoop::Event connect_event{
            ServerEventLoop::EventType::NEW_CONNECTION, 5, ""};

        REQUIRE(loop.process_event(connect_event));

        ServerEventLoop::Event data_event{
            ServerEventLoop::EventType::CLIENT_DATA, 5, "test data"};
        REQUIRE(loop.process_event(data_event));

        ServerEventLoop::Event command_event{
            ServerEventLoop::EventType::SERVER_COMMAND, STDIN_FILENO, "quit"};
        REQUIRE(loop.process_event(command_event));
    }
}

TEST_CASE("HttpServer - Socket Resource Management", "[server][resource_management]") {
    SECTION("Socket Resource Management") {
        int test_fd = socket(AF_INET, SOCK_STREAM, 0);
        {
            SocketGuard guard(test_fd);
            REQUIRE(guard.get() == test_fd);
        }
    }
}

TEST_CASE("HttpServer - Client Collection Management", "[server][client_management]") {
    SECTION("Client Collection Management") {
        ClientManager manager;
        manager.add_client(std::make_unique<ClientSession>(5, "127.0.0.1"));
        REQUIRE(manager.has_client(5));
        REQUIRE(manager.client_count() == 1);
    }
}

TEST_CASE("HttpServer - Connection Management", "[server][connection_management]") {
    SECTION("Starting up server") {
        HttpServer server(8082);
        REQUIRE_NOTHROW(server.start());
        server.stop();
    }

    HttpServer server(8083);
    server.start();

    SECTION("Accept new connection") {
        HttpClient client("localhost", 8083);
        REQUIRE(client.connect_to_server() == 0);
    }

    SECTION("Handle multiple connections") {
        HttpClient second_client("localhost", 8083);
        REQUIRE(second_client.connect_to_server() == 0);
    }

    SECTION("Stopping running server") {
        REQUIRE_NOTHROW(server.stop());
    }
}

TEST_CASE("HttpServer - Route Handling", "[server][route_handling]") {
    HttpServer server(8084);

    SECTION("Register GET route handler") {
        REQUIRE_NOTHROW(server.register_get("/test", [](const HttpRequest &req) -> HttpResponse {
            return HttpResponse::ok("Test response");
        }));
    }

    SECTION("Register POST route handler") {
        REQUIRE_NOTHROW(server.register_post("/api/data", [](const HttpRequest &req) -> HttpResponse {
            if (req.has_header("content-type") &&
                req.get_header("content-type") == "application/json") {
                return HttpResponse::json_response("{\"status\":\"success\"}");
            }
            return HttpResponse::bad_request("Invalid content-type");
        }));
    }

    SECTION("Start server with registered routes") {
        REQUIRE_NOTHROW(server.start());
    }
    server.stop();
}


TEST_CASE("HttpServer - Request Handling", "[server][request_handling]") {
    HttpServer server(8085);

    server.register_get("/test", [](const HttpRequest &req) -> HttpResponse {
        return HttpResponse::ok("GET test response");
    });

    server.register_post("/api/data", [](const HttpRequest &req) -> HttpResponse {
        if (req.has_header("content-type") &&
            req.get_header("content-type") == "application/json") {
            return HttpResponse::json_response("{\"status\":\"success\"}");
        }
        return HttpResponse::bad_request("Invalid content-type");
    });
    
    server.start();

    HttpClient client("localhost", 8085);
    client.connect_to_server();

    HttpRequest test_request;
    test_request.create_get("/test");

    SECTION("Respond to GET request on registered get route") {
        HttpResponse response = client.send_request(test_request);
        REQUIRE(response.status_code == 200);
        REQUIRE(response.body == "GET test response");
    }

    client.disconnect();

    /* SECTION("Register POST route handler") {

        HttpRequest test_request;
        test_request.create_post("/api/data");
        test_request.set_header("Content-Type", "application/json");

        auto response = server.handle_request(test_request);
        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("content-type") == "application/json");
    }

    SECTION("Handle non-existent route") {
        HttpRequest test_request;
        test_request.create_get("/nonextistent");

        auto response = server.handle_request(test_request);
        REQUIRE(response.status_code == 404);
    } */

}

/* TEST_CASE("HttpServer - Response Streaming", "[server]") {
    HttpServer server(8084);

    SECTION("Stream large response") {
        std::string large_data(1014 * 1014, 'X');  // 1MB of data

        server.get(
            "/stream", [&large_data](const HttpRequest &req) -> HttpResponse {
                HttpResponse response;
                response.set_streaming(
                    [&large_data](std::ostream &os) { os << large_data; },
                    large_data.length(), "text/plain");
                return response;
            });

        HttpClient client("localhost", 8084);
        client.connect_to_server();
        HttpRequest request;
        request.create_get("/stream");

        auto response = client.send_request(request);
        REQUIRE(response.body.lenth() == large_data.length());
    }
} */
