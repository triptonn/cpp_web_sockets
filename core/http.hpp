// Copyright [2025] <Nicolas Selig>
//
//

#pragma once
#include "../core/logger.hpp"
#include "string_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <type_traits>
#include <uchar.h>
#include <unistd.h>

/////////////////////////////////
// HTTP Request
/////////////////////////////////

class HttpRequest {
  public:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    // Default constructor just creating an empty HttpRequest shell
    HttpRequest() {}

    static HttpRequest parse(const std::string &raw_request);

    bool has_header(const std::string &name) const;
    std::string get_header(const std::string &name) const;
    void set_header(const std::string &key, const std::string &value);

    void create_get(const std::string &request_uri,
                    const std::map<std::string, std::string> &parameters = {});

    std::string to_string() const;

    void create_post(const std::string &request_uri) {
        method = "POST";
        path = request_uri;
        version = "HTTP/1.1";
        set_header("host", "localhost");
        set_header("content-length", "0");
    };

    void create_post(const std::string &request_uri,
                     const std::map<std::string, std::string> &form_data);

    template <typename T>
    void create_post(const std::string &request_uri, const T &data,
                     const std::string &content_type = "application/json");

    void create_put(const std::string &request_uri) {
        method = "PUT";
        path = request_uri;
        version = "HTTP/1.1";
        set_header("host", "localhost");
        set_header("content-length", "0");
    };

    void create_put(const std::string &request_uri,
                    const std::map<std::string, std::string> &form_data);

    template <typename T>
    void create_put(const std::string &request_uri, const T &data,
                    const std::string &content_type = "application/json");

    void create_delete(const std::string &request_uri) {
        method = "DELETE";
        path = percent_encoding(request_uri, "spaces");
        version = "HTTP/1.1";
        set_header("host", "localhost");
        set_header("content-length", "0");
    };

    void create_delete(const std::string &request_uri,
                       const std::map<std::string, std::string> &params);
};

template <typename T>
void HttpRequest::create_post(const std::string &request_uri, const T &data,
                              const std::string &content_type) {
    method = "POST";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;

    if constexpr (std::is_same_v<T, std::string>) {
        content = data;
    } else {
        static_assert(std::is_same_v<T, std::string>,
                      "Unsupported data type for POST request");
    }

    set_header("content-type", content_type);
    set_header("content-length", std::to_string(content.length()));

    body = content;
}

template <typename T>
void HttpRequest::create_put(const std::string &request_uri, const T &data,
                             const std::string &content_type) {
    method = "PUT";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;

    if constexpr (std::is_same_v<T, std::string>) {
        content = data;
    } else {
        static_assert(std::is_same_v<T, std::string>,
                      "Unsupported data type for PUT request");
    }

    set_header("content-type", content_type);
    set_header("content-length", std::to_string(content.length()));

    body = content;
}

inline bool HttpRequest::has_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return headers.find(lower_name) != headers.end();
}

inline std::string HttpRequest::get_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto it = headers.find(lower_name);
    return (it != headers.end()) ? it->second : "";
}

inline void HttpRequest::set_header(const std::string &key,
                                    const std::string &value) {
    std::string lower_key = key;
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    headers[lower_key] = value;
}

/////////////////////////////////
// HTTP Response
/////////////////////////////////

class HttpResponse {
  public:
    int status_code;
    std::string reason_phrase;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse(int code = 200, std::string text = "OK",
                 std::string vers = "HTTP/1.1") {
        status_code = code;
        reason_phrase = text;
        version = vers;
    }

    // Status code 200
    static HttpResponse ok(const std::string &body = "");
    static HttpResponse json_response(const std::string &json = "");
    static HttpResponse html_response(const std::string &html = "");
    static HttpResponse binary_response(const std::vector<uint8_t> &binary);

    static HttpResponse switching_protocol();
    static HttpResponse not_found(const std::string &resource = "");
    static HttpResponse server_error(const std::string &message = "");
    static HttpResponse bad_request(const std::string &message = "");

    // parsing
    static HttpResponse parse(const std::string &raw_response);

    bool has_header(const std::string &name) const;
    void set_header(const std::string &key, const std::string &value);
    std::string get_header(const std::string &name) const;

    HttpResponse &set_body(const std::string &content,
                           const std::string &content_type = "text/plain") {
        body = content;
        set_header("Content-Type", content_type);
        set_header("Content-Length", std::to_string(content.length()));
        return *this;
    }

    bool is_binary_response() { return is_binary; };

    HttpResponse &set_binary_body(
        const std::vector<uint8_t> &binary_content,
        const std::string &content_type = "application/octet-stream") {
        body = std::string(binary_content.begin(), binary_content.end());
        is_binary = true;
        set_header("Content-Type", content_type);
        set_header("Content-Length", std::to_string(body.length()));
        return *this;
    }

    std::vector<uint8_t> get_binary_body() {
        return std::vector<uint8_t>(body.begin(), body.end());
    }

    std::string to_string() const;

