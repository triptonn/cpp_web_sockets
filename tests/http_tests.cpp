// Copyright [2025] <Nicolas Selig>
//
//

#define CATCH_CONFIG_MAIN
#include "../core/http.hpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

TEST_CASE("HTTP Request Parsing - Basic GET", "[http]") {
    std::string sample_get_request = "GET /index.html HTTP/1.1\r\n"
                                     "Host: example.com\r\n"
                                     "User-Agent: Mozilla/5.0\r\n"
                                     "Accept: text/html\r\n"
                                     "Connection: keep-alive\r\n"
                                     "\r\n";

    HttpRequest request = HttpRequest::parse(sample_get_request);
    REQUIRE(request.method == "GET");
    REQUIRE(request.path == "/index.html");
    REQUIRE(request.version == "HTTP/1.1");
    REQUIRE(request.headers.size() == 4);
    REQUIRE(request.has_header("host"));
    REQUIRE(request.get_header("Host") == "example.com");
    REQUIRE(request.body.empty());
}

TEST_CASE("HTTP Request Parsing - Basic POST", "[http]") {
    std::string sample_post_request =
        "POST /submit-form HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=john&password=pass";

    HttpRequest request = HttpRequest::parse(sample_post_request);

    REQUIRE(request.method == "POST");
    REQUIRE(request.path == "/submit-form");
    REQUIRE(request.headers.size() == 3);
    REQUIRE(request.get_header("content-type") ==
            "application/x-www-form-urlencoded");
    REQUIRE(request.body == "username=john&password=pass\n");
}

TEST_CASE("HTTP Request Parsing - Edge Cases", "[http]") {
    SECTION("Empty request") {
        HttpRequest request = HttpRequest::parse("");
        REQUIRE(request.method.empty());
        REQUIRE(request.headers.empty());
    }

    SECTION("Malformed request line") {
        HttpRequest request = HttpRequest::parse("GET /index.html\r\n\r\n");
        REQUIRE(request.method == "GET");
        REQUIRE(request.path == "/index.html");
        REQUIRE(request.version.empty());
    }

    SECTION("Headers with no values") {
        HttpRequest request = HttpRequest::parse("GET / HTTP/1.1\r\n"
                                                 "Empty-Header:\r\n"
                                                 "\r\n");
        REQUIRE(request.has_header("empty-header"));
        REQUIRE(request.get_header("empty-header").empty());
    }

    SECTION("Case insensitivity") {
        HttpRequest request = HttpRequest::parse("GET / HTTP/1.1\r\n"
                                                 "Content-Type: text/html\r\n"
                                                 "\r\n");
        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.has_header("Content-Type"));
        REQUIRE(request.has_header("CONTENT-TYPE"));
    }
}

TEST_CASE("HTTP Response - Basic Construction", "[http]") {
    HttpResponse response;
    REQUIRE(response.status_code == 200);
    REQUIRE(response.status_text == "OK");
    REQUIRE(response.version == "HTTP/1.1");

    HttpResponse custom_response(404, "Not Found");
    REQUIRE(custom_response.status_code == 404);
    REQUIRE(custom_response.status_text == "Not Found");
}

TEST_CASE("HTTP Response - Headers Management", "[http]") {
    HttpResponse response;

    response.set_header("Content-Type", "text/html");
    response.set_header("Server", "MyServer/1.0");

    REQUIRE(response.headers.size() == 2);
    REQUIRE(response.has_header("content-type"));
    REQUIRE(response.get_header("Content-Type") == "text/html");

    REQUIRE(response.has_header("CONTENT-TYPE"));
    REQUIRE(response.get_header("content-TYPE") == "text/html");

    response.set_header("Content-Type", "application/json");
    REQUIRE(response.get_header("Content-Type") == "application/json");

    REQUIRE_FALSE(response.has_header("X-Custom"));
    REQUIRE(response.get_header("X-Custom") == "");
}

TEST_CASE("HTTP Response - Factory Methods", "[http]") {
    SECTION("OK Response") {
        HttpResponse ok = HttpResponse::ok("Everything is fine");
        REQUIRE(ok.status_code == 200);
        REQUIRE(ok.body == "Everything is fine");
    }

    SECTION("Not Found Response") {
        HttpResponse not_found = HttpResponse::not_found("/missing.html");
        REQUIRE(not_found.status_code == 404);
        REQUIRE(not_found.body.find("/missing.html") != std::string::npos);
    }

    SECTION("Server Error Response") {
        HttpResponse error =
            HttpResponse::server_error("Database connection failed");
        REQUIRE(error.status_code == 500);
        REQUIRE(error.body.find("Database connection failed") !=
                std::string::npos);
    }

    SECTION("Bad Request Response") {
        HttpResponse bad = HttpResponse::bad_request("Invalid parameter");
        REQUIRE(bad.status_code == 400);
        REQUIRE(bad.body.find("Invalid parameter") != std::string::npos);
    }
}

TEST_CASE("HTTP Response - Edge Cases", "[http]") {
    SECTION("Empty body") {
        HttpResponse response;
        std::string result = response.to_string();
        REQUIRE(result.find("Content-Length: 0") != std::string::npos);
    }

    SECTION("Non-standard status code") {
        HttpResponse response(418, "I'm a teapot");
        REQUIRE(response.status_code == 418);
        REQUIRE(response.status_text == "I'm a teapot");
    }

    SECTION("Headers with special characters") {
        HttpResponse response;
        response.set_header("X-Custom", "Value with spaces and symbols: !Q#$%");
        REQUIRE(response.get_header("X-Custom") ==
                "Value with spaces and symbols: !Q#$%");
    }
}
