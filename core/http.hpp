// Copyright [2025] <Nicolas Selig>
//
//

#pragma once
#include <algorithm>
#include <map>
#include <type_traits>
#include <string>

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

    void create_get(std::string request_uri,
                    std::map<std::string, std::string> parameters = {});

    std::string to_string() const;

    void create_put(std::string& request_uri,
                    const std::map<std::string, std::string> parameters = {},
                    std::string content_type = "application/x-www-form-urlencoded");

    template<typename T>
    void create_post(const std::string& request_uri,
                    const T& data,
                    const std::string& content_type = "application/json");

    void create_post(const std::string& request_uri,
                     const std::map<std::string, std::string>& form_data);
};

template<typename T>
void HttpRequest::create_post(const std::string& request_uri, const T& data, 
                              const std::string& content_type) {
    method = "POST";
    path = request_uri;
    version = "HTTP/1.1";

    std::string content;

    if constexpr (std::is_same_v<T, std::string>) {
        content = data;
    } else {
        static_assert(std::is_same_v<T, std::string>, "Unsupported data type for POST request");
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

    static HttpResponse switching_protocol();
    static HttpResponse ok(const std::string &body = "");
    static HttpResponse not_found(const std::string &resource = "");
    static HttpResponse server_error(const std::string &message = "");
    static HttpResponse bad_request(const std::string &message = "");

    bool has_header(const std::string &name) const;
    void set_header(const std::string &key, const std::string &value);
    std::string get_header(const std::string &name) const;

    void set_body(const std::string &content,
                  const std::string &content_type = "text/plain") {
        body = content;
        set_header("Content-Type", content_type);
        set_header("Content-Length", std::to_string(content.length()));
    }
    std::string to_string() const;
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
