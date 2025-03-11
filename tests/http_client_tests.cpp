// Copyright [2025] <Nicolas Selig>
//
//

#define CATCH_CONFIG_MAIN
#include "../core/http.hpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

TEST_CASE("HttpClient - Constructor and Hostname Resolution", "[client]") {
    SECTION("Valid hostname resolution - localhost") {
        REQUIRE_NOTHROW(HttpClient("localhost", 8080));
    }

    SECTION("Valid hostname resolution - actual domain") {
        REQUIRE_NOTHROW(HttpClient("google.com", 80));
    }

    SECTION("Invalid hostname") {
        REQUIRE_THROWS_AS(HttpClient("invalid.nonexistent.domain", 8080),
                          std::runtime_error);
    }
}

TEST_CASE("HttpClient - Request Creation and Formatting", "[client]") {
    HttpClient client("localhost", 8080);

    SECTION("Create and format GET request") {
        HttpRequest request;
        request.create_get("/test");

        std::string formatted = request.to_string();
        REQUIRE(formatted.find("GET /test HTTP/1.1") == 0);
        REQUIRE(formatted.find("Host: localhost\r\n") != std::string::npos);
    }

    SECTION("Create and format POST request with form data") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {{"username", "test"},
                                                        {"password", "123"}};
        request.create_post("/login", form_data);

        std::string formatted = request.to_string();
        REQUIRE(formatted.find("POST /login HTTP/1.1") == 0);
        REQUIRE(formatted.find(
                    "Content-Type: application/x-www-form-urlencoded\r\n") !=
                std::string::npos);
        REQUIRE(((formatted.find("username=test&password=123") !=
                  std::string::npos) ||
                 (formatted.find("password=123&username=test") !=
                  std::string::npos)));
    }

    SECTION("Create and format POST request with JSON") {
        HttpRequest request;
        std::string json_data = R"({"name": "test", "age": 25})";
        request.create_post("/api/user", json_data, "application/json");

        std::string formatted = request.to_string();
        REQUIRE(formatted.find("POST /api/user HTTP/1.1\r\n") == 0);
        REQUIRE(formatted.find("Content-Type: application/json") !=
                std::string::npos);
        REQUIRE(formatted.find(json_data) != std::string::npos);
    }
}

TEST_CASE("HttpClient - Response Parsing", "[client]") {
    SECTION("Parse successful response") {
        std::string raw_response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Lenght: 21\r\n"
                                   "\r\n"
                                   "{\"status\": \"success\"}\r\n";

        HttpResponse response = HttpResponse::parse(raw_response);
        REQUIRE(response.status_code == 200);
        REQUIRE(response.reason_phrase == "OK");
        REQUIRE(response.get_header("content-type") == "application/json");
        REQUIRE(response.body == "{\"status\": \"success\"}\r\n");
    }

    SECTION("Parse error response") {
        std::string raw_response = "HTTP/1.1 404 Not found\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 20\r\n"
                                   "\r\n"
                                   "Resource not found\r\n";

        HttpResponse response = HttpResponse::parse(raw_response);
        REQUIRE(response.status_code == 404);
        REQUIRE(response.reason_phrase == "Not found");
        REQUIRE(response.get_header("content-type") == "text/plain");
        REQUIRE(response.get_header("content-length") == "20");
        REQUIRE(response.body == "Resource not found\r\n");
    }
}

/* TEST_CASE("Client Request-Response Functionality", "[client]") {
    SECTION("Send GET request and receive response") {
        HttpClient client("localhost", 8080);
        REQUIRE_NOTHROW(client.connect_to_server());

        HttpRequest request;
        request.create_get("/test");
        auto response = client.send_request(request);

        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("content-type") == "text/plain");

        client.disconnect();
    }

    SECTION("POST request with form data") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {{"name", "test"},
                                                        {"value", "123"}};
        request.create_post("/api/data", form_data);

        REQUIRE_NOTHROW(client.connect_to_server());
        auto response = client.send_request(request);

        REQUIRE(response.status_code == 200);
    }

    SECTION("Request to non-existent endpoint") {
        HttpRequest request;
        request.create_get("/nonexistent");

        REQUIRE_NOTHROW(client.connect_to_server());
        auto response = client.send_request(request);

        REQUIRE(response.status_code == 404);
    }
} */

/* TEST_CASE("HttpClient - Resource Management", "[client]") {
    SECTION("Proper cleanup on destruction") {
        {
            HttpClient client("localhost", 8080);
            client.connect_to_server();
        }
        REQUIRE_NOTHROW(HttpClient("localhost", 8080));
    }
}

TEST_CASE("HttpClient - Error Handling", "[client]") {
    SECTION("Send request without connection") {
        HttpClient client("localhost", 8080);
        HttpRequest request;
        request.create_get("/test");

        REQUIRE_THROWS(client.send_request(request));
    }

    SECTION("Reconnection after server disonnect") {
        HttpClient client("localhost", 8080);
        client.connect_to_server();
        client.disconnect();

        REQUIRE_NOTHROW(client.connect_to_server());
    }
} */
