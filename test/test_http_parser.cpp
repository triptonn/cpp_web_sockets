// Copyright [2025] <Nicolas Selig>
//
//

#include "../core/http.hpp"

#include <iostream>
#include <string>

int main() {
    std::string sample_get_request = "GET /index.html HTTP/1.1\r\n"
                                     "Host: example.com\r\n"
                                     "User-Agent: Mozilla/5.0\r\n"
                                     "Accept: text/html\r\n"
                                     "Connection: keep-alive\r\n"
                                     "\r\n";

    std::string sample_post_request =
        "POST /submit-form HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=john&password=pass";

    HttpRequest get_request = HttpRequest::parse(sample_get_request);
    std::cout << "=== GET Request ===\n";
    std::cout << "Method: " << get_request.method << "\n";
    std::cout << "Path: " << get_request.path << "\n";
    std::cout << "Version: " << get_request.version << "\n";
    std::cout << "Method:\n";
    for (const auto &[key, value] : get_request.headers) {
        std::cout << "   " << key << ": " << value << "\n";
    }
    std::cout << "Body: " << get_request.body << "\n\n";

    HttpRequest post_request = HttpRequest::parse(sample_post_request);
    std::cout << "=== POST Request ===\n";
    std::cout << "Method: " << post_request.method << "\n";
    std::cout << "Path: " << post_request.path << "\n";
    std::cout << "Version: " << post_request.version << "\n";
    std::cout << "Method:\n";
    for (const auto &[key, value] : post_request.headers) {
        std::cout << "   " << key << ": " << value << "\n";
    }
    std::cout << "Body: " << post_request.body << "\n\n";

    bool has_header_test = get_request.has_header("Host");
    bool has_header_not_test = get_request.has_header("Date");

    std::cout << "has header test: " << has_header_test << "\n";
    std::cout << "has header not test: " << has_header_not_test << "\n";

    return 0;
}
