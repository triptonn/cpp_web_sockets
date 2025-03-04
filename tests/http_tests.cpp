// Copyright [2025] <Nicolas Selig>
//
//

#define CATCH_CONFIG_MAIN
#include "../core/http.hpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

/////////////////////////////////
// HTTP Request Parsing
/////////////////////////////////

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

/////////////////////////////////
// HTTP Request Creation
/////////////////////////////////

TEST_CASE("HTTP Client Request Creation - GET Method", "[http]") {
    SECTION("Create basic GET request") {
        HttpRequest request;
        REQUIRE_NOTHROW(request.create_get("/index.html"));

        std::string expected_request = "GET /index.html HTTP/1.1\r\n"
                                       "Host: localhost\r\n"
                                       "\r\n";

        REQUIRE(request.to_string() == expected_request);
    }

    SECTION("Add custom headers to GET request") {
        HttpRequest request;
        request.create_get("/api/data");

        request.set_header("Authorization", "Bearer token123");
        request.set_header("Accept", "application/json");
        request.set_header("X-Customer-Header", "custom-value");

        std::string request_str = request.to_string();

        REQUIRE(request.get_header("Authorization") == "Bearer token123");
        REQUIRE(request.get_header("Accept") == "application/json");
        REQUIRE(request.get_header("X-Customer-Header") == "custom-value");
        REQUIRE(request_str.find("Authorization: Bearer token123\r\n") !=
                std::string::npos);
        REQUIRE(request_str.find("Accept: application/json\r\n") !=
                std::string::npos);
        REQUIRE(request_str.find("X-Customer-Header: custom-value\r\n") !=
                std::string::npos);
    }

    SECTION("GET request with query parameters") {
        HttpRequest request;
        std::map<std::string, std::string> params = {
            {"page", "1"}, {"limit", "10"}, {"sort", "desc"}};

        REQUIRE_NOTHROW(request.create_get("/api/items", params));

        std::string request_str = request.to_string();
        REQUIRE(request.path.find("/api/items?") == 0);
        REQUIRE(request.path.find("page=1") != std::string::npos);
        REQUIRE(request.path.find("limit=10") != std::string::npos);
        REQUIRE(request.path.find("sort=desc") != std::string::npos);
        REQUIRE(request.path.find("&") != std::string::npos);

        std::string path = request.path;
        REQUIRE(path.find("page=1") != std::string::npos);
        REQUIRE(path.find("limit=10") != std::string::npos);
        REQUIRE(path.find("sort=desc") != std::string::npos);
    }

    SECTION("GET request with special characters in parameters") {
        HttpRequest request;
        std::map<std::string, std::string> params = {{"search", "hello world"},
                                                     {"tag", "c++"}};

        REQUIRE_NOTHROW(request.create_get("/api/search", params));

        std::string path = request.path;
        REQUIRE(path.find("search=hello%20world") != std::string::npos);
        REQUIRE(path.find("tag=c%2B%2B") != std::string::npos);
    }
}

TEST_CASE("HTTP Client Request - POST Method", "[http]") {
    SECTION("Basic POST request with empty body") {
        HttpRequest request;
        REQUIRE_NOTHROW(request.create_post("/api/ping"));

        REQUIRE(request.method == "POST");
        REQUIRE(request.path == "/api/ping");
        REQUIRE(request.version == "HTTP/1.1");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") == "0");
        REQUIRE(request.body.empty());
    }

    SECTION("Create POST request with custom headers") {
        HttpRequest request;
        std::string text_body = "This is a plain text body";

        REQUIRE_NOTHROW(
            request.create_post("/api/messages", text_body, "text/plain"));
        request.set_header("Authorization", "Bearer token123");
        request.set_header("X-Custom-Header", "custom-value");

        REQUIRE(request.has_header("authorization"));
        REQUIRE(request.get_header("Authorization") == "Bearer token123");
        REQUIRE(request.has_header("x-custom-header"));
        REQUIRE(request.get_header("X-Custom-Header") == "custom-value");

        std::string request_str = request.to_string();
        REQUIRE(request_str.find("Authorization: Bearer token123\r\n") !=
                std::string::npos);
        REQUIRE(request_str.find("X-Custom-Header: custom-value\r\n") !=
                std::string::npos);
    }

    SECTION("Create POST request with form data") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {
            {"username", "john_doe"},
            {"email", "john@example.com"},
            {"subscribe", "true"}};

        REQUIRE_NOTHROW(request.create_post("/api/users", form_data));

        REQUIRE(request.method == "POST");
        REQUIRE(request.path == "/api/users");
        REQUIRE(request.version == "HTTP/1.1");
        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") ==
                "application/x-www-form-urlencoded");
        REQUIRE(request.has_header("content-length"));

        REQUIRE(request.body.find("username=john_doe") != std::string::npos);
        REQUIRE(request.body.find("email=john@example.com") !=
                std::string::npos);
        REQUIRE(request.body.find("subscribe=true") != std::string::npos);
        REQUIRE(request.body.find("&") != std::string::npos);
    }

    SECTION("Create POST request with login form data") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {
            {"username", "testuser"},
            {"password", "testpass"}
        };

        REQUIRE_NOTHROW(request.create_post("/login", form_data));

        REQUIRE(request.method == "POST");
        REQUIRE(request.path == "/login");
        REQUIRE(request.get_header("content-type") == "application/x-www-form-urlencoded");
        REQUIRE(request.body.find("username=testuser") != std::string::npos);
        REQUIRE(request.body.find("password=testpass") != std::string::npos);
    }

    SECTION("Create POST request with JSON data") {
        HttpRequest request;
        std::string json_data =
            R"({"name":"John Doe","age":30,"email":"john@example.com"})";

        REQUIRE_NOTHROW(
            request.create_post("/api/users", json_data, "application/json"));

        REQUIRE(request.method == "POST");
        REQUIRE(request.path == "/api/users");
        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") == "application/json");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") ==
                std::to_string(json_data.length()));
        REQUIRE(request.body == json_data);
    }

    SECTION("POST request with form data containing special characters") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {
            {"search", "hello world"},
            {"tags", "c++, programming"},
            {"special", "!@#$%^&*()"}
        };

        REQUIRE_NOTHROW(request.create_post("/api/search", form_data));

        REQUIRE(request.body.find("search=hello%20world") != std::string::npos);
        REQUIRE(request.body.find("tags=c%2B%2B%2C%20programming") !=
                std::string::npos);
        REQUIRE(request.body.find("special=%21%40%23%24%25%5E%26%2A%28%29") !=
                std::string::npos);
    }

    SECTION("POST request with binary data") {
        HttpRequest request;
        std::string binary_data = "Binary\0Data\0With\0Nulls";
        binary_data.resize(20);

        REQUIRE_NOTHROW(request.create_post("/api/upload", binary_data,
                                            "application/octet-stream"));

        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") ==
                "application/octet-stream");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") == "20");
        REQUIRE(request.body.size() == 20);
    }

/* TEST_CASE("HTTP Client Request - PUT Method", "[http]") {
    SECTION("Create basic PUT request with form data") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {
            {"id", "123"}, {"name", "Updated Product"}, {"price", "29.99"}};

        REQUIRE_NOTHROW(request.create_put("/api/products/123", form_data));

        REQUIRE(request.method == "PUT");
        REQUIRE(request.path == "/api/products/123");
        REQUIRE(request.version == "HTTP/1.1");
        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") ==
                "application/x-www-form-urlencoded");
        REQUIRE(request.has_header("content-length"));

        REQUIRE(request.body.find("id=123") != std::string::npos);
        REQUIRE(request.body.find("name=Updated%20Product") !=
                std::string::npos);
        REQUIRE(request.body.find("price=29.99") != std::string::npos);
        REQUIRE(request.body.find("&") != std::string::npos);
    }

    SECTION("Create PUT request with JSON data") {
        HttpRequest request;
        std::string json_body =
            R"({"id":123,"name":"Updated Product","price":"29.99"})";

        REQUIRE_NOTHROW(request.create_put("/api/products/123", json_body,
                                           "application/json"));

        REQUIRE(request.method == "PUT");
        REQUIRE(request.path == "/api/products/123");
        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") == "application/json");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") ==
                std::to_string(json_body.length()));
        REQUIRE(request.body == json_body);
    }

    // TODO: Next steps...

    SECTION("Create PUT request with custom header") {}

    SECTION("PUT request with form data containing special characters") {}

    SECTION("PUT request with empty body") {}

    SECTION("PUT request with binary data") {}

    SECTION("PUT request with conditional headers") {} */
}

/////////////////////////////////
// HTTP Response Creation
/////////////////////////////////

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
