// Copyright [2025] <Nicolas Selig>
//
//

#pragma once
#include "string_utils.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

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
    std::string status_text;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse(int code = 200, std::string text = "OK",
                 std::string vers = "HTTP/1.1") {
        status_code = code;
        status_text = text;
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
    void set_streaming(
        std::function<void(std::ostream&)> stream_callback,
        size_t content_length,
        const std::string &content_type = "text/plain"
    );
    void write_to_stream(std::ostream &os) const;    

private:
    bool is_binary = false;
    bool is_streaming = false;

    std::function<void(std::ostream&)> stream_callback;
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
    HttpClient(std::string host, short int host_port) {
        if (host == "localhost") {
            ip4_array[0] = 127;
            ip4_array[1] = 0;
            ip4_array[2] = 0;
            ip4_array[3] = 1;
        }
        port = host_port;
    }

    void connect();

    HttpResponse send_request(HttpRequest request);

private:
    std::array<char, 4> ip4_array;
    short int port;
};
