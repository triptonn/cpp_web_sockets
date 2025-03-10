// Copyright [2025] <Nicolas Selig>
//
//

#include <iostream>
#define CATCH_CONFIG_MAIN
#include "../core/http.hpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

/////////////////////////////////
// HTTP Response Creation
/////////////////////////////////

TEST_CASE("HTTP Response - Basic Construction", "[http]") {
    HttpResponse response;
    REQUIRE(response.status_code == 200);
    REQUIRE(response.reason_phrase == "OK");
    REQUIRE(response.version == "HTTP/1.1");

    HttpResponse custom_response(404, "Not Found");
    REQUIRE(custom_response.status_code == 404);
    REQUIRE(custom_response.reason_phrase == "Not Found");
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
        REQUIRE(response.reason_phrase == "I'm a teapot");
    }

    SECTION("Headers with special characters") {
        HttpResponse response;
        response.set_header("X-Custom", "Value with spaces and symbols: !Q#$%");
        REQUIRE(response.get_header("X-Custom") ==
                "Value with spaces and symbols: !Q#$%");
    }
}

TEST_CASE("HTTP Response - Enhanced Features", "[http]") {
    SECTION("JSON Response") {
        std::string json = R"({"name":"John","age":30})";
        HttpResponse response = HttpResponse::json_response(json);

        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("Content-Type") == "application/json");
        REQUIRE(response.body == json);
    }

    SECTION("HTML Response") {
        std::string html = "<html><body>Hello</body></html>";
        HttpResponse response = HttpResponse::html_response(html);

        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("Content-Type") == "text/html");
        REQUIRE(response.body == html);
    }

    SECTION("Binary Response") {
        std::vector<uint8_t> binary_data = {
            0x89, 0x50, 0x4E, 0x47,
            0x0D, 0x0A, 0x1A, 0x0A};  // PNG header HttpResponse response;
        HttpResponse response = HttpResponse::binary_response(binary_data);
        response.set_binary_body(binary_data, "image/png");

        REQUIRE(response.is_binary_response());
        REQUIRE(response.get_header("Content-Type") == "image/png");
        REQUIRE(response.get_header("Content-Length") == "8");
        REQUIRE(response.get_binary_body() == binary_data);
    }

    SECTION("Streaming Response") {
        HttpResponse response;
        std::string large_content = "Large content that would be streamed";

        response.set_streaming(
            [&large_content](std::ostream &os) { os << large_content; },
            large_content.length(), "text/plain");

        REQUIRE(response.is_streaming_response());
        REQUIRE(response.get_header("Content-Type") == "text/plain");
        REQUIRE(response.get_header("Content-Length") ==
                std::to_string(large_content.length()));

        std::ostringstream test_stream;
        response.write_to_stream(test_stream);
        REQUIRE(test_stream.str() == large_content);
    }
}
