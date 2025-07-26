#include <emmintrin.h>  // SSE2
#include <regex>
#include <netinet/in.h>
#include <vector>
#include <cerrno> 
#include <cstring>
#include <unordered_map>
#include "utils.hpp"
#include "router.hpp"

// ========================
// Declaraciones externas
// ========================
extern bool validate_token(const std::string& token);
extern void handle_edit_user(int& client_sock, const std::string& request, const std::string& body);
// extern int connect_to_tmux_socket();
// extern std::string read_tmux_pane(int fd, const std::string& target_pane);

// ========================
// Restricciones
// ========================
static const std::vector<std::string> botKeywords = {
    "bot","crawl", "spider","slurp", "fetch", "scrape", "scanner", "archive", "httpclient", "wget", "libwww", "python", "scrapy",
    "aiohttp", "go-http-client", "okhttp", "java", "perl", "node.js", "nodejs", "node", "axios", "http_request2", "ruby", "feed", "go", "lib",
    "facebookexternalhit", "facebook", "meta", "discord", "discordbot", "slackbot", "telegrambot", "telegram", "whatsapp", "postmanruntime",
    "postman", "chatgpt", "openai", "gpt", "ahrefs", "semrush", "dotbot", "baiduspider", "yandex", "google", "gemeni", "grok", "duckduck",
    "mj12bot", "screaming", "frog", "bingpreview", "duckduckbot", "microsoft", "bing", "oracle", "sun", "http"
};

