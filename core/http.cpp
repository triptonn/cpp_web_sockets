// Copyright [2025] <Nicolas Selig>
//
//

#include <bits/types/struct_timeval.h>
#include <chrono>
#include <exception>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../core/http.hpp"
#include "../core/logger.hpp"
#include "../core/string_utils.hpp"

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
    if (request.has_header("content-length")) {
        size_t content_length = std::stoul(request.get_header("content-length"));
        std::vector<char> body_buffer(content_length);
        // stream.ignore(2); // ignore \r\n
        if (stream.read(body_buffer.data(), content_length)) {
            request.body = std::string(body_buffer.data(), content_length);
        }
    } else {
        std::stringstream body_stream;
        body_stream << stream.rdbuf();
        request.body = body_stream.str();
        if (!request.body.empty() && request.body.back() == '\n') {
            request.body.pop_back();
            if (!request.body.empty() && request.body.back() == '\r') {
                request.body.pop_back();
            }
        }
    }
    return request;
}

bool HttpRequest::has_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return headers.find(lower_name) != headers.end();
}

std::string HttpRequest::get_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto it = headers.find(lower_name);
    return (it != headers.end()) ? it->second : "";
}

void HttpRequest::set_header(const std::string &key, const std::string &value) {
    std::string lower_key = key;
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    headers[lower_key] = value;
}

