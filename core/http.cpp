// Copyright [2025] <Nicolas Selig>
//
//

#include "http.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

HttpRequest HttpRequest::parse(const std::string &raw_request) {
    HttpRequest request;
    std::istringstream stream(raw_request);
    std::string line;

    if (std::getline(stream, line)) {
        std::istringstream request_line(line);
        request_line >> request.method >> request.path >> request.version;
    }

    while (std::getline(stream, line) && !line.empty() && line != "\r") {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            request.headers[key] = value;
        }
    }

    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }

    if (!body.empty()) {
        request.body = body;
    }
    return request;
}

HttpResponse HttpResponse::ok(const std::string &body) {
    HttpResponse response(200, "OK");
    if (!body.empty()) {
        response.set_body(body);
    }
    return response;
}

HttpResponse HttpResponse::not_found(const std::string &resource) {
    HttpResponse response(404, "Not Found");
    std::string body =
        "The requested resource '" + resource + "' was not found.";
    response.set_body(body, "text/html");
    return response;
}

HttpResponse HttpResponse::server_error(const std::string &message) {
    HttpResponse response(500, "Internal Server Error");
    std::string body = "Server error '" + message;
    response.set_body(body, "text/html");
    return response;
}

HttpResponse HttpResponse::bad_request(const std::string &message) {
    HttpResponse response(400, "Bad Request");
    std::string body = "Bad request: '" + message;
    response.set_body(body, "text/html");
    return response;
}

std::string HttpResponse::to_string() const {
    std::ostringstream response_stream;
    response_stream << version << " " << status_code << " " << status_text
                    << "\r\n";

    auto headers_copy = headers;

    if (!has_header("content-length")) {
        headers_copy["content-length"] = std::to_string(body.length());
    }

    for (const auto [name, value] : headers_copy) {
        std::string display_name;
        bool capitalize = true;
        for (char c : name) {
            if (capitalize && std::isalpha(c)) {
                display_name += std::toupper(c);
                capitalize = false;
            } else if (c == '-') {
                display_name += c;
                capitalize = true;
            } else {
                display_name += c;
            }
        }
        std::cout << display_name << ": " << value << "\r\n";
        response_stream << display_name << ": " << value << "\r\n";
    }

    response_stream << "\r\n";
    if (!body.empty()) {
        response_stream << body;
    }

    return response_stream.str();
}