bool containsBotUserAgent(const std::string& userAgent) {
    std::string lowered;
    lowered.reserve(userAgent.size());
    lowered = to_lower(userAgent);
    for (const auto& keyword : botKeywords) {
        if (lowered.find(keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static const char* allowed_prefixes[] = {
    "/", "/panel", "/login", "/console", "/api", "/css", "/js"
};

static const char* sql_injection_signatures[] = {
    "'", "\"", "--", ";", "/*", "*/", "xp_", "exec", "union", "select"
};

static const std::regex safe_pattern("^/[a-zA-Z0-9/_\\-.]*$");

static bool has_consecutive_slashes(const char* path) {
    for (size_t i = 1; path[i] != '\0'; ++i) {
        if ((path[i] == '/' && path[i - 1] == '/') || (path[i] == '\\' && path[i - 1] == '\\')) {
            return true;
        }
    }
    return false;
}

// SIMD versión para caracteres de control (0x00 - 0x1F)
static bool contains_control_chars_simd(const char* s, size_t len) {
    const __m128i control_mask = _mm_set1_epi8(0x1F);
    const __m128i zero = _mm_setzero_si128();

    for (size_t i = 0; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i));
        __m128i is_control = _mm_cmplt_epi8(chunk, zero);                // signed < 0
        is_control = _mm_or_si128(is_control, _mm_cmplt_epi8(chunk, control_mask));

        if (_mm_movemask_epi8(is_control)) {
            return true;
        }
    }

    // Final bytes
    for (size_t i = len & ~15; i < len; ++i) {
        if (static_cast<unsigned char>(s[i]) < 0x20) {
            return true;
        }
    }

    return false;
}

static bool is_path_safe(const std::string& raw_path) {

    if (__builtin_expect(raw_path.empty() || raw_path.size() > 40, 0)) {
        throw std::runtime_error("Path vacio o muy largo" + raw_path);
    }

    char path[41] = {};
    size_t j = 0;

    for (size_t i = 0; i < raw_path.length() && j < sizeof(path) - 1; ++i) {
        if (raw_path[i] == '%' && i + 2 < raw_path.size()) {
            char hex[3] = { raw_path[i + 1], raw_path[i + 2], '\0' };
            path[j++] = static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        } else {
            path[j++] = raw_path[i];
        }
    }

    path[j] = '\0';

    if (__builtin_expect(path[0] != '/', 0)) {
        throw std::runtime_error("Path index 0 diferente a root /");
    }

    if (__builtin_expect(contains_control_chars_simd(path, j),0)) {
        throw std::runtime_error("Path contiene caracteres de control");
    }

    if (__builtin_expect(strstr(path, "..") || strchr(path, '~') || strchr(path, '\\') || strchr(path, '%'),0)) {
        throw std::runtime_error("Path contiene .., ~ o \\");
    }

    if (__builtin_expect(has_consecutive_slashes(raw_path.c_str()),0)) {
        throw std::runtime_error("Path consecutivos slashes //");
    }

    if (__builtin_expect(!std::regex_match(path, safe_pattern),0)) {
        throw std::runtime_error("Path peligroso detectado");
    }

    for (const char* sig : sql_injection_signatures) {
        if (__builtin_expect(strstr(path, sig) != nullptr, 0)) {
            throw std::runtime_error("sql injection detectado");
        }
    }

    for (const char* prefix : allowed_prefixes) {
        size_t len = std::strlen(prefix);
        if (__builtin_expect(std::strncmp(path, prefix, len) == 0, 1)) {
            return true;
        }
    }

    return false;
}

// ========================
// Manejo de peticiones
// ========================

static const std::vector<char> empty_body;

std::string safe_strerror(int errnum) {
    char buffer[256];
#if defined(__GLIBC__) && !_GNU_SOURCE
    strerror_r(errnum, buffer, sizeof(buffer));
    return std::string(buffer);
#else
    return std::string(strerror_r(errnum, buffer, sizeof(buffer)));
#endif
}

inline std::string get_status_text(int code) {
    switch (code) {
        [[likely]] case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        [[likely]] case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        [[likely]] case 401: return "Unauthorized";
        case 403: return "Forbidden";
        [[likely]] case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 429: return "Too Many Requests";
        [[likely]] case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        [[unlikely]] default:  return "Unknown Status";
    }
}

static const std::string get_cookie_value(const std::string& cookie_header, const std::string& key) {
    size_t pos = 0;
    const size_t len = cookie_header.length();

    while (pos < len) {
        while (pos < len && (cookie_header[pos] == ' ' || cookie_header[pos] == ';')) ++pos;

        size_t equal_pos = cookie_header.find('=', pos);
        if (equal_pos == std::string::npos) break;

        std::string name = cookie_header.substr(pos, equal_pos - pos);

        if (name == key) {
            size_t value_start = equal_pos + 1;
            size_t value_end = cookie_header.find(';', value_start);
            if (value_end == std::string::npos) value_end = len;
            return cookie_header.substr(value_start, value_end - value_start);
        }

        // Avanzar al siguiente cookie
        pos = cookie_header.find(';', equal_pos);
        if (pos == std::string::npos) break;
        ++pos;
    }

    return "";
}

///////////////////////////////////
// Métodos de HttpResponse
//////////////////////////////////
HttpResponse::HttpResponse(int code, std::string_view status, std::string_view content, std::string_view type)
    : status_code(code), status_text(status), content_type(type) {
    body.assign(content.begin(), content.end());
}

// void HttpResponse::set_status(int code, std::string_view text) {
//     status_code = code;
//     status_text = text;
// }

void HttpResponse::set_status(int code) {
    status_code = code;
    switch (code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 400: status_text = "Bad Request"; break;
        case 401: status_text = "Unauthorized"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
}

void HttpResponse::set_json_body(const json& j) {
    std::string str = j.dump(); // serializa el JSON a string
    set_body(str);
    set_content_type("application/json");
}

void HttpResponse::set_body(std::string_view content) {
    body.assign(content.begin(), content.end());
}

void HttpResponse::set_content_type(std::string_view type) {
    content_type = type;
}

void HttpResponse::add_header(std::string_view key, std::string_view value) {
    headers[std::string(key)] = std::string(value);
}

void HttpResponse::add_cookie(std::string_view cookie) {
    cookies.emplace_back(cookie);
}

std::string HttpResponse::build_header_string(bool dev_mode) const {
    std::ostringstream out;

    out << http_version << " " << status_code << " " << status_text << "\r\n";
    out << "Content-Type: " << content_type << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";

    out << "X-Strapicarus: god\r\n";
    out << "X-Frame-Options: DENY\r\n";
    out << "Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self'; object-src 'none'; frame-ancestors 'none'\r\n";

    if (!dev_mode) {
        out << "Strict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n";
    }

    out << "Referrer-Policy: no-referrer\r\n";
    out << "Permissions-Policy: camera=(), microphone=(), geolocation=(), fullscreen=(), payment=(), usb=(), clipboard-write=()\r\n";

    for (const auto& [key, value] : headers) {
        out << key << ": " << value << "\r\n";
    }

    for (const auto& cookie : cookies) {
        out << "Set-Cookie: " << cookie << "\r\n";
    }

    out << (close_connection ? "Connection: close\r\n" : "Connection: keep-alive\r\n");
    out << "\r\n";
    return out.str();
}

bool HttpResponse::send(bool dev_mode) const {
    std::string header_str = build_header_string(dev_mode);

    if (!send_all(*this, header_str.data(), header_str.size())) {
        throw std::runtime_error("Error al enviar cabecera: " + safe_strerror(errno));
    }

    if (!body.empty()) {
        if (!send_all(*this, body.data(), body.size())) {
            throw std::runtime_error("Error al enviar cuerpo: " + safe_strerror(errno));
        }
    }
    close_client(client_sock);
    return true;
}


///////////////////////////////////
// Manejo del Requeest.
//////////////////////////////////
static const std::unordered_map<std::string, std::string> parse_urlencoded_params(const std::string& s) {
    std::unordered_map<std::string, std::string> result;
    std::istringstream stream(s);
    std::string pair;

    while (std::getline(stream, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq == std::string::npos) continue;
        std::string key = pair.substr(0, eq);
        std::string value = pair.substr(eq + 1);
        std::replace(value.begin(), value.end(), '+', ' '); // Simplificado
        result[key] = value;
    }
    return result;
}

static bool parse_client_request(int& client_sock, const std::string& client_ip, ClientRequest& request_out) {
    if (__builtin_expect(client_sock < 0, 0)) {
        throw std::runtime_error("Socket inválido");
        
    }
    char buffer[config.max_req_buf_size];
    ssize_t bytes_received = recv(client_sock, buffer, config.max_req_buf_size - 1, 0);

    if (__builtin_expect(bytes_received <= 0, 0)) {
        throw std::runtime_error("Error leyendo bytes_received del socket o conexión cerrada");
    }

    if (__builtin_expect(bytes_received == config.max_req_buf_size - 1, 0)) { // buffer lleno, es anormal...
        // Posible truncamiento: el cliente podría haber enviado más de lo permitido.
        throw std::runtime_error("Petición excede el tamaño máximo permitido (8KB)");
    }

    buffer[bytes_received] = '\0';

    std::string request_str(buffer);
    request_out.client_sock = client_sock;
    request_out.client_ip = client_ip;
    request_out.raw_request = request_str;

    std::istringstream stream(request_str);

    // Parsear primera línea
    std::string line;
    std::string full_path;

    if (!std::getline(stream, line)) {
        throw std::runtime_error("LÍNEA NO VALIDA");
    }

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    std::istringstream first_line(line);
    if (!(first_line >> request_out.method >> full_path >> request_out.http_version)) {
        throw std::runtime_error("FORMATOP INVÁLIDO");
    }

    if(!is_path_safe(full_path)){
        throw std::runtime_error("PATH no seguro...");
    } 

    // Separar path y query string, para futuros usos
    size_t qpos = full_path.find('?');
    if (qpos != std::string::npos) {
        request_out.path = full_path.substr(0, qpos);
        std::string query = full_path.substr(qpos + 1);
        request_out.get_params = parse_urlencoded_params(query);
    } else {
        request_out.path = full_path;
    }

    // Parsear headers
    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        trim_header_line(key);
        trim_header_line(value);
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end()); // por si acaso...

        request_out.headers[key] = value;

        if (key == "Host") {
            request_out.host = value;
        } else if (key == "User-Agent") {
            if (containsBotUserAgent(value)){
                throw std::runtime_error("User-Agent NO PERMITIDO...");
            } 
            request_out.user_agent = value;
        } else if (key == "Accept") {
            request_out.accept = value;
        } else if (key == "Content-Type") {
            request_out.content_type = value;
        } else if (key == "Content-Length") {
            request_out.content_length = value;
        } else if (key == "Cookie") {
            debug_log("Cookie: " + value);
            request_out.cookie = value;
            request_out.authorization = get_cookie_value(request_out.cookie, "Authorization");
            if (validate_token(request_out.authorization)){
                request_out.validated = true;
            } 
        } else if (key == "Connection") {
            request_out.connection = value;
        } else if (key == "Upgrade-Insecure-Requests") {
            request_out.upgrade_insecure_requests = value;
        } else if (key == "Cache-Control") {
            request_out.cache_control = value;
        } else if (key == "Pragma") {
            request_out.pragma = value;
        } else if (key == "Referer") {
            request_out.referer = value;
        } 
    }

    // Leer body (si hay Content-Length)
    int content_length = 0;
    if (!request_out.content_length.empty()) {
        try {
            content_length = std::stoi(request_out.content_length);
        } catch (...) {
            throw std::runtime_error("Fallṕ parse content_length");
        }
    }

    if (content_length > 0) {
        std::string remaining = stream.str().substr(stream.tellg());
        if (remaining.size() >= static_cast<size_t>(content_length)) {
            request_out.body = remaining.substr(0, content_length);

            if (request_out.content_type.find("application/x-www-form-urlencoded") != std::string::npos) { // usos futuros
                request_out.post_params = parse_urlencoded_params(request_out.body);
            }

            if (request_out.content_type.find("application/json") != std::string::npos) {
                try {
                    debug_log(request_out.body);
                    request_out.json_body = json::parse(request_out.body);
                } catch (const std::exception& e) {
                    throw std::runtime_error(std::string("Error al parsear JSON: ") + e.what());
                }
            }else{
                throw std::runtime_error("Se esperaba formato json");
            }
        }
    }
    return true;
}

void handle_get_request(const ClientRequest& req) {
    std::string filename;
    HttpResponse res;
    res.client_sock = req.client_sock;

    if (has_dot(req.path))
    {
        if (get_mime_type(req.path) == "Error"){
            send_error(req.client_sock, 404, false);
            return;
        }
        filename = get_public_path() + req.path;
        res.set_content_type(get_mime_type(filename));
    }else{
        if (req.path == "/") [[likely]] {
            if (req.validated) [[likely]] {
                res.set_status(302);
                res.add_header("Location", "/panel");
                filename = get_public_path() + "/panel/index.html";
            } else [[unlikely]] {
                filename = get_public_path() + "/index.html";
            }
        } else if (req.path == "/panel") [[likely]] {
            if (!req.validated) [[unlikely]] {
                send_error(req.client_sock, 401, true);
                return;
            } else [[likely]] {
                filename = get_public_path() + "/panel/index.html";
            }
        }
        res.set_content_type(get_mime_type(filename));
    }

    if (!std::filesystem::exists(filename)) [[unlikely]] {
        send_error(req.client_sock, 404, true);
        return;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) [[unlikely]] {
        send_error(req.client_sock, 500, true);
        return;
    }

    res.body.assign((std::istreambuf_iterator<char>(file)), {});
    if(res.send()){
        debug_log("Response Sent...\n\n\n");
    }
}

void handle_post_request(const ClientRequest& request) {
    if (request.path == "/login")
    {
        if (!dispatch_route(request)) {
            debug_log("POST dispatch_route fail...");
        }
    } else if(!request.validated){
        send_error(request.client_sock, 403, true);
    }else{
        if (!dispatch_route(request)) {
            send_error(request.client_sock, 404, false);
        }
    }    
}

void handle_report_request(const ClientRequest& request) {

    if (!dispatch_route(request)) {
        debug_log("REPORT dispatch_route fail...");
    }
}

void handle_client(int client_sock) {

    ClientRequest request;
    if(!parse_client_request(client_sock, "0.0.0.0", request)){
        throw std::runtime_error("Fallo Parseo de request");
    }
    debug_log("\n_____________________________________________________________________");
    debug_log("\n\n" + request.raw_request);
    try{
        if (request.method == "GET") {
            handle_get_request(request);
        } else if (request.method == "POST") {
            handle_post_request(request);
        } else if (request.method == "REPORT") {
            if (request.validated) {
                handle_report_request(request);
            } else {
                send_error(request.client_sock, 401, true);
            }
        } else {
            send_error(request.client_sock, 405, true);
        }
    } catch (const std::runtime_error& e) {
        debug_log(e.what());
        send_error(request.client_sock, 500, true);
    }
}