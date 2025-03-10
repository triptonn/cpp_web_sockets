// Copyright [2025] <Nicolas Selig>
//
//

#pragma once
#include "../core/logger.hpp"
#include "string_utils.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
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

    HttpRequest() {}

    static HttpRequest parse(const std::string &raw_request);
    bool has_header(const std::string &name) const;
    std::string get_header(const std::string &name) const;
    void set_header(const std::string &key, const std::string &value);
    std::string to_string() const;

    void create_get(const std::string &request_uri,
                    const std::map<std::string,
                    std::string> &parameters = {});

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

    static HttpResponse not_found(const std::string &resource = "");
    static HttpResponse server_error(const std::string &message = "");
    static HttpResponse bad_request(const std::string &message = "");

    static HttpResponse switching_protocol();

    // parsing
    static HttpResponse parse(const std::string &raw_response);

    bool has_header(const std::string &name) const;
    void set_header(const std::string &key, const std::string &value);
    std::string get_header(const std::string &name) const;

    HttpResponse &set_body(const std::string &content,
                           const std::string &content_type = "text/plain");

    bool is_binary_response() { return is_binary; };

    HttpResponse &set_binary_body(
        const std::vector<uint8_t> &binary_content,
        const std::string &content_type = "application/octet-stream");

    std::vector<uint8_t> get_binary_body() {
        return std::vector<uint8_t>(body.begin(), body.end());
    }

    bool is_streaming_response() { return is_streaming; };
    void set_streaming(std::function<void(std::ostream &)> stream_callback,
                       size_t content_length,
                       const std::string &content_type = "text/plain");

    void write_to_stream(std::ostream &os) const;

    std::string to_string() const;

  private:
    bool is_binary = false;
    bool is_streaming = false;

    std::function<void(std::ostream &)> stream_callback;
};

/////////////////////////////////
// HTTP Client
/////////////////////////////////

class HttpClient {
  public:
    HttpClient(std::string host_name, short int host_port)
        : hostname(host_name), port(host_port) {
        if (!resolve_hostname()) {
            throw std::runtime_error("Failed to resolve hostname: " + hostname);
        }
        client_log.write("Client " + std::to_string(client_fd) + " created");
    }

    signed int connect_to_server();
    HttpResponse send_request(HttpRequest request);
    void disconnect();

    ~HttpClient() {
        client_log.write("Removing client " + std::to_string(client_fd));
        if (is_connected) {
            close(client_fd);
            is_connected = false;
        }
    };

  private:
    Logger client_log = Logger("client.log");

    std::string hostname;
    std::string port_str;

    bool is_connected = false;
    int client_fd;
    struct sockaddr res;
    struct sockaddr_in addr;
    short int port;

    bool resolve_hostname();
};

/////////////////////////////////
// HTTP Server
/////////////////////////////////

class FdSet {
public:
    FdSet() { clear(); }

    void add(int fd) {
        FD_SET(fd, &read_fds);
        tracked_fds.insert(fd);
        max_fd = std::max(max_fd, fd);
    }

    void remove(int fd) {
        FD_CLR(fd, &read_fds);
        tracked_fds.erase(fd);
        if (fd == max_fd) {
            max_fd = 0;
            for (int tracked_fd : tracked_fds) {
                max_fd = std::max(max_fd, tracked_fd);
            }
        }
    }

    bool is_set(int fd) const {
        return FD_ISSET(fd, &read_fds);
    }

    void clear() {
        FD_ZERO(&read_fds);
        tracked_fds.clear();
        max_fd = 0;
    }

    int get_max_fd() const {
        return max_fd;
    }

    const fd_set* get_read_fds() const {
        return &read_fds;
    }

    fd_set* get_read_fds() {
        return &read_fds;
    }

private:
    fd_set read_fds;
    std::set<int> tracked_fds;
    int max_fd = 0;
};

class ClientSession {
public:
    ClientSession(int fd, const std::string& ip) : fd(fd), ip_address(ip), connected_time(std::chrono::steady_clock::now()) {}

    int get_fd() const { return fd; }
    const std::string& get_ip() const { return ip_address; }
    bool is_active() const { return active; }
    void set_active(bool state) { active = state; }

private:
    int fd;
    std::string ip_address;
    bool active = true;
    std::chrono::steady_clock::time_point connected_time;
    std::chrono::steady_clock::time_point last_activity;
};

