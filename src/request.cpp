#include "debug_log.hpp"
#include <regex>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <cerrno> 
#include <cstring>
#include <unordered_map>
#include <filesystem>

#define BUFFER_SIZE 8192

extern bool DEV_MODE;
extern std::string get_public_path();
extern bool validate_token(const std::string& token);
extern std::string get_mime_type(const std::string& path);
extern void handle_login(int& client_sock, const std::string& body);
extern void handle_edit_user(int& client_sock, const std::string& request, const std::string& body);

std::string read_request(int& client_sock) {
    debug_log("read_request");
    char buffer[BUFFER_SIZE] = {0};
    int bytes = read(client_sock, buffer, BUFFER_SIZE - 1);
    return std::string(buffer, bytes);
}

void close_client(int& client_sock){
    debug_log("Closed client_sock");
    close(client_sock);
}

void send_response(int& client_sock, const std::string& status, const std::vector<char>& body, const std::string& content_type, const std::string& extra_headers = "") {
    try{
        std::ostringstream header;
        header << "HTTP/1.1 " << status << "\r\n";
        header << "Content-Type: " << content_type << "\r\n";
        header << "Content-Length: " << body.size() << "\r\n";
        header << "X-Strapicarus: god\r\n";
        header << "X-Frame-Options: DENY\r\n";
        header << "Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self'; object-src 'none'; frame-ancestors 'none'\r\n";
        if (!DEV_MODE)
        {
            header << "Strict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n";
        }
        header << "Referrer-Policy: no-referrer\r\n";
        header << "Permissions-Policy: camera=(), microphone=(), geolocation=(), fullscreen=(), payment=(), usb=(), clipboard-write=()\r\n";
        if (!extra_headers.empty()){
            header << extra_headers;
        }
        header << "Connection: close\r\n\r\n";
        std::string header_str = header.str();
        debug_log("Sending response");
        ssize_t sent1 = send(client_sock, header_str.c_str(), header_str.size(), MSG_NOSIGNAL);
        if (sent1 == -1) {
            debug_log(std::string("Error al enviar cabecera: ") + std::strerror(errno));
            close_client(client_sock);
            return;
        }

        ssize_t sent2 = send(client_sock, body.data(), body.size(), MSG_NOSIGNAL); //MSG_NOSIGNAL para evitar SIGPIPE
        if (sent2 == -1) {
            debug_log(std::string("Error al enviar cuerpo: ") + std::strerror(errno));
            close_client(client_sock);
            return;
        }
    }catch(const std::exception& e) {
        debug_log(std::string("Excepción al manejar cliente: ") + e.what());
    } catch (...) {
        debug_log("Excepción desconocida al manejar cliente");
    }
    debug_log("Response sended");
}

void send_error(int& client_sock, int code) {
    static const std::unordered_map<int, std::string> status_map = {
        {400, "400 Bad Request"},
        {401, "401 Unauthorized"},
        {403, "403 Forbidden"},
        {404, "404 Not Found"},
        {405, "405 Method Not Allowed"},
        {500, "500 Internal Server Error"}
    };

    auto it = status_map.find(code);
    std::string status = (it != status_map.end()) ? it->second : "500 Internal Server Error";
    debug_log("send_error " + status);

    std::string filepath = get_public_path() + "/error/" + std::to_string(code) + ".html";
    debug_log("Error html path :" + filepath);
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        // Fallback si no encuentra el HTML
        debug_log("Error file not found: " + filepath);
        std::string fallback = "<html><body><h1>" + status + "</h1><p>Error HTML no encontrado.</p></body></html>";
        std::vector<char> fallback_vec(fallback.begin(), fallback.end());
        send_response(client_sock, status, fallback_vec, "text/html");
        return;
    }

    std::vector<char> content((std::istreambuf_iterator<char>(file)), {});
    send_response(client_sock, status, content, "text/html");
}

// --------------------- /run ---------------------
void handle_run_command(int& client_sock, const std::string& request, const std::string& body) {
    std::smatch match;
    std::regex token_rx("Authorization:\\s*(.*?)\r\n");

    if (!std::regex_search(request, match, token_rx)) {
        send_error(client_sock, 403);
        close_client(client_sock);
        return;
    }

    std::string token = match[1];
    if (!validate_token(token)) {
        send_error(client_sock, 403);
        close_client(client_sock);
        return;
    }

    std::regex cmd_rx("\"cmd\"\\s*:\\s*\"(.*?)\"");
    if (std::regex_search(body, match, cmd_rx)) {
        std::string cmd = match[1];
        std::string full_cmd = "screen -S mysession -X stuff '" + cmd + "\\n'";
        system(full_cmd.c_str());
        std::string json = "{\"mensaje\":\" comando enviado \"}";
        std::vector<char> body(json.begin(), json.end());
        send_response(client_sock, "200 OK", body, "application/json");
    } else {
        send_error(client_sock, 404);
    }

    close_client(client_sock);
}

