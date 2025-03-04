// Copyright [2025] <Nicolas Selig>
//
//

#include <algorithm>
#include <sstream>
#include <string>

#include "http.hpp"
#include "string_utils.hpp"

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

void HttpRequest::create_get(std::string request_uri,
                             std::map<std::string, std::string> parameters) {
    method = "GET";
    path = request_uri;
    if (!parameters.empty()) {
        path += "?";
        int size = parameters.size();

        for (const auto [name, value] : parameters) {
            std::string processed_name;
            std::string processed_value;

            if (name.find(" ") != std::string::npos ||
                name.find("+") != std::string::npos) {
                processed_name = handle_special_chars_for_path(name);
                path += processed_name;
            } else {
                path += name;
            }

            path += "=";

            if (value.find(" ") != std::string::npos ||
                value.find("+") != std::string::npos) {
                processed_value = handle_special_chars_for_path(value);
                path += processed_value;
            } else {
                path += value;
            }

            if (size > 1) {
                path += "&";
                size -= 1;
            } else {
                continue; 
            }
        }
    }
    version = "HTTP/1.1";
    set_header("Host", "localhost");
}

void HttpRequest::create_post(const std::string& request_uri, const std::map<std::string, std::string>& form_data) {
    method = "POST";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;
    bool first = true;

    for (const auto& [key, value] : form_data) {
        if (!first) {
            content += "&";
        }
        content += handle_special_chars_for_path(key) + "=" + handle_special_chars_for_path(value);
        first = false;
    }

    set_header("content-type", "application/x-www-form-urlencoded");
    set_header("content-length", std::to_string(content.length()));
    body = content;
}

std::string HttpRequest::to_string() const {
    auto headers_copy = headers;

    std::ostringstream request_stream;
    request_stream << method << " " << path << " " << version << "\r\n";
    for (const auto [name, value] : headers_copy) {
        std::string display_name = format_header_name(name);
        request_stream << display_name << ": " << value << "\r\n";
    }

    request_stream << "\r\n";
    if (!body.empty()) {
        request_stream << body;
    }

    return request_stream.str();
}

HttpResponse HttpResponse::switching_protocol() {
    // TODO: Implment correct header values
    HttpResponse response(101, "Switching Protocols");
    response.set_header("Upgrade", "websocket");
    response.set_header("Connection", "Upgrade");
    response.set_header("Sec-WebSocket-Accept", "SAMPLE_CODE");

    return response;
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
        std::string display_name = format_header_name(name);
        response_stream << display_name << ": " << value << "\r\n";
    }

    response_stream << "\r\n";
    if (!body.empty()) {
        response_stream << body;
    }

    return response_stream.str();
}
