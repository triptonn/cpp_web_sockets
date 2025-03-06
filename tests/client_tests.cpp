// Copyright [2025] <Nicolas Selig>
//
//

#define CATCH_CONFIG_MAIN
#include "../core/http.hpp"
#include "../client/myclient.cpp"
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

//int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }

TEST_CASE("Client Request-Response Functionality", "[client]") {
    SECTION("Send GET request and receive response") {
        HttpClient client("localhost", 8080);

        HttpRequest request;
        request.create_get("/test");

        REQUIRE_NOTHROW(client.connect());

        auto response = client.send_request(request);

        REQUIRE(response.status_code == 200);
        REQUIRE(response.get_header("content-type") == "text/plain");
    }
}