void HttpRequest::create_get(
    const std::string &request_uri,
    const std::map<std::string, std::string> &parameters) {
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
                processed_name = percent_encoding(name);
                path += processed_name;
            } else {
                path += name;
            }

            path += "=";

            if (value.find(" ") != std::string::npos ||
                value.find("+") != std::string::npos) {
                processed_value = percent_encoding(value);
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

void HttpRequest::create_post(
    const std::string &request_uri,
    const std::map<std::string, std::string> &form_data) {
    method = "POST";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;
    bool first = true;

    for (const auto &[key, value] : form_data) {
        if (!first) {
            content += "&";
        } else {
            first = false;
        }

        if (key != "email") {
            content += percent_encoding(key) + "=" + percent_encoding(value);
        } else {
            content += percent_encoding(key) + "=" + value;
        }
    }
    set_header("content-type", "application/x-www-form-urlencoded");
    set_header("content-length", std::to_string(content.length()));
    body = content;
}

void HttpRequest::create_put(
    const std::string &request_uri,
    const std::map<std::string, std::string> &form_data) {
    method = "PUT";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;
    bool first = true;
    for (const auto &[key, value] : form_data) {
        if (!first) {
            content += "&";
        } else {
            first = false;
        }
        if (key != "email") {
            content += percent_encoding(key) + "=" + percent_encoding(value);
        } else {
            content += percent_encoding(key) + "=" + value;
        }
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

void HttpRequest::create_delete(
    const std::string &request_uri,
    const std::map<std::string, std::string> &parameters) {
    method = "DELETE";
    path = request_uri;
    if (!parameters.empty()) {
        path += "?";
        int size = parameters.size();

        for (const auto [name, value] : parameters) {
            std::string processed_name;
            std::string processed_value;

            if (name.find(" ") != std::string::npos ||
                name.find("+") != std::string::npos) {
                processed_name = percent_encoding(name);
                path += processed_name;
            } else {
                path += name;
            }

            path += "=";

            if (value.find(" ") != std::string::npos ||
                value.find("+") != std::string::npos) {
                processed_value = percent_encoding(value);
                path += processed_value;
            } else {
                path += value;
            }

            if (size > 1) {
                path += "&";
                size -= 1;
            }
        }
    }
    version = "HTTP/1.1";
    set_header("Host", "localhost");
}

/////////////////////////////////
// HTTP Response
/////////////////////////////////

HttpResponse HttpResponse::ok(const std::string &body) {
    HttpResponse response(200, "OK");
    response.set_header("Content-Type", "text/plain");
    if (!body.empty()) {
        response.set_header("Content-Length", std::to_string(body.length()));
        response.set_body(body);
    }
    return response;
}

HttpResponse HttpResponse::json_response(const std::string &json_body) {
    HttpResponse response(200, "OK");
    response.set_body(json_body);
    response.set_header("Content-Type", "application/json");
    return response;
}

HttpResponse HttpResponse::html_response(const std::string &html_body) {
    HttpResponse response(200, "OK");
    response.set_body(html_body);
    response.set_header("Content-Type", "text/html");
    return response;
}

HttpResponse
HttpResponse::binary_response(const std::vector<uint8_t> &binary_body) {
    HttpResponse response(200, "OK");
    response.set_binary_body(binary_body);
    response.set_header("Content-Type", "image/png");
    response.set_header(
        "Content-Length",
        std::to_string(sizeof(response.get_header("Content-Type"))));
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

void HttpResponse::set_streaming(
    std::function<void(std::ostream &)> stream_callback, size_t content_length,
    const std::string &content_type) {
    this->stream_callback = stream_callback;
    this->is_streaming = true;
    set_header("Content-Type", content_type);
    set_header("Content-Length", std::to_string(content_length));
}

// TODO(triptonn): Implement protocol switching using correct header values
/* HttpResponse HttpResponse::switching_protocol() {
    HttpResponse response(101, "Switching Protocols");
    response.set_header("Upgrade", "websocket");
    response.set_header("Connection", "Upgrade");
    response.set_header("Sec-WebSocket-Accept", "SAMPLE_CODE");
    return response;
} */

HttpResponse HttpResponse::parse(const std::string &raw_response) {
    HttpResponse response;
    std::istringstream stream(raw_response);
    std::string line;

    if (std::getline(stream, line)) {
        if (!line.empty() || line.back() == '\r') {
            line.pop_back();
        }

        size_t first_space = line.find(' ');
        if (first_space != std::string::npos) {
            response.version = line.substr(0, first_space);
            size_t second_space = line.find(' ', first_space + 1);
            if (second_space != std::string::npos) {
                std::string status_code_str = line.substr(
                    first_space + 1, second_space - first_space - 1);
                response.status_code = std::stoi(status_code_str);
                response.reason_phrase = line.substr(second_space + 1);
            }
        }
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

            response.headers[key] = value;
        }
    }

    std::string body;
    if (response.has_header("content-length")) {
        size_t content_length = std::stoul(response.get_header("content-length"));
        std::vector<char> body_buffer(content_length);
        stream.ignore(2); // skip \r\n
        if (stream.read(body_buffer.data(), content_length)) {
            response.body = std::string(body_buffer.data(), content_length);
        }
    } else {
        std::stringstream body_stream;
        body_stream << stream.rdbuf();
        response.body = body_stream.str();
        if (!response.body.empty() && response.body.back() == '\n') {
            response.body.pop_back();
            if (!response.body.empty() && response.body.back() == '\r') {
                response.body.pop_back();
            }
        }
    }
    return response;
}

bool HttpResponse::has_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return headers.find(lower_name) != headers.end();
}

void HttpResponse::set_header(const std::string &key,
                              const std::string &value) {
    std::string lower_key = key;

    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    headers[lower_key] = value;
}

std::string HttpResponse::get_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto it = headers.find(lower_name);
    return (it != headers.end()) ? it->second : "";
}

HttpResponse &HttpResponse::set_body(const std::string &content,
                                     const std::string &content_type) {
    body = content;
    set_header("Content-Type", content_type);
    set_header("Content-Length", std::to_string(content.length()));
    return *this;
}

HttpResponse &
HttpResponse::set_binary_body(const std::vector<uint8_t> &binary_content,
                              const std::string &content_type) {
    body = std::string(binary_content.begin(), binary_content.end());
    is_binary = true;
    set_header("Content-Type", content_type);
    set_header("Content-Length", std::to_string(body.length()));
    return *this;
}

void HttpResponse::write_to_stream(std::ostream &os) const {
    if (is_streaming && stream_callback) {
        stream_callback(os);
    }
}

std::string HttpResponse::to_string() const {
    std::ostringstream response_stream;
    response_stream << version << " " << status_code << " " << reason_phrase
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

/////////////////////////////////
// HTTP Client
/////////////////////////////////

signed int HttpClient::connect_to_server() {
    if (connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)) < 0) {
        client_log.write("Client " + std::to_string(client_fd) +
                         " failed to connect");
        close(client_fd);
        return -1;
    }
    is_connected = true;
    client_log.write("Connected to server: " + hostname + ":" + port_str);
    return 0;
}

