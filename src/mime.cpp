// #include "debug_log.hpp"
// #include <string>
// #include <iostream>
// #include <algorithm>
// #include <cctype>

// inline std::string trim(const std::string& s) {
//     auto start = s.find_first_not_of(" \r\n\t");
//     auto end = s.find_last_not_of(" \r\n\t");
//     return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
// }

// inline std::string to_lower(std::string s) {
//     std::transform(s.begin(), s.end(), s.begin(),
//                    [](unsigned char c) { return std::tolower(c); });
//     return s;
// }

// inline std::string get_mime_type(const std::string& raw_path) {
//     const std::string path = to_lower(trim(raw_path));
//     if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
//     if (path.ends_with(".css")) return "text/css";
//     if (path.ends_with(".txt")) return "text/plain; charset=utf-8";
//     if (path.ends_with(".js")) return "application/javascript";
//     if (path.ends_with(".json")) return "application/json";
//     if (path.ends_with(".png")) return "image/png";
//     if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
//     if (path.ends_with(".gif")) return "image/gif";
//     if (path.ends_with(".svg")) return "image/svg+xml";
//     if (path.ends_with(".ico")) return "image/x-icon";
//     if (path.ends_with(".woff")) return "font/woff";
//     if (path.ends_with(".woff2")) return "font/woff2";
//     if (path.ends_with(".ttf")) return "font/ttf";
//     if (path.ends_with(".otf")) return "font/otf";
//     if (path.ends_with(".mp3")) return "audio/mpeg";
//     if (path.ends_with(".wav")) return "audio/wav";
//     if (path.ends_with(".mp4")) return "video/mp4";
//     if (path.ends_with(".zip")) return "application/zip";
//     if (path.ends_with(".rar")) return "application/vnd.rar";
//     if (path.ends_with(".exe")) return "application/octet-stream";
//     if (path.ends_with(".bin")) return "application/octet-stream";
//     return "Error";
// }








// void init_database() {
//     sqlite3* db;
//     int rc = sqlite3_open("security.db", &db);
//     if (rc != SQLITE_OK) {
//         std::cerr << "No se pudo abrir la base de datos.\n";
//         std::exit(1);
//     }

//     const char* create_bans =
//         "CREATE TABLE IF NOT EXISTS banned_ips ("
//         "ip TEXT PRIMARY KEY,"
//         "banned_at INTEGER"
//         ");";

//     const char* create_hits =
//         "CREATE TABLE IF NOT EXISTS ip_hits ("
//         "ip TEXT,"
//         "hit_time INTEGER"
//         ");";

//     char* err_msg = nullptr;
//     sqlite3_exec(db, create_bans, nullptr, nullptr, &err_msg);
//     sqlite3_exec(db, create_hits, nullptr, nullptr, &err_msg);

//     sqlite3_close(db);
// }

// // -- CHECK BAN --
// bool is_ip_banned(const std::string& ip) {
//     std::scoped_lock lock(db_mutex);
//     sqlite3* db;
//     sqlite3_open("security.db", &db);
//     const char* sql = "SELECT 1 FROM banned_ips WHERE ip = ? LIMIT 1;";
//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
//     sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
//     bool banned = (sqlite3_step(stmt) == SQLITE_ROW);
//     sqlite3_finalize(stmt);
//     sqlite3_close(db);
//     return banned;
// }

// // -- REGISTER HIT --
// void register_ip_hit(const std::string& ip) {
//     std::scoped_lock lock(db_mutex);
//     sqlite3* db;
//     sqlite3_open("security.db", &db);
//     const char* sql = "INSERT INTO ip_hits (ip, hit_time) VALUES (?, ?);";
//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
//     sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
//     sqlite3_bind_int64(stmt, 2, std::time(nullptr));
//     sqlite3_step(stmt);
//     sqlite3_finalize(stmt);
//     sqlite3_close(db);
// }

// // -- BAN IP --
// void ban_ip(const std::string& ip) {
//     std::scoped_lock lock(db_mutex);
//     sqlite3* db;
//     sqlite3_open("security.db", &db);
//     const char* sql = "INSERT OR IGNORE INTO banned_ips (ip, banned_at) VALUES (?, ?);";
//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
//     sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
//     sqlite3_bind_int64(stmt, 2, std::time(nullptr));
//     sqlite3_step(stmt);
//     sqlite3_finalize(stmt);
//     sqlite3_close(db);
// }

// // -- GET IP FROM HEADERS (CLOUDFLARE/NGINX) --
// std::string extract_real_ip(const std::string& request_raw, const std::string& fallback_ip) {
//     std::istringstream stream(request_raw);
//     std::string line;
//     while (std::getline(stream, line)) {
//         if (line.starts_with("CF-Connecting-IP: ")) {
//             return line.substr(20);
//         } else if (line.starts_with("X-Forwarded-For: ")) {
//             return line.substr(18);
//         }
//     }
//     return fallback_ip;
// }

// // -- HANDLE CLIENT --
// void handle_client(int client_sock, sockaddr_in client_addr) {
//     char ip_str[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
//     std::string ip = ip_str;

//     char buffer[8192];
//     ssize_t received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
//     if (received <= 0) {
//         close(client_sock);
//         return;
//     }
//     buffer[received] = '\0';
//     std::string request(buffer);

//     std::string real_ip = extract_real_ip(request, ip);

//     if (is_ip_banned(real_ip)) {
//         std::string ban_msg = "HTTP/1.1 451 Bad Hacker\r\nConnection: close\r\n\r\nAccess denied.";
//         send(client_sock, ban_msg.c_str(), ban_msg.size(), MSG_NOSIGNAL);
//         close(client_sock);
//         return;
//     }

//     register_ip_hit(real_ip);

//     // TODO: Add rate-limit logic here if needed

//     // Aquí llamarías a tu lógica principal
//     // process_http_request(client_sock, request, real_ip);

//     close(client_sock);
// }

// // -- MAIN LOOP --
// int main() {
//     init_database();

//     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd == -1) {
//         std::cerr << "Fallo al crear socket\n";
//         return 1;
//     }

//     int opt = 1;
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     sockaddr_in server_addr{};
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(8080);
//     server_addr.sin_addr.s_addr = INADDR_ANY;

//     if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         std::cerr << "Fallo al hacer bind()\n";
//         return 1;
//     }

//     if (listen(server_fd, 100) < 0) {
//         std::cerr << "Fallo al hacer listen()\n";
//         return 1;
//     }

//     std::cout << "Servidor escuchando en el puerto 8080...\n";

//     while (true) {
//         sockaddr_in client_addr{};
//         socklen_t addrlen = sizeof(client_addr);
//         int client_sock = accept(server_fd, (sockaddr*)&client_addr, &addrlen);
//         if (client_sock < 0) {
//             continue;
//         }

//         std::thread(handle_client, client_sock, client_addr).detach();
//     }

//     close(server_fd);
//     return 0;
// }


