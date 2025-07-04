// #include "debug_log.hpp"
#include <emmintrin.h>  // SSE2
#include <regex>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <cerrno> 
#include <cstring>
#include <unordered_map>
#include <filesystem>
#include "utils.hpp"
#include "tmux_manager.hpp"
#include <json.hpp>

using json = nlohmann::json;


// ========================
// Declaraciones externas
// ========================
extern bool validate_token(const std::string& token);
extern std::string get_mime_type(const std::string& path);
extern void handle_login(int& client_sock, const std::string& body);
extern void handle_edit_user(int& client_sock, const std::string& request, const std::string& body);
extern int connect_to_tmux_socket();
extern std::string read_tmux_pane(int fd, const std::string& target_pane);
extern bool dispatch_route(int& client_sock, const std::string& method, const std::string& path, const std::string& body, const bool& validated);


// ========================
// Restricciones
// ========================

static const std::regex userAgentRegex(R"(^User-Agent:\s*(.+)\r?$)", std::regex::icase | std::regex::multiline);
static const std::regex botPattern(R"((bot|crawl|spider|slurp|fetch|scrape|scanner|archiver|httpclient|wget|curl|libwww|python|scrapy|aiohttp|go-http-client|okhttp|java|perl|node\.js|axios|http_request2|ruby|feed|facebookexternalhit|discordbot|slackbot|telegrambot|WhatsApp|PostmanRuntime|chatgpt|openai|gpt|ahrefs|semrush|dotbot|baiduspider|yandex|mj12bot|screaming frog|bingpreview|duckduckbot))", std::regex::icase);