HttpResponse HttpClient::send_request(HttpRequest request) {
    if (!is_connected) {
        throw std::runtime_error("Not connected to server");
    }

    std::string request_str = request.to_string();
    if (send(client_fd, request_str.c_str(), request_str.length(), 0) < 0) {
        throw std::runtime_error("Failed to send request");
    }

    client_log.write("Client " + std::to_string(client_fd) +
                     " sent request: " + request.method + ", " + request.path);

    std::string response_data;
    char buffer[4096];
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);

        int select_result =
            select(client_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (select_result < 0) {
            throw std::runtime_error("Select error");
        } else if (select_result == 0) {
            throw std::runtime_error("Timeout waiting for response");
        }

        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            throw std::runtime_error("Error receiving response");
        }

        if (bytes_received == 0) {
            break;
        }

        client_log.write("Client " + std::to_string(client_fd) +
                         " received response");

        buffer[bytes_received] = '\0';
        response_data += buffer;

        if (response_data.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    return HttpResponse::parse(response_data);
}

void HttpClient::disconnect() {
    is_connected = false;
    client_log.write("Client " + std::to_string(client_fd) + " disconnected");
}

bool HttpClient::resolve_hostname() {
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    port_str = std::to_string(port);
    int status =
        getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &result);

    if (status != 0) {
        throw std::runtime_error("Error using getaddrinfo: " +
                                 std::to_string(*gai_strerror(status)));
        return false;
    }

    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (client_fd == -1) {
            continue;
        }

        if (rp->ai_family == AF_INET) {
            memcpy(&addr, rp->ai_addr, sizeof(struct sockaddr_in));
            break;
        }
    }

    freeaddrinfo(result);

    if (rp == nullptr) {
        throw std::runtime_error("Could not resolve hostname '" + hostname +
                                 "'");
        return false;
    }

    return true;
}

/////////////////////////////////
// HTTP Server
/////////////////////////////////

