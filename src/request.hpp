#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "utils.hpp"
#include <string>
// #include <vector>
#include <unordered_map>
#include <sstream>
#include "json.hpp"
#include <unistd.h>      // close()
#include <sys/socket.h>  // socket(), shutdown(), SHUT_RDWR

using json = nlohmann::json;

inline void close_client(int sock) {
    shutdown(sock, SHUT_RDWR); // Cierre limpio de lectura y escritura
    close(sock);               // Libera descriptor
}

struct HttpResponse {
    int client_sock;
    int status_code = 200;
    std::string status_text = "OK";
    std::string http_version = "HTTP/1.1";

    std::string content_type = "text/html; charset=utf-8";
    std::vector<char> body;

    std::unordered_map<std::string, std::string> headers;
    std::vector<std::string> cookies;

    bool close_connection = true;

    HttpResponse() = default;

    HttpResponse(int code, std::string_view status, std::string_view content, std::string_view type = "text/html");

    void set_status(int code, std::string_view text);
    void set_status(int code);
    void set_json_body(const json& j);
    void set_body(std::string_view content);
    void set_content_type(std::string_view type);
    void add_header(std::string_view key, std::string_view value);
    void add_cookie(std::string_view cookie);

    std::string build_header_string(bool dev_mode = true) const;

    bool send(bool dev_mode = true) const;
};

struct ClientRequest {
    int client_sock;
    std::string client_ip;

    std::string method;
    std::string path;
    std::string http_version;

    std::unordered_map<std::string, std::string> headers;
    std::string raw_request;
    std::string body;
    std::string status;
    bool validated = false;

    // Headers comunes
    std::string host;
    std::string user_agent;
    std::string accept;
    std::string accept_language;
    std::string accept_encoding;
    std::string content_type;
    std::string content_length;
    std::string authorization;
    std::string connection;
    std::string upgrade_insecure_requests;
    std::string cache_control;
    std::string pragma;
    std::string referer;
    std::string cookie;

    // Parámetros
    std::unordered_map<std::string, std::string> get_params;
    std::unordered_map<std::string, std::string> post_params;

    // JSON body
    json json_body = json::object();
};

inline bool send_error(int client_sock, int code, bool send_file = false) {
    HttpResponse res;
    res.client_sock = client_sock;
    res.set_status(code);

    debug_log("Send error " + std::to_string(code));

    if (send_file) [[likely]] {
        std::string filepath = get_public_path() + "/error/" + std::to_string(code) + ".html";

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) [[unlikely]] {
            debug_log("Error file not found: " + filepath);
            std::string fallback = "<html><body><h1>" + res.status_text + "</h1><p>Error HTML no encontrado.</p></body></html>";
            res.set_body(fallback);
        } else {
            res.body.assign((std::istreambuf_iterator<char>(file)), {});
        }
    } else [[unlikely]] {
        std::string basic = "<html><body><h1>" + res.status_text + "</h1></body></html>";
        res.set_body(basic);
    }

    res.set_content_type("text/html; charset=utf-8");
    return res.send(config.dev_mode);
}

inline bool send_all(const HttpResponse& response, const char* buffer, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(response.client_sock, buffer + total_sent, length - total_sent, MSG_NOSIGNAL);
        if (sent == -1) {
            return false;
        }
        if (sent == 0) {
            break;
        }
        total_sent += sent;
    }
    return total_sent == length;
}

#endif // REQUEST_HPP