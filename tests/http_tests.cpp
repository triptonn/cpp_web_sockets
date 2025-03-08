// Copyright [2025] <Nicolas Selig>
//
//

#include <chrono>
#include <memory>
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
        std::string expected_request = "GET /index.html HTTP/1.1\r\n"
                                       "Host: localhost\r\n"
                                       "\r\n";
        REQUIRE_NOTHROW(request.create_get("/index.html"));

        REQUIRE(request.method == "GET");
        REQUIRE(request.path == "/index.html");
        REQUIRE(request.version == "HTTP/1.1");
        REQUIRE(request.get_header("host") == "localhost");

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
            {"username", "testuser"}, {"password", "testpass"}};

        REQUIRE_NOTHROW(request.create_post("/login", form_data));

        REQUIRE(request.method == "POST");
        REQUIRE(request.path == "/login");
        REQUIRE(request.get_header("content-type") ==
                "application/x-www-form-urlencoded");
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
            {"special", "!@#$%^&*()"}};

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
}

TEST_CASE("HTTP Client Request - PUT Method", "[http]") {
    SECTION("PUT request with empty body") {
        HttpRequest request;

        REQUIRE_NOTHROW(request.create_put("/api/products/123/activate"));

        REQUIRE(request.method == "PUT");
        REQUIRE(request.path == "/api/products/123/activate");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") == "0");
        REQUIRE(request.body.empty());
    }

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

    SECTION("Create PUT request with custom header and xml payload") {
        HttpRequest request;
        std::string xml_body =
            "<product><id>123</id><name>Updated Product</name></product>";

        REQUIRE_NOTHROW(request.create_put("/api/products/123", xml_body,
                                           "application/xml"));
        request.set_header("Authorization", "Bearer token123");
        request.set_header("If-Match", "\"abc123\"");

        REQUIRE(request.has_header("authorization"));
        REQUIRE(request.get_header("Authorization") == "Bearer token123");
        REQUIRE(request.has_header("if-match"));
        REQUIRE(request.get_header("If-Match") == "\"abc123\"");

        std::string request_str = request.to_string();
        REQUIRE(request_str.find("Authorization: Bearer token123\r\n") !=
                std::string::npos);
        REQUIRE(request_str.find("If-Match: \"abc123\"\r\n") !=
                std::string::npos);
    }

    SECTION("PUT request with form data containing special characters") {
        HttpRequest request;
        std::map<std::string, std::string> form_data = {
            {"description", "Product with & special < characters"},
            {"tags", "electronics, gadgets"},
            {"symbols", "!@#$%^&*()"}};

        REQUIRE_NOTHROW(request.create_put("/api/products/123", form_data));

        REQUIRE(request.body.find("description=Product%20with%20%26%20special%"
                                  "20%3C%20characters") != std::string::npos);
        REQUIRE(request.body.find("tags=electronics%2C%20gadgets") !=
                std::string::npos);
        REQUIRE(request.body.find("symbols=%21%40%23%24%25%5E%26%2A%28%29") !=
                std::string::npos);
    }

    SECTION("PUT request with binary data") {
        HttpRequest request;

        std::string binary_data = "Binary\0Data\0For\0PUT";
        binary_data.resize(18);

        REQUIRE_NOTHROW(request.create_put("/api/products/123/image",
                                           binary_data, "image/png"));

        REQUIRE(request.has_header("content-type"));
        REQUIRE(request.get_header("content-type") == "image/png");
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") == "18");
        REQUIRE(request.body.size() == 18);
    }

    SECTION("PUT request with conditional headers") {
        HttpRequest request;
        std::string json_body = R"({"status":"active"})";

        REQUIRE_NOTHROW(request.create_put("/api/products/123/status",
                                           json_body, "application/json"));
        request.set_header("If-Unmodified-Since",
                           "Wed, 21 Oct 2015 07:28:00 GMT");
        request.set_header("If-Match", "\"737060cd8c284d8af7ad3082f209582d\"");

        REQUIRE(request.has_header("if-unmodified-since"));
        REQUIRE(request.has_header("if-match"));
        REQUIRE(request.get_header("if-unmodified-since") ==
                "Wed, 21 Oct 2015 07:28:00 GMT");
        REQUIRE(request.get_header("if-match") ==
                "\"737060cd8c284d8af7ad3082f209582d\"");
    }
}