class ServerEventLoop {
public:
    enum class EventType {
        NONE,
        NEW_CONNECTION,
        CLIENT_DATA,
        CLIENT_DISCONNECT,
        SERVER_COMMAND,
    };

    struct Event {
        EventType type;
        int fd;
        std::string data;
    };

    bool process_event(const Event& event) {
        switch(event.type) {
            case EventType::NEW_CONNECTION:
                return handle_new_connection(event);
            case EventType::CLIENT_DATA:
                return handle_client_data(event);
            case EventType::CLIENT_DISCONNECT:
                return handle_client_disconnect(event);
            case EventType::SERVER_COMMAND:
                return handle_server_command(event);
            default:
                return false;
        }
    }

private:
    bool handle_new_connection(const Event& event) { return true; }
    bool handle_client_data(const Event& event) { return true; }
    bool handle_client_disconnect(const Event& event) { return true; }
    bool handle_server_command(const Event& event) { return true; }
};

class SocketGuard {
public:
    explicit SocketGuard(int fd) : fd(fd) {}

    ~SocketGuard() {
        if (fd >= 0) {
            close(fd);
        }
    }

    int get() const { return fd; }
    int release() {
        int temp = fd;
        fd = -1;
        return temp;
    }

private:
    int fd;
};

class ClientManager {
public:
    void add_client(std::unique_ptr<ClientSession> client) {
        std::lock_guard<std::mutex> lock(mutex);
        clients[client->get_fd()] = std::move(client);
    }

    void remove_client(int fd) {
        std::lock_guard<std::mutex> lock(mutex);
        clients.erase(fd);
    }

    bool has_client(int fd) const {
        std::lock_guard<std::mutex> lock(mutex);
        return clients.find(fd) != clients.end();
    }

    size_t client_count() const {
        std::lock_guard<std::mutex> lock(mutex);
        return clients.size();
    }

    const std::unordered_map<int, std::unique_ptr<ClientSession>>& get_clients() const {
        return clients;
    }

    std::vector<int> get_client_fds() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<int> fds;
        fds.reserve(clients.size());
        for (const auto& client : clients) {
            fds.push_back(client.first);
        }
        return fds;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        clients.clear();
    }

private:
    mutable std::mutex mutex;
    std::unordered_map<int, std::unique_ptr<ClientSession>> clients;
};

class HttpServer {
public:
    Logger server_log = Logger("server.log");

    using RouteHandler = std::function<HttpResponse(const HttpRequest &)>;

    HttpServer(short int port) : host_port(port) {
        if (host_port < 1024) {
            throw std::runtime_error("Port number must not be below 1024");
        }
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        server_log.write("Server " + std::to_string(server_fd) + " started");

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

    ~HttpServer() {
        if (server_running.load()) {
            stop();
        }
        if (server_thread.joinable()) {
            server_thread.join();
        }
        server_log.write("Server shutting down");
    }

    void start();
    void stop();

    void get(const std::string &path, RouteHandler handler) {
        register_route("GET", path, handler);
    }

    void post(const std::string &path, RouteHandler handler) {
        register_route("POST", path, handler);
    }

    void register_route(const std::string &method,
                        const std::string &path,
                        RouteHandler handler) {
        routes.push_back({method, path, handler});
    }


private:
    std::atomic<bool> server_running{false};
    std::mutex client_mutex;
    std::condition_variable client_cv;
    std::thread server_thread;

    std::condition_variable queue_cv;

    struct Route {
        std::string method;
        std::string path;
        RouteHandler handler;
    };
    std::vector<Route> routes;

    FdSet fd_set;

    ClientManager client_manager;

    short int host_port;
    timeval timeout;
    int server_fd;
    std::string server_fd_str;
    int flags;

    struct sockaddr_in addr;

    void main_loop();

    void handle_new_connection();
    void handle_user_input();
    void handle_new_client(int client_fd, const sockaddr_in& client_addr);
    int handle_client_data(int client_fd);
    void handle_client_disconnect(int client_fd);
    bool check_connection(int client_fd);

    HttpResponse route_request(const HttpRequest &request);
};