bool containsBotUserAgent(const std::string& httpRequest) {
    std::smatch match;
    if (std::regex_search(httpRequest, match, userAgentRegex)) {
        std::string userAgent = match[1].str();
        return std::regex_search(userAgent, botPattern);
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
    debug_log("Is path safe?....");
    debug_log("Path: " + raw_path);
    if (__builtin_expect(raw_path.empty() || raw_path.size() > 40, 0)) {
        debug_log("Path empty or too long");
        return false;
    }

    char path[64] = {};
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
    debug_log(path);
    if (__builtin_expect(path[0] != '/', 0)) {
        debug_log("Path start diferent to root /");
        return false;
    }

    if (__builtin_expect(contains_control_chars_simd(path, j),0)) {
        debug_log("Path contains controls chars");
        return false;
    }

    if (__builtin_expect(strstr(path, "..") || strchr(path, '~') || strchr(path, '\\') || strchr(path, '%'),0)) {
        debug_log("Path contains .., ~ or \\");
        return false;
    }

    if (__builtin_expect(has_consecutive_slashes(raw_path.c_str()),0)) {
        debug_log("Path has consecutive slashes //");
        return false;
    }

    if (__builtin_expect(!std::regex_match(path, safe_pattern),0)) {
        debug_log("Path do not meet regex safe pathern");
        return false;
    }

    // SQL injection signatures (branch-hinting on unlikely match)
    for (const char* sig : sql_injection_signatures) {
        if (__builtin_expect(strstr(path, sig) != nullptr, 0)) {
            debug_log("Path have sql injection pathern");
            return false;
        }
    }

    // Allowed prefixes (hint: likely match)
    for (const char* prefix : allowed_prefixes) {
        size_t len = std::strlen(prefix);
        if (__builtin_expect(std::strncmp(path, prefix, len) == 0, 1)) {
            debug_log("Path has allowed_prefixes");
            return true;
        }
    }
    debug_log("Path do not find any malicious pather");

    return false;
}

// ========================
// Manejo de peticiones
// ========================

struct ClientRequest {
    int client_sock;
    std::string client_ip;

    std::string method;
    std::string path;
    std::string http_version;

    std::unordered_map<std::string, std::string> headers;
    std::string raw_request;
    std::string body;

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

// Función para parsear parámetros tipo key=value&key2=value2
static std::unordered_map<std::string, std::string> parse_urlencoded_params(const std::string& s) {
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

// Función para parsear el request
static bool parse_client_request(int client_sock, const std::string& client_ip, ClientRequest& request_out) {
    char buffer[config.max_req_buf_size];
    ssize_t bytes_received = recv(client_sock, buffer, config.max_req_buf_size - 1, 0);

    if (bytes_received <= 0) return false;
    buffer[bytes_received] = '\0';

    std::string request_str(buffer);
    request_out.client_sock = client_sock;
    request_out.client_ip = client_ip;
    request_out.raw_request = request_str;

    std::istringstream stream(request_str);
    std::string line;

    // Parsear primera línea
    std::string full_path;
    if (!std::getline(stream, line)) return false;
    std::istringstream first_line(line);
    if (!(first_line >> request_out.method >> full_path >> request_out.http_version)) {
        return false;
    }

    // Separar path y query string
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

        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        request_out.headers[key] = value;

        if (key == "Host") request_out.host = value;
        else if (key == "User-Agent") request_out.user_agent = value;
        else if (key == "Accept") request_out.accept = value;
        // else if (key == "Accept-Language") request_out.accept_language = value;
        // else if (key == "Accept-Encoding") request_out.accept_encoding = value;
        else if (key == "Content-Type") request_out.content_type = value;
        else if (key == "Content-Length") request_out.content_length = value;
        else if (key == "Authorization") request_out.authorization = value;
        else if (key == "Connection") request_out.connection = value;
        else if (key == "Upgrade-Insecure-Requests") request_out.upgrade_insecure_requests = value;
        else if (key == "Cache-Control") request_out.cache_control = value;
        else if (key == "Pragma") request_out.pragma = value;
        else if (key == "Referer") request_out.referer = value;
        else if (key == "Cookie") request_out.cookie = value;
    }

    // Leer body (si hay Content-Length)
    int content_length = 0;
    if (!request_out.content_length.empty()) {
        try {
            content_length = std::stoi(request_out.content_length);
        } catch (...) {}
    }

    if (content_length > 0) {
        std::string remaining = stream.str().substr(stream.tellg());
        if (remaining.size() >= static_cast<size_t>(content_length)) {
            request_out.body = remaining.substr(0, content_length);

            // application/x-www-form-urlencoded
            if (request_out.content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
                request_out.post_params = parse_urlencoded_params(request_out.body);
            }

            // application/json
            if (request_out.content_type.find("application/json") != std::string::npos) {
                try {
                    request_out.json_body = json::parse(request_out.body);
                } catch (const std::exception& e) {
                    std::cerr << "Error al parsear JSON: " << e.what() << "\n";
                    request_out.json_body = json::object();
                }
            }
        }
    }

    return true;
}

static const std::regex get_req_regex("GET (/[^ ]*) HTTP/1.1");
static const std::vector<char> empty_body;

static const std::string read_request(const int& client_sock) {
    if (__builtin_expect(client_sock < 0, 0)) {
        debug_log("Socket inválido");
        return "Error";
    }
    char buffer[config.max_req_buf_size];
    int bytes = read(client_sock, buffer, config.max_req_buf_size - 1);

    if (__builtin_expect(bytes <= 0, 0)) {
        throw std::runtime_error("Error leyendo del socket o conexión cerrada");
    }

    if (__builtin_expect(bytes == config.max_req_buf_size - 1, 0)) { // buffer lleno, es anormal según el diseño.
        // Posible truncamiento: el cliente podría haber enviado más de lo permitido.
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
        throw std::runtime_error("Petición excede el tamaño máximo permitido (8KB)");
    }

    return std::string(buffer, bytes);
}

void close_client(const int& client_sock){
    debug_log("Close client_sock");
    shutdown(client_sock, SHUT_RDWR);
    close(client_sock);
}

void send_response(const int& client_sock, const std::string& status, const std::vector<char>& body, const std::string& content_type, const std::string& extra_headers = "") {
    
    try{
        std::ostringstream header;

        header << "HTTP/1.1 " << status << "\r\n";
        header << "Content-Type: " << content_type << "\r\n";
        header << "Content-Length: " << body.size() << "\r\n";
        header << "X-Strapicarus: god\r\n";
        header << "X-Frame-Options: DENY\r\n";
        header << "Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self'; object-src 'none'; frame-ancestors 'none'\r\n";

        if (__builtin_expect(!config.dev_mode, 0)) //cambiar 0 a 1 para producción o mejor eliminar el if, solo si se espera cominucación SSL
        {
            header << "Strict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n";
        }

        header << "Referrer-Policy: no-referrer\r\n";
        header << "Permissions-Policy: camera=(), microphone=(), geolocation=(), fullscreen=(), payment=(), usb=(), clipboard-write=()\r\n";

        if (!extra_headers.empty()) {
            header << extra_headers;
            if (__builtin_expect(extra_headers.back() != '\n',0)) {
                header << "\r\n";
            }
        }

        header << "Connection: close\r\n\r\n";
        std::string header_str = header.str();

        debug_log("Sending response");
        ssize_t sent1 = send(client_sock, header_str.c_str(), header_str.size(), MSG_NOSIGNAL);

        if (__builtin_expect(sent1 == -1, 0)) {
            debug_log(std::string("Error al enviar cabecera: ") + std::strerror(errno));
            close_client(client_sock);
            return;
        }

        ssize_t sent2 = send(client_sock, body.data(), body.size(), MSG_NOSIGNAL); //MSG_NOSIGNAL para evitar cierre silencioso por SIGPIPE
        if (__builtin_expect(sent2 == -1, 0)) {
            debug_log(std::string("Error al enviar cuerpo: ") + std::strerror(errno));
            close_client(client_sock);
            return;
        }

        // debug_log("Response HEADER: \n" + header.str());
        // debug_log(std::string(body.begin(), body.end()));

    }catch(const std::exception& e) {
        debug_log(std::string("Excepción al manejar cliente: ") + e.what());
    } catch (...) {
        debug_log("Excepción desconocida al manejar cliente");
    }

    close_client(client_sock);
}

void send_error(const int& client_sock, const int& code, const bool& send_file = false) {
    static const std::unordered_map<int, std::string> status_map = {
        {400, "400 Bad Request"},
        {401, "401 Unauthorized"},
        {403, "403 Forbidden"},
        {404, "404 Not Found"},
        {405, "405 Method Not Allowed"},
        {408, "408 Request Timeout"},
        {409, "409 Conflict"},
        {410, "410 Gone"},
        {418, "418 I'm a teapot"},
        {429, "429 Too Many Requests"},
        {451, "451 Unavailable For Legal Reasons"},
        {500, "500 Internal Server Error"},
        {501, "501 Not Implemented"},
        {502, "502 Bad Gateway"},
        {503, "503 Service Unavailable"},
        {504, "504 Gateway Timeout"}
    };

    auto it = status_map.find(code);
    std::string status = (it != status_map.end()) ? it->second : "500 Internal Server Error";
    debug_log("send_error " + status);
    if(__builtin_expect(send_file, 0)) {
        std::string filepath = get_public_path() + "/error/" + std::to_string(code) + ".html";
        debug_log("Error html path :" + filepath);
        std::ifstream file(filepath, std::ios::binary);

        if (!file.is_open()) [[unlikely]] {
            // Fallback si no encuentra el HTML
            debug_log("Error file not found: " + filepath);
            std::string fallback = "<html><body><h1>" + status + "</h1><p>Error HTML no encontrado.</p></body></html>";
            std::vector<char> fallback_vec(fallback.begin(), fallback.end());
            send_response(client_sock, status, fallback_vec, "text/html");
            return;
        }else [[likely]] {
            std::vector<char> content((std::istreambuf_iterator<char>(file)), {});
            send_response(client_sock, status, content, "text/html");    
        }
    }else [[likely]]{
         send_response(client_sock, status, empty_body, "text/html");
    }
    
}


void get_auth_cookie(const std::string& request, std::string& token){
    std::regex auth_regex(R"(Cookie:\s*(.*))");
    std::smatch match;

    if (std::regex_search(request, match, auth_regex)) [[likely]] {
        std::string cookies = match[1];
        auto eol = cookies.find_first_of("\r\n");
        if (eol != std::string::npos){
                cookies = cookies.substr(0, eol);
        }
        std::regex token_regex(R"(Authorization=([^;]+))");
        std::smatch token_match;
        if (std::regex_search(cookies, token_match, token_regex)) [[likely]] {
            debug_log("AUTH cookie FOUND");
            token = token_match[1];
        } else  [[unlikely]] {
            debug_log("Authorization token no encontrado en Cookie..");
            token = "";
        }
    }
    // Aquí agregar un contador de peticiones por ip para realizar baneo si excede 5
}

// void handle_console_request(int& client_sock){//, const std::string& request) {
//     try {
//         int tmux_fd = connect_to_tmux_socket();
//         std::string pane_output = read_tmux_pane(tmux_fd, "txt2scrn:0.0");
//         close(tmux_fd);
//         std::vector<char> body(pane_output.begin(), pane_output.end());
//         send_response(client_sock, "200 OK", body, "text/plain; charset=utf-8");
//     } catch (const std::exception& ex) {
//         debug_log(ex.what());
//         send_error(client_sock, 500);
//     }
//     close_client(client_sock);
// }

// --------------------- GET ---------------------
void handle_get_request(int& client_sock, const std::string& request, 
                        bool& validated, std::string& extra_header, 
                        std::string& status) 
{
    std::smatch match;
    if (!std::regex_search(request, match, get_req_regex)) {
        debug_log("regex not found");
        send_error(client_sock, 400, true);
        close_client(client_sock);
        return;
    }

    std::string path = match[1];
    if(!is_path_safe(path)) send_error(client_sock, 400, true);

    std::string filename;
    debug_log("path:" + path);
    if (path == "/" || path == "/index" || path == "/index.html" || path == "/login") [[likely]] {
        if(validated) [[likely]] {
            debug_log("TOKEN FOUND - REDIRECTING...");
            extra_header = "Location: /panel\r\n";
            status = "302 FOUND";
            filename = get_public_path() + "/panel/index.html";
        } else [[unlikely]] {
            debug_log("TOKEN NOT FOUND");
            filename = get_public_path() + "/index.html";
        }
        
    } else if (path == "panel" || path == "/panel" || path == "/panel/" || path == "/panel/index.html") [[unlikely]] {
        
        if (!validated) [[unlikely]] {
            //agregar contador por ip para baneos
            debug_log("Rejected token");
            send_error(client_sock, 401, true);
            close_client(client_sock);
            return;
        } else [[likely]] {
            filename = get_public_path() + "/panel/index.html";
        }
    } else if (get_mime_type(path) == "Error") [[unlikely]] {
        //Contador para baneos
        send_error(client_sock, 404, false);
        debug_log("MIME file " + get_mime_type(filename));
        debug_log("Rejected file" + filename);
        close_client(client_sock);
        return;
    } else [[likely]] {
        filename = get_public_path() + path;
    }

    if (!std::filesystem::exists(filename)) [[unlikely]] {
        //contador baneos
        send_error(client_sock, 404, false);
        close_client(client_sock);
        return;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) [[unlikely]] {
        //contador baneo
        send_error(client_sock, 500, true);
        close_client(client_sock);
        return;
    }

    std::vector<char> content;
    // if (!is_head) [[unlikely]] {
        //contador baneo
        content.assign((std::istreambuf_iterator<char>(file)), {});
    // }
    std::string mime = get_mime_type(filename);
    send_response(client_sock, status, content, mime, extra_header);
    // close_client(client_sock);
}

void handle_post_request(int& client_sock, const std::string& request, const bool& validated) {
    std::smatch match;
    std::regex post_req_regex("POST (/[^ ]+) HTTP/1.1");
    if (!std::regex_search(request, match, post_req_regex)) {
        debug_log("WORNG POST URL REGEX...");
        send_error(client_sock, 400, false);
        close_client(client_sock);
        return;
    }

    std::string endpoint = match[1];
    std::size_t body_pos = request.find("\r\n\r\n");
    std::string body = (body_pos != std::string::npos) ? request.substr(body_pos + 4) : "";
    debug_log("POST BODY: " + body);
    if (!dispatch_route(client_sock, "POST", endpoint, body, validated)) {
        debug_log("dispatch_route ERROR...");
        send_error(client_sock, 404, false);
        close_client(client_sock);
    }
}

void handle_report_request(int& client_sock, const std::string& request, const bool& validated) {
    std::smatch match;
    std::regex post_req_regex("REPORT (/[^ ]+) HTTP/1.1");
    if (!std::regex_search(request, match, post_req_regex)) {
        send_error(client_sock, 400, false);
        close_client(client_sock);
        return;
    }

    std::string endpoint = match[1];
    std::size_t body_pos = request.find("\r\n\r\n");
    std::string body = (body_pos != std::string::npos) ? request.substr(body_pos + 4) : "";

    if (!dispatch_route(client_sock, "REPORT", endpoint, body, validated)) {
        send_error(client_sock, 404, false);
    }
    close_client(client_sock);
}

void handle_client(int& client_sock) {
    std::string request = read_request(client_sock);
    debug_log("\n\n" + request + "\n\n");
    if (containsBotUserAgent(request)) return;
    
    std::string token;
    bool validated = false;
    std::string status = "200 OK";
    get_auth_cookie(request, token);
    std::string extra_header = "";
    if(!token.empty() && validate_token(token)) validated = true;

    if (request.starts_with("OPTIONS") || request.starts_with("HEAD")) {
        // std::string extra_headers;                                               ///////////////////////////////////
        // extra_headers += "Access-Control-Allow-Methods: GET, POST, REPORT\r\n";  ////                Evitar Cabeceras 
        // extra_headers += "Access-Control-Allow-Headers: Content-Type\r\n";       ////  Access-Control-* Relacionadas con CORS, CORS no se usa.
        // extra_headers += "Access-Control-Max-Age: 600\r\n"; // cache preflight   ///////////////////////////////////
        return send_response(client_sock, "204 No Content", empty_body, "", "");
    }
    if (request.starts_with("GET")) {
        debug_log("Recieved GET");
        handle_get_request(client_sock, request, validated, extra_header, status);
    } else if (request.starts_with("POST")) {
        debug_log("Recieved POST");
        handle_post_request(client_sock, request, validated);
    } else if (request.starts_with("REPORT") && validated) {
        if (!validated) return send_error(client_sock, 403, false);
        debug_log("Recieved REPORT");
        handle_report_request(client_sock, request, validated);
    } else {
        debug_log("Recieved NOT FOUND METHOD");
        send_error(client_sock, 405, false);
        close_client(client_sock);
    }
}

// void handle_get_request(int& client_sock, const std::string& request) {
//     std::smatch match;
//     std::regex get_req_regex("GET (/[^ ]*) HTTP/1.1");
//     debug_log(request);
//     if (!std::regex_search(request, match, get_req_regex)) {
//         debug_log("regex not found");
//         send_error(client_sock, 400);
//         close_client(client_sock);
//         return;
//     }

//     std::string token;
//     get_auth_cookie(request, token);

//     std::string path = match[1];
//     debug_log("path:" + path);

//     // 1) Comandos Rápidos: categorías
//     if (path == "/api/quick/world") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/quick/world");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/quick/item") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/quick/item");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/quick/player") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/quick/player");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/quick/mob") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/quick/mob");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/quick/special") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/quick/special");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 2) Gestión de Jugadores
//     else if (path == "/api/player/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/kick") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/kick");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/ban") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/ban");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/unban") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/unban");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/whitelist/add") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/whitelist/add");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/whitelist/remove") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/whitelist/remove");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/whitelist/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/whitelist/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/op/add") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/op/add");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/op/remove") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/op/remove");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/op/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/op/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 3) Gestión de Plugins/Mods
//     else if (path == "/api/plugin/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/plugin/mods/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/mods/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/plugin/reload") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/reload");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/plugin/info") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/info");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/plugin/toggle") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/toggle");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/plugin/logs") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/plugin/logs");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 4) Gestión de Mundos
//     else if (path == "/api/world/info") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/info");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/change-spawn") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/change-spawn");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/regenerate-chunks") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/regenerate-chunks");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/clear-entities") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/clear-entities");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/save") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/save");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/toggle-autosave") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/toggle-autosave");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 5) Gestión de Backups
//     else if (path == "/api/backup/full") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/full");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/backup/world-only") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/world-only");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/backup/list") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/list");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/backup/restore") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/restore");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/backup/delete") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/delete");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/backup/configure-path") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/backup/configure-path");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 6) Comandos de Rayos
//     else if (path == "/api/lightning/simple") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/simple");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/lightning/storm") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/storm");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/lightning/punishment") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/punishment");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/lightning/blessing") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/blessing");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/lightning/area") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/area");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/lightning/follow") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/lightning/follow");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 7) Comandos de Items
//     else if (path == "/api/item/give/diamond_sword") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/diamond_sword");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/diamond_pickaxe") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/diamond_pickaxe");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/diamond_armor") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/diamond_armor");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/elytra") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/elytra");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/totem_of_undying") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/totem_of_undying");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/enchanted_golden_apple") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/enchanted_golden_apple");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/netherite_sword") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/netherite_sword");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/bow") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/bow");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/arrow") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/arrow");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/item/give/ender_pearl") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/item/give/ender_pearl");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 8) Comandos de Mobs
//     else if (path == "/api/mob/summon-on-player") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/mob/summon-on-player");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/mob/execute-summon") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/mob/execute-summon");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/mob/summon-multiple") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/mob/summon-multiple");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/mob/summon-with-effects") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/mob/summon-with-effects");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/mob/clear-specific") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/mob/clear-specific");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 9) Comandos de Jugador
//     else if (path == "/api/player/tp/spawn") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/tp/spawn");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/tp/coords") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/tp/coords");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/gamemode/creative") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/gamemode/creative");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/gamemode/survival") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/gamemode/survival");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/gamemode/adventure") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/gamemode/adventure");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/gamemode/spectator") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/gamemode/spectator");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/heal") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/heal");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/feed") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/feed");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/player/clear-inventory") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/player/clear-inventory");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

//     // 10) Comandos de Mundo Rápidos
//     else if (path == "/api/world/time/day") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/time/day");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/time/night") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/time/night");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/time/noon") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/time/noon");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/time/midnight") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/time/midnight");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/weather/clear") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/weather/clear");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/weather/rain") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/weather/rain");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/weather/thunder") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/weather/thunder");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/difficulty/peaceful") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/difficulty/peaceful");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/difficulty/hard") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/difficulty/hard");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/gamerule/stop-daylight") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/gamerule/stop-daylight");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/gamerule/start-daylight") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/gamerule/start-daylight");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/gamerule/disable-mob-spawn") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/gamerule/disable-mob-spawn");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }
//     else if (path == "/api/world/gamerule/enable-mob-spawn") {
//         if (token.empty() || !validate_token(token)) {
//             send_error(client_sock, 401);
//             close_client(client_sock);
//             return;
//         }
//         debug_log("Endpoint: /api/world/gamerule/enable-mob-spawn");
//         std::string json = "{}";
//         std::vector<char> body(json.begin(), json.end());
//         send_response(client_sock, "200 OK", body, "application/json");
//         close_client(client_sock);
//         return;
//     }

// }