    bool is_streaming_response() { return is_streaming; };
    void set_streaming(std::function<void(std::ostream &)> stream_callback,
                       size_t content_length,
                       const std::string &content_type = "text/plain");
    void write_to_stream(std::ostream &os) const;

  private:
    bool is_binary = false;
    bool is_streaming = false;

    std::function<void(std::ostream &)> stream_callback;
};

inline bool HttpResponse::has_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return headers.find(lower_name) != headers.end();
}

inline void HttpResponse::set_header(const std::string &key,
                                     const std::string &value) {
    std::string lower_key = key;

    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    headers[lower_key] = value;
}

inline std::string HttpResponse::get_header(const std::string &name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto it = headers.find(lower_name);
    return (it != headers.end()) ? it->second : "";
}

/////////////////////////////////
// HTTP Client
/////////////////////////////////

class HttpClient {
  public:
    Logger client_log = Logger("client.log");

    HttpClient(std::string host_name, short int host_port)
        : hostname(host_name), port(host_port) {
        std::cout << "Creating client \n";

        if (!resolve_hostname()) {
            throw std::runtime_error("Failed to resolve hostname: " + hostname);
        }
    }

    signed int connect_to_server() {
        std::cout << "Hostname: " << addr.sin_addr.s_addr << std::endl;
        std::cout << "Port: " << addr.sin_port << std::endl;

        if (connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr),
                    sizeof(addr)) < 0) {
            std::cerr << "Connect failed\n";
            close(client_fd);
            return -1;
        }
        is_connected = true;
        client_log.write("Connected to server: " + hostname + ":" + port_str);
        return 0;
    };

    HttpResponse send_request(HttpRequest request);

    void disconnect() {
        is_connected = false;
        client_log.write("Disconnected from server");
    };

    ~HttpClient() {
        if (is_connected) {
            close(client_fd);
            is_connected = false;
        }
        client_log.write("Removing client");
    };

  private:
    std::string hostname;
    std::string port_str;

    bool is_connected = false;
    int client_fd;
    struct sockaddr res;
    struct sockaddr_in addr;
    short int port;

    bool resolve_hostname() {
        struct addrinfo hints;
        struct addrinfo *result;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        port_str = std::to_string(port);

        std::cout << "Hostname: " << hostname << std::endl;
        std::cout << "Port: " << port_str << std::endl;

        int status =
            getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &result);

        if (status != 0) {
            std::cerr << "getaddrinfo error: " << gai_strerror(status)
                      << std::endl;
            return false;
        }

        struct addrinfo *rp;
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if (client_fd == -1) {
                continue;
            }

            std::cout << "before Memcpy: " << addr.sin_addr.s_addr << ", "
                      << addr.sin_port << std::endl;
            if (rp->ai_family == AF_INET) {
                memcpy(&addr, rp->ai_addr, sizeof(struct sockaddr_in));
                std::cout << "after Memcpy: " << addr.sin_addr.s_addr << ", "
                          << addr.sin_port << std::endl;
                break;
            }
        }

        freeaddrinfo(result);

        if (rp == nullptr) {
            std::cerr << "Could not resolve hostname" << std::endl;
            return false;
        }

        return true;
    }
};

/////////////////////////////////
// HTTP Server
/////////////////////////////////

class HttpServer {
  private:
    std::mutex client_mutex;
    std::atomic<bool> server_running{false};

    std::thread server_thread;

    std::vector<int> client_fds;
    fd_set read_fds;
    short int host_port;
    int max_fd = 100;
    timeval timeout;
    int server_fd;
    int flags;

    struct sockaddr_in addr;

    using RouteHandler = std::function<HttpResponse(const HttpRequest &)>;
    struct Route {
        std::string method;
        std::string path;
        RouteHandler handler;
    };
    std::vector<Route> routes;

    void handle_new_connection();
    void handle_user_input();
    int handle_client_data(int client_fd);
    void main_loop();

    HttpResponse route_request(const HttpRequest &request) {
        for (const auto route : routes) {
            if (route.method == request.method && route.path == request.path) {
                return route.handler(request);
            }
        }
        return HttpResponse::not_found(request.path);
    }

  public:
    Logger server_log = Logger("server.log");

    HttpServer(short int port) : host_port(port) {
        std::cout << "Logger initialized" << std::endl;

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        int reuse = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                       sizeof(reuse)) < 0) {
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }
        flags = fcntl(server_fd, F_GETFL);
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

        addr.sin_family = AF_INET;
        addr.sin_port = htons(host_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    void start();
    void stop();

    void get(const std::string &path, RouteHandler handler) {
        register_route("GET", path, handler);
    }

    void post(const std::string &path, RouteHandler handler) {
        register_route("POST", path, handler);
    }

    void register_route(const std::string &method, const std::string &path,
                        RouteHandler handler) {
        routes.push_back({method, path, handler});
    }

    ~HttpServer() {
        if (server_running.load()) {
            stop();
        }
        /* if (server_thread.joinable()) {
            server_thread.join();
        } */
        server_log.write("Server shutting down");
    }
};