void get_auth_cookie(const std::string& request, std::string& token){
    std::regex auth_regex(R"(Cookie:\s*(.*))");
    std::smatch match;

    if (std::regex_search(request, match, auth_regex)) {
        std::string cookies = match[1];
        auto eol = cookies.find_first_of("\r\n");
        if (eol != std::string::npos)
            cookies = cookies.substr(0, eol);

        std::regex token_regex(R"(Authorization=([^;]+))");
        std::smatch token_match;
        if (std::regex_search(cookies, token_match, token_regex)) {
            debug_log("TOKEN FOUND");
            token = token_match[1];
        } else {
            debug_log("Authorization token no encontrado en Cookie..");
            token = "";        }
    }
}

// --------------------- GET ---------------------
void handle_get_request(int& client_sock, const std::string& request) {
    std::smatch match;
    std::regex get_req_regex("GET (/[^ ]*) HTTP/1.1");
    debug_log(request);
    if (!std::regex_search(request, match, get_req_regex)) {
        debug_log("regex not found");
        send_error(client_sock, 400);
        close_client(client_sock);
        return;
    }
    std::string token;
    get_auth_cookie(request, token);
    std::string extra_header = "";
    std::string status = "200 OK";

    std::string path = match[1];
    std::string filename;
    debug_log("path:" + path);
    if (path == "/" || path == "/index" || path == "/index.html") {
        if(!token.empty() && validate_token(token)){
            debug_log("TOKEN FOUND - REDIRECTING...");
            extra_header = "Location: /panel\r\n";
            status = "302";
            filename = get_public_path() + "/panel/index.html";
        } else {
            debug_log("TOKEN NOT FOUND");
            filename = get_public_path() + "/index.html";
        }
        
    } else if (path == "panel" || path == "/panel" || path == "/panel/" || path == "/panel/index.html") {
        
        if (token.empty() || !validate_token(token)) {
            debug_log("Rejected token");
            send_error(client_sock, 401);
            close_client(client_sock);
            return;
        }

        filename = get_public_path() + "/panel/index.html";
    } else if (get_mime_type(path) == "Error") {
        send_error(client_sock, 404);
        debug_log("MIME file " + get_mime_type(filename));
        debug_log("Rejected file" + filename);
        close_client(client_sock);
        return;
    } else {
        filename = get_public_path() + path;
    }

    if (!std::filesystem::exists(filename)) {
        send_error(client_sock, 404);
        close_client(client_sock);
        return;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        send_error(client_sock, 500);
        close_client(client_sock);
        return;
    }

    std::vector<char> content((std::istreambuf_iterator<char>(file)), {});
    std::string mime = get_mime_type(filename);
    send_response(client_sock, status, content, mime, extra_header);
    close_client(client_sock);
}

// --------------------- POST ---------------------
void handle_post_request(int& client_sock, const std::string& request) {
    std::smatch match;
    std::regex post_req_regex("POST (/\\w+) HTTP/1.1");
    debug_log(request);
    if (!std::regex_search(request, match, post_req_regex)) {
        send_error(client_sock, 400);
        close_client(client_sock);
        return;
    }

    std::string endpoint = match[1];
    std::size_t body_pos = request.find("\r\n\r\n");
    if (body_pos == std::string::npos) {
        send_error(client_sock, 400);
        close_client(client_sock);
        return;
    }

    std::string body = request.substr(body_pos + 4);

    if (endpoint == "/login") {
        handle_login(client_sock, body);
    } else if (endpoint == "/run") {
        handle_run_command(client_sock, request, body);
    } else if (endpoint == "/edit_user") {
        handle_edit_user(client_sock, request, body);
    } else {
        send_error(client_sock, 404);
        close_client(client_sock);
    }
}

void handle_client(int& client_sock) {
    std::string request = read_request(client_sock);

    if (request.starts_with("GET ")) {
        handle_get_request(client_sock, request);
    } else if (request.starts_with("POST ")) {
        handle_post_request(client_sock, request);
    } else {
        send_error(client_sock, 405);
        close_client(client_sock);
    }
}