TEST_CASE("HTTP Client Request - DELETE Method", "[http]") {
    SECTION("Create basic DELETE request") {
        HttpRequest request;

        REQUIRE_NOTHROW(request.create_delete("/api/products/123"));

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/products/123");
        REQUIRE(request.version == "HTTP/1.1");
        REQUIRE(request.body.empty());
        REQUIRE(request.has_header("content-length"));
        REQUIRE(request.get_header("content-length") == "0");
    }

    SECTION("DELETE request with query parameters") {
        HttpRequest request;
        std::map<std::string, std::string> params = {{"force", "true"},
                                                     {"notify", "admin"}};
        REQUIRE_NOTHROW(request.create_delete("/api/products/123", params));

        REQUIRE(request.method == "DELETE");
        REQUIRE((request.path == "/api/products/123?force=true&notify=admin" ||
                 request.path == "/api/products/123?notify=admin&force=true"));
        REQUIRE(request.body.empty());
    }

    SECTION("DELETE request with authentication headers") {
        HttpRequest request;

        REQUIRE_NOTHROW(request.create_delete("/api/users/456"));
        request.set_header("Authorization", "Bearer token123");

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/users/456");
        REQUIRE(request.has_header("authorization"));
        REQUIRE(request.get_header("Authorization") == "Bearer token123");

        std::string request_str = request.to_string();
        REQUIRE(request_str.find("Authorization: Bearer token123\r\n") !=
                std::string::npos);
    }

    SECTION("DELETE request with conditional headers") {
        HttpRequest request;
        REQUIRE_NOTHROW(request.create_delete("/api/documents/789"));
        request.set_header("If-Match", "\"abc123\"");

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/documents/789");
        REQUIRE(request.has_header("if-match"));
        REQUIRE(request.get_header("if-match") == "\"abc123\"");
    }

    SECTION("DELETE request with special characters in path") {
        HttpRequest request;

        REQUIRE_NOTHROW(
            request.create_delete("/api/files/report with spaces.pdf"));

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/files/report%20with%20spaces.pdf");
    }

    SECTION("DELETE request with special characters in query parameters") {
        HttpRequest request;
        std::map<std::string, std::string> params = {
            {"reason", "duplicate content"}, {"tag", "temp & draft"}};

        REQUIRE_NOTHROW(request.create_delete("/api/posts/101", params));

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path.find("/api/posts/101?") == 0);
        REQUIRE(request.path.find("reason=duplicate%20content") !=
                std::string::npos);
        REQUIRE(request.path.find("tag=temp%20%26%20draft") !=
                std::string::npos);
    }

    SECTION("DELETE request with Accept header for response format") {
        HttpRequest request;

        REQUIRE_NOTHROW(request.create_delete("/api/comments/202"));
        request.set_header("Accept", "application/json");

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/comments/202");
        REQUIRE(request.has_header("accept"));
        REQUIRE(request.get_header("Accept") == "application/json");
    }

    SECTION("DELETE request with multiple headers") {
        HttpRequest request;

        REQUIRE_NOTHROW(request.create_delete("/api/subscription/303"));
        request.set_header("Authorization", "Bearer token123");
        request.set_header("X-Request-ID", "req-123-456");
        request.set_header("User-Agent", "MyClient/1.0");

        REQUIRE(request.method == "DELETE");
        REQUIRE(request.path == "/api/subscription/303");
        REQUIRE(request.has_header("authorization"));
        REQUIRE(request.has_header("x-request-id"));
        REQUIRE(request.has_header("user-agent"));
        REQUIRE(request.get_header("X-Request-ID") == "req-123-456");
        REQUIRE(request.get_header("User-Agent") == "MyClient/1.0");
    }
}

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

TEST_CASE("HttpClient - Connection Management", "[client]") {
    std::unique_ptr<HttpServer> server;

    SECTION("Connect to running server") {
        server = std::make_unique<HttpServer>(8080);
        server->start();

        HttpClient client("localhost", 8080);
        REQUIRE(client.connect_to_server() == 0);

        server->stop();
    }

    SECTION("Connection state tracking") {
        server = std::make_unique<HttpServer>(8080);
        server->start();

        HttpClient client("localhost", 8080);
        client.connect_to_server();
        client.disconnect();
        REQUIRE_NOTHROW(client.connect_to_server());

        server->stop();
    }

    SECTION("Connect to non-existent server") {
        HttpClient client("localhost", 9999);
        REQUIRE(client.connect_to_server() == -1);
    }
}

TEST_CASE("HttpServer - Basic Setup", "[server]") {
    SECTION("Server initialization with valid port") {
        REQUIRE_NOTHROW(HttpServer(8080));
    }

    /* SECTION("Server initialization with invalid port") {
        REQUIRE_THROWS_AS(HttpServer(80), std::runtime_error);
    }

    SECTION("Server initialization with already in use port") {
        HttpServer server1(8081);
        REQUIRE_THROWS_AS(HttpServer(8081); std::runtime_error);
    } */
}

/* TEST_CASE("HttpServer - Request HAndling", "[server]") {
    HttpServer(8082);

    SECTION("Register GET route handler") {
        server.get("/test", [](const HttpRequest &req) -> HttpResponse {
            return HttpResponse::ok("Test response");
        });

        HttpRequest test_request;
        test_request.create_get("/test");

        auto response = server.handle_request(test_request);
        REQUIRE(response.status_code == 200);
        REQUIRE(response.body == "Test response");
    }

    SECTION("Register POST route handler") {
        server.post("/api/data", [](const HttpRequest &req) -> HttpResponse {
            if (req.has_header("content-type") &&
                req.get_header("content-type") == "application/json") {
                return HttpResponse::json_response("{\"status\":\"success\"}");
            }
            return HttpResponse::bad_request("Invalid content-type");
        });

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
    }
} */

/* TEST_CASE("HttpServer - Connection Management", "[server]") {
    HttpServer server(8083);

    SECTION("Accept new connection") {
        REQUIRE_NOTHROW(server.start());
        HttpClient client("localhost", 8083);
        REQUIRE(client.connect_to_server() == 0);
    }

    SECTION("Handle multiple connections") {
        REQUIRE_NOTHROW(server.start());
        std::vector<HttpClient> clients;

        for (int i = 0; i < 3; i++) {
            clients.emplace_back("localhost", 8083);
            REQUIRE(clients.back().connect_to_server() == 0);
        }
    }
}

TEST_CASE("HttpServer - Response Streaming", "[server]") {
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

TEST_CASE("Client Request-Response Functionality", "[client]") {
    std::unique_ptr<HttpServer> server = std::make_unique<HttpServer>(8087);
    server->get("/test", [](const HttpRequest &req) {
        return HttpResponse::ok("Test response");
    });

    server->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SECTION("Send GET request and receive response") {
        HttpClient client("localhost", 8087);
        REQUIRE_NOTHROW(client.connect_to_server());

        HttpRequest request;
        request.create_get("/test");
        auto response = client.send_request(request);

        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("content-type") == "text/plain");

        client.disconnect();
    }

    /* SECTION("POST request with form data") {
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
    } */

    server->stop();
}

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