HttpServer::HttpServer(int16_t port) {
    host_port = port;
    if (host_port < 1024) {
        throw std::runtime_error("Port number must not be below 1024");
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    server_socket = std::make_unique<SocketGuard>(fd);
    server_fd = server_socket->get();

    server_log.write("Server " + std::to_string(server_fd) +
                     " started");

    flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(host_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        server_log.write("Server failed to bind to port " + std::to_string(host_port));
        throw std::runtime_error("Port already in use or in TIME_WAIT state");;
    } else {
        server_log.write("Server bound to port " + std::to_string(host_port));
    }
}


void HttpServer::start() {
    if (server_running.load()) {
        return;
    }

    int listen_return = listen(server_fd, fd_set.get_max_fd());
    if (listen_return < 0) {
        throw std::runtime_error("Server failed to listen");
    }

    server_running.store(true);
    server_log.write("Server starting on port " + std::to_string(host_port));

    server_thread = std::thread([this]() {
        while (server_running.load()) {
            /* std::chrono::milliseconds to = std::chrono::milliseconds(100);
            auto event = event_loop.wait_for_event(to);
            if (event != std::nullopt) {
                handle_event(event);
            } */
            main_loop();
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void HttpServer::stop() {
    if (!server_running.load()) {
        return;
    }

    server_running.store(false);
    queue_cv.notify_one();

    if (server_thread.joinable()) {
        server_thread.join();
    }

    server_log.write("Server stopped");

    for (int client_fd : client_manager.get_client_fds()) {
        if (client_fd > 0) {
            close(client_fd);
        }
    }
    client_manager.clear();

    if (server_fd > 0) {
        close(server_fd);
    }
}

void HttpServer::main_loop() {
    while (server_running.load()) {
        fd_set.clear();
        fd_set.add(server_fd);
        fd_set.add(STDIN_FILENO);

        for (int client_fd : client_manager.get_client_fds()) {
            fd_set.add(client_fd);
        }

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        int select_return =
            select(fd_set.get_max_fd() + 1, fd_set.get_read_fds(), nullptr,
                   nullptr, &timeout);

        if (select_return > 0) {
            if (fd_set.is_set(server_fd)) {
                event_loop.push_event({
                    ServerEventLoop::EventType::NEW_CONNECTION,
                    server_fd,
                    ""
                });
            }

            if (fd_set.is_set(STDIN_FILENO)) {
                std::string input;
                std::getline(std::cin, input);
                event_loop.push_event({
                    ServerEventLoop::EventType::SERVER_COMMAND,
                    STDIN_FILENO,
                    ""
                });
            }

            for (int fd : client_manager.get_client_fds()) {
                if (fd_set.is_set(fd)) {
                    event_loop.push_event({
                        ServerEventLoop::EventType::CLIENT_DATA,
                        fd,
                        ""
                    });
                }
            }
        }

        while (event_loop.has_events()) {
            auto event = event_loop.get_next_event();
            handle_event(event);
        }
    }
}

void HttpServer::handle_event(const ServerEventLoop::Event &event) {
    if (!server_running.load()) {
        return;
    }

    try {
        switch (event.type) {
        case ServerEventLoop::EventType::NEW_CONNECTION:
            handle_new_connection();
            break;
        case ServerEventLoop::EventType::CLIENT_DATA:
            if (client_manager.has_client(event.fd)) {
                handle_client_data(event.fd);
            }
            break;
        case ServerEventLoop::EventType::CLIENT_DISCONNECT:
            if (client_manager.has_client(event.fd)) {
                handle_client_disconnect(event.fd);
            }
            break;
        case ServerEventLoop::EventType::SERVER_COMMAND:
            if (event.data == "quit") {
                stop();
            }
            break;
        default:
            server_log.write("Unknown event type received");
            break;
        }
    } catch (const std::exception &e) {
        server_log.write("Error handling event: " + std::string(e.what()));
    }
}

void HttpServer::handle_new_connection() {
    if (fd_set.is_set(server_fd)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int new_client_fd =
            accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr),
                   &client_len);
        if (new_client_fd < 0) {
            server_log.write("Failed to accept new client connection");
            return;
        }

        handle_new_client(new_client_fd, client_addr);
    }
}

void HttpServer::handle_user_input() {
    if (fd_set.is_set(STDIN_FILENO)) {
        if (!server_running.load()) {
            return;
        }

        std::string input;
        std::getline(std::cin, input);

        if (input == "quit") {
            stop();
            server_log.write("Server terminated by user");
        }
    }
}

void HttpServer::handle_new_client(int client_fd,
                                   const sockaddr_in &client_addr) {
    std::unique_ptr<ClientSession> session_ptr(new ClientSession(
        client_fd, std::to_string(client_addr.sin_addr.s_addr)));
    fd_set.add(client_fd);
    client_manager.add_client(std::move(session_ptr));

    std::string client_fd_str = std::to_string(client_fd);

    std::string log_message = "Client " + client_fd_str + " connected";
    server_log.write(log_message);
}

int HttpServer::handle_client_data(int client_fd) {
    char buffer[4096];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        if (client_manager.has_client(client_fd) == true) {
            client_manager.remove_client(client_fd);
        }

        server_log.write("Client disconnected: " + std::to_string(client_fd));
        close(client_fd);
        return -5;
    }

    buffer[bytes_received] = '\0';
    server_log.write("Received: " + std::string(buffer));

    HttpRequest request = HttpRequest::parse(buffer);
    HttpResponse response = route_request(request);

    std::string response_str = response.to_string();
    int sending_status =
        send(client_fd, response_str.c_str(), response_str.length(), 0);
    if (sending_status == -1) {
        server_log.write("Failed to send response: " +
                         std::string(strerror(errno)));
        return -1;
    }

    server_log.write("Sent reponse: " + std::to_string(sending_status) +
                     " bytes");
    return 0;
}

void HttpServer::handle_client_disconnect(int client_fd) {
    if (!client_manager.has_client(client_fd)) {
        return;
    }
    fd_set.remove(client_fd);
    client_manager.remove_client(client_fd);

    close(client_fd);

    std::string log_message =
        "Client " + std::to_string(client_fd) + " disconnected";
    server_log.write(log_message);
}

bool HttpServer::check_connection(int client_fd) {
    return client_manager.has_client(client_fd);
}

HttpResponse HttpServer::route_request(const HttpRequest &request) {
    for (const auto route : routes) {
        if (route.method == request.method && route.path == request.path) {
            return route.handler(request);
        }
    }
    return HttpResponse::not_found(request.path);
}
