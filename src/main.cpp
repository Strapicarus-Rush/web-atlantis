#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
// #include <vector>
#include <unordered_map>
#include <map>
#include <regex>
#include <netinet/in.h>
#include <unistd.h>
#include <sqlite3.h>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <pwd.h>

// Variables de configuración con valores por defecto
inline std::string DB_PATH = "users.db";
inline int PORT = 8080;
inline int TOKEN_EXPIRY = 3600;

#define BUFFER_SIZE 8192

bool DEBUG_MODE = false;
bool LOG_MODE = false;
std::ofstream log_file;

std::map<std::string, time_t> valid_tokens;

const std::string style_css = R"ELCSS(:root {
  --bg-light: #ffffff;
  --bg-dark: #121212;
  --bg-gray: #2e2e2e;

  --text-light: #000000;
  --text-dark: #ffffff;
  --accent: #4a90e2;
  --error: #ff4c4c;
  --radius: 8px;
}

body {
  margin: 0;
  padding: 0;
  font-family: "Segoe UI", sans-serif;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 100vh;
  transition: background 0.3s, color 0.3s;
}

/* Tema claro por defecto */
body.light {
  background: var(--bg-light);
  color: var(--text-light);
}

body.dark {
  background: var(--bg-dark);
  color: var(--text-dark);
}

body.gray {
  background: var(--bg-gray);
  color: var(--text-dark);
}

.login-container {
  width: 90%;
  max-width: 400px;
  background: rgba(255, 255, 255, 0.05);
  backdrop-filter: blur(6px);
  padding: 2rem;
  border-radius: var(--radius);
  box-shadow: 0 4px 12px rgba(0,0,0,0.2);
}

.login-container h2 {
  text-align: center;
  margin-bottom: 1.5rem;
}

form {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

input {
  padding: 0.6rem;
  border: 1px solid #ccc;
  border-radius: var(--radius);
  font-size: 1rem;
}

button {
  padding: 0.7rem;
  background: var(--accent);
  color: white;
  border: none;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.3s;
}

button:hover {
  background: #357ac8;
}

.status-msg {
  margin-top: 1rem;
  text-align: center;
  color: var(--error);
}

.theme-selector {
  position: absolute;
  top: 1rem;
  right: 1rem;
  display: flex;
  gap: 0.5rem;
}

.theme-selector button {
  background: transparent;
  border: 1px solid #ccc;
  color: inherit;
  padding: 0.4rem 0.8rem;
  border-radius: var(--radius);
  cursor: pointer;
  font-size: 0.9rem;
  transition: background 0.2s, color 0.2s;
}

.theme-selector button:hover {
  background: var(--accent);
  color: white;
})ELCSS";

const std::string index_html = R"ELHTML(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Login</title>
  <link rel="stylesheet" href="style.css">
  <script>
    function getCookie(name) {
      const match = document.cookie.match(new RegExp('(^| )' + name + '=([^;]+)'));
      return match ? match[2] : null;
    }

    function applyInitialTheme() {
      const cookieTheme = getCookie("theme");
      const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
      document.body.className = cookieTheme || systemTheme;
    }

    applyInitialTheme();
  </script>
</head>
<body>
  <div class="theme-selector">
    <button onclick="setTheme('light')">Blanco</button>
    <button onclick="setTheme('dark')">Negro</button>
    <button onclick="setTheme('gray')">Gris</button>
  </div>

  <div class="login-container">
    <h2>Iniciar sesión</h2>
    <form id="login-form">
      <label for="user">Usuario</label>
      <input type="text" id="user" required>

      <label for="pass">Contraseña</label>
      <input type="password" id="pass" required>

      <button type="submit">Entrar</button>
    </form>
    <div id="status" class="status-msg"></div>
  </div>

  <script>
    function setTheme(theme) {
      document.body.className = theme;
      document.cookie = `theme=${theme}; path=/; max-age=31536000`;
    }

    document.getElementById("login-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      const user = document.getElementById("user").value;
      const pass = document.getElementById("pass").value;

      const res = await fetch("/login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ user, pass }),
      });

      const status = document.getElementById("status");
      if (res.ok) {
        const data = await res.json();
        localStorage.setItem("auth_token", data.token);
        window.location.href = "panel.html";
      } else {
        status.textContent = "Error: Usuario o contraseña incorrectos.";
      }
    });
  </script>
</body>
</html>
)ELHTML";

const std::string panel_html = R"HTML(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <title>Panel</title>
</head>
<body>
  <h2>Enviar comando a screen</h2>
  <form id="cmd-form">
    <label>Comando: <input type="text" id="cmd" required></label>
    <button type="submit">Ejecutar</button>
  </form>
  <div id="status"></div>

  <script>
    const token = localStorage.getItem("auth_token");
    if (!token) {
      alert("No autenticado. Redirigiendo al login.");
      window.location.href = "index.html";
    }

    document.getElementById("cmd-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      const cmd = document.getElementById("cmd").value;

      const res = await fetch("/run", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Authorization": token
        },
        body: JSON.stringify({ cmd }),
      });

      const status = document.getElementById("status");
      if (res.ok) {
        status.innerText = "Comando enviado con éxito.";
      } else {
        status.innerText = "Error al enviar comando.";
      }
    });
  </script>
</body>
</html>
)HTML";

std::string get_base_path() {
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }
    return std::string(home) + "/.web-atlantis";
}

void ensure_public_files(const std::string& base_path) {
    const std::string public_dir = base_path + "/public";
    std::filesystem::create_directories(public_dir);

    const std::string index_file = public_dir + "/index.html";
    const std::string panel_file = public_dir + "/panel.html";
    const std::string style_file = public_dir + "/style.css";

    if (!std::filesystem::exists(index_file)) {
        std::ofstream(index_file) << index_html;
    }

    if (!std::filesystem::exists(panel_file)) {
        std::ofstream(panel_file) << panel_html;
    }

    if (!std::filesystem::exists(style_file)) {
        std::ofstream(style_file) << style_css;
    }
}

void load_or_create_config() {
    const std::string base_path = get_base_path();
    const std::string config_path = base_path + "/server.config";

    std::filesystem::create_directories(base_path);
    ensure_public_files(base_path);

    std::ifstream infile(config_path);
    if (!infile.is_open()) {
        // Crear config por defecto
        std::ofstream outfile(config_path);
        outfile << "DB_PATH=" << DB_PATH << "\n";
        outfile << "PORT=" << PORT << "\n";
        outfile << "TOKEN_EXPIRY=" << TOKEN_EXPIRY << "\n";
        outfile.close();
        std::cout << "Archivo de configuración creado en " << config_path << "\n";
        return;
    }

    // Leer config existente
    std::unordered_map<std::string, std::string> config;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
        }
    }

    // Asignación con validación
    if (config.count("DB_PATH")) {
        DB_PATH = config["DB_PATH"];
    }

    if (config.count("PORT")) {
        int port = std::stoi(config["PORT"]);
        if (port >= 1024 && port <= 65535) {
            PORT = port;
        } else {
            std::cerr << "Valor inválido para PORT. Usando por defecto: " << PORT << "\n";
        }
    }

    if (config.count("TOKEN_EXPIRY")) {
        int expiry = std::stoi(config["TOKEN_EXPIRY"]);
        if (expiry > 0) {
            TOKEN_EXPIRY = expiry;
        } else {
            std::cerr << "TOKEN_EXPIRY inválido. Usando por defecto: " << TOKEN_EXPIRY << "\n";
        }
    }
}

std::string get_public_path() {
    return get_base_path() + "/public";
}

std::string get_log_path() {
    return get_base_path() + "/server.log";
}

void debug_log(const std::string& msg) {
    if (DEBUG_MODE) std::cerr << "[DEBUG] " << msg << std::endl;
    if (LOG_MODE && log_file.is_open()) log_file << "[LOG] " << msg << std::endl;
}

inline std::string generate_salt(std::size_t length = 16) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<unsigned char> dis(0, 255);

    std::ostringstream oss;
    for (std::size_t i = 0; i < length; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dis(gen));
    return oss.str();
}

inline std::string sha256(const std::string& input) {
    std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 computation failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

    return oss.str();
}

inline std::string hash_password(const std::string& password) {
    std::string salt = generate_salt();
    std::string hashed = sha256(salt + password);
    return salt + ":" + hashed;
}

inline bool verify_password(const std::string& password, const std::string& stored) {
    auto sep = stored.find(':');
    if (sep == std::string::npos) return false;
    std::string salt = stored.substr(0, sep);
    std::string hash = stored.substr(sep + 1);
    return sha256(salt + password) == hash;
}

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string s;
    while (std::getline(iss, s, delim))
        parts.push_back(s);
    return parts;
}

bool add_user(const std::string& username, const std::string& password) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        debug_log()
        return false;
    }

    std::string hashed_pass = hash_password(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_pass.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}

bool initialize_database() {
    if (std::filesystem::exists(DB_PATH)) return true;

    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        );
        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";

    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return add_user("admin", "admin");
}

std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    if (!HMAC(EVP_sha256(),
              key.data(), key.size(),
              reinterpret_cast<const unsigned char*>(data.data()), data.size(),
              result, &len)) {
        throw std::runtime_error("HMAC failed");
    }

    std::ostringstream oss;
    for (unsigned int i = 0; i < 32; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(result[i]);

    return oss.str();
}

// std::string generate_token(const std::string& username) {
//     time_t now = time(nullptr);
//     std::string data = username + ":" + std::to_string(now);
//     std::string signature = hmac_sha256("secret_token_key", data);
//     std::string token = data + ":" + signature;

//     valid_tokens[token] = now + TOKEN_EXPIRY;
//     return token;
// }

std::string generate_token(int user_id) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK)
        throw std::runtime_error("Failed to open database");

    // Obtener username desde user_id
    const char* get_user_sql = "SELECT username FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, get_user_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare statement for username");
    }

    sqlite3_bind_int(stmt, 1, user_id);
    std::string username;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        username = reinterpret_cast<const char*>(name);
    } else {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("User ID not found");
    }
    sqlite3_finalize(stmt);

    // Generar token firmado
    time_t now = time(nullptr);
    std::string data = username + ":" + std::to_string(now);
    std::string signature = hmac_sha256("secret_token_key", data);
    std::string token = data + ":" + signature;

    // Insertar en base de datos
    const char* insert_sql = "INSERT INTO tokens (token, user_id, created_at) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare token insert");
    }

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("Failed to insert token");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return token;
}

// bool validate_token(const std::string& token) {
//     sqlite3* db;
//     if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

//     const char* sql = "SELECT username, created_at FROM tokens WHERE token = ?";
//     sqlite3_stmt* stmt;
//     if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
//         sqlite3_close(db);
//         return false;
//     }

//     sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
//     bool valid = false;

//     if (sqlite3_step(stmt) == SQLITE_ROW) {
//         const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
//         time_t created_at = static_cast<time_t>(sqlite3_column_int64(stmt, 1));

//         std::string data = std::string(username) + ":" + std::to_string(created_at);
//         std::string expected_sig = hmac_sha256("secret_token_key", data);
//         auto parts = split(token, ':');

//         if (parts.size() == 3 && parts[2] == expected_sig && time(nullptr) <= created_at + TOKEN_EXPIRY)
//             valid = true;
//     }

//     sqlite3_finalize(stmt);
//     sqlite3_close(db);
//     return valid;
// }

bool validate_token(const std::string& token) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* sql = R"(
        SELECT u.username, t.created_at, t.user_id
        FROM tokens t
        JOIN users u ON t.user_id = u.id
        WHERE t.token = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    bool valid = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        time_t created_at = static_cast<time_t>(sqlite3_column_int64(stmt, 1));

        auto parts = split(token, ':');
        if (parts.size() == 3) {
            std::string data = std::string(username) + ":" + std::to_string(created_at);
            std::string expected_sig = hmac_sha256("secret_token_key", data);

            if (parts[2] == expected_sig && time(nullptr) <= created_at + TOKEN_EXPIRY)
                valid = true;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return valid;
}

// bool validate_token(const std::string& token) {
//     auto parts = split(token, ':');
//     if (parts.size() != 3) return false;

//     std::string username = parts[0];
//     std::string timestamp = parts[1];
//     std::string signature = parts[2];

//     std::string data = username + ":" + timestamp;
//     std::string expected_sig = hmac_sha256("secret_token_key", data);

//     if (signature != expected_sig) return false;

//     time_t issued = std::stol(timestamp);
//     if (time(nullptr) > issued + TOKEN_EXPIRY) return false;

//     return true;
// }
bool validate_user(const std::string& user, const std::string& pass, int& out_user_id) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* query = "SELECT id, password FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    bool ok = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* stored = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (verify_password(pass, stored)) {
            out_user_id = id;
            ok = true;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}
// bool validate_user(const std::string& user, const std::string& pass) {
//     sqlite3* db;
//     sqlite3_open(DB_PATH.c_str(), &db);
//     sqlite3_stmt* stmt;
//     const char* query = "SELECT id, password FROM users WHERE username = ?";
//     sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
//     sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
//     bool ok = false;
//     if (sqlite3_step(stmt) == SQLITE_ROW) {
//         const char* stored = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
//         ok = verify_password(pass, stored);
//     }
//     sqlite3_finalize(stmt);
//     sqlite3_close(db);
//     return ok;
// }

std::string read_request(int client_sock) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes = read(client_sock, buffer, BUFFER_SIZE - 1);
    return std::string(buffer, bytes);
}

std::string get_mime_type(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".ico")) return "image/x-icon";
    if (path.ends_with(".woff")) return "font/woff";
    if (path.ends_with(".woff2")) return "font/woff2";
    if (path.ends_with(".ttf")) return "font/ttf";
    if (path.ends_with(".otf")) return "font/otf";
    if (path.ends_with(".mp3")) return "audio/mpeg";
    if (path.ends_with(".wav")) return "audio/wav";
    if (path.ends_with(".mp4")) return "video/mp4";
    if (path.ends_with(".zip")) return "application/zip";
    if (path.ends_with(".rar")) return "application/vnd.rar";
    if (path.ends_with(".exe")) return "application/octet-stream";
    if (path.ends_with(".bin")) return "application/octet-stream";
    return "application/octet-stream";
}

void send_response(int client_sock, const std::string& status, const std::vector<char>& body, const std::string& content_type) {
    std::ostringstream header;
    header << "HTTP/1.1 " << status << "\r\n";
    header << "Content-Type: " << content_type << "\r\n";
    header << "Content-Length: " << body.size() << "\r\n";
    header << "X-Strapicarus: estuvo aquí en atlanis\r\n";
    header << "Connection: close\r\n\r\n";

    std::string header_str = header.str();
    send(client_sock, header_str.c_str(), header_str.size(), 0);
    send(client_sock, body.data(), body.size(), 0);
}

// --------------------- GET ---------------------
void handle_get_request(int client_sock, const std::string& request) {
    std::smatch match;
    std::regex get_req_regex("GET (/[^ ]*) HTTP/1.1");

    if (!std::regex_search(request, match, get_req_regex)) {
        send_response(client_sock, "400 Bad Request", "Petición GET malformada");
        close(client_sock);
        return;
    }

    std::string path = match[1];
    std::string filename;

    if (path == "/" || path == "/index" || path == "/index.html") {
        filename = get_public_path() + "/index.html";
    } else if (path == "/panel" || path == "/panel.html") {
        std::smatch auth_match;
        std::regex auth_regex("Authorization: ([^\r\n]+)");
        if (!std::regex_search(request, auth_match, auth_regex)) {
            send_response(client_sock, "401 Unauthorized", "Falta el token");
            close(client_sock);
            return;
        }

        std::string token = auth_match[1];
        if (!validate_token(token)) {
            send_response(client_sock, "401 Unauthorized", "Token inválido o expirado");
            close(client_sock);
            return;
        }

        filename = get_public_path() + "/panel.html";
    } else {
        filename = get_public_path() + path;
    }

    if (!std::filesystem::exists(filename)) {
        send_response(client_sock, "404 Not Found", "Archivo no encontrado");
        close(client_sock);
        return;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        send_response(client_sock, "500 Internal Server Error", "Error abriendo el archivo");
        close(client_sock);
        return;
    }

    std::vector<char> content((std::istreambuf_iterator<char>(file)), {});
    std::string mime = get_mime_type(filename);
    send_response(client_sock, "200 OK", content, mime);
    close(client_sock);
}

// --------------------- POST ---------------------
void handle_post_request(int client_sock, const std::string& request) {
    std::smatch match;
    std::regex post_req_regex("POST (/\\w+) HTTP/1.1");

    if (!std::regex_search(request, match, post_req_regex)) {
        send_response(client_sock, "400 Bad Request", "Petición POST malformada");
        close(client_sock);
        return;
    }

    std::string endpoint = match[1];
    std::size_t body_pos = request.find("\r\n\r\n");
    if (body_pos == std::string::npos) {
        send_response(client_sock, "400 Bad Request", "Falta el body");
        close(client_sock);
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
        send_response(client_sock, "404 Not Found", "Endpoint POST no encontrado");
        close(client_sock);
    }
}

// --------------------- /login ---------------------
void handle_login(int client_sock, const std::string& body) {
    std::smatch match;
    std::regex user_pass("\"user\"\\s*:\\s*\"(.*?)\".*?\"pass\"\\s*:\\s*\"(.*?)\"");

    if (std::regex_search(body, match, user_pass)) {
        std::string user = match[1];
        std::string pass = match[2];
        if (validate_user(user, pass)) {
            std::string token = generate_token(user);
            send_response(client_sock, "200 OK", "{\"token\":\"" + token + "\"}", "application/json");
        } else {
            send_response(client_sock, "403 Forbidden", "Credenciales inválidas");
        }
    } else {
        send_response(client_sock, "400 Bad Request", "Faltan campos user/pass");
    }

    close(client_sock);
}

// --------------------- /run ---------------------
void handle_run_command(int client_sock, const std::string& request, const std::string& body) {
    std::smatch match;
    std::regex token_rx("Authorization:\\s*(.*?)\r\n");

    if (!std::regex_search(request, match, token_rx)) {
        send_response(client_sock, "403 Forbidden", "Falta el token");
        close(client_sock);
        return;
    }

    std::string token = match[1];
    if (!validate_token(token)) {
        send_response(client_sock, "403 Forbidden", "Token inválido o expirado");
        close(client_sock);
        return;
    }

    std::regex cmd_rx("\"cmd\"\\s*:\\s*\"(.*?)\"");
    if (std::regex_search(body, match, cmd_rx)) {
        std::string cmd = match[1];
        std::string full_cmd = "screen -S mysession -X stuff '" + cmd + "\\n'";
        system(full_cmd.c_str());
        send_response(client_sock, "200 OK", "Comando enviado");
    } else {
        send_response(client_sock, "400 Bad Request", "Falta el campo cmd");
    }

    close(client_sock);
}

// --------------------- /edit_user ---------------------
void handle_edit_user(int client_sock, const std::string& request, const std::string& body) {
    std::smatch match;
    std::regex token_rx("Authorization:\\s*(.*?)\r\n");

    if (!std::regex_search(request, match, token_rx)) {
        send_response(client_sock, "403 Forbidden", "Falta token de autorización");
        close(client_sock);
        return;
    }

    std::string token = match[1];
    if (!validate_token(token)) {
        send_response(client_sock, "403 Forbidden", "Token inválido");
        close(client_sock);
        return;
    }

    std::regex user_rx("\"user\"\\s*:\\s*\"(.*?)\"");
    std::regex pass_rx("\"pass\"\\s*:\\s*\"(.*?)\"");
    std::regex new_pass_rx("\"new_pass\"\\s*:\\s*\"(.*?)\"");

    if (std::regex_search(body, match, user_rx)) {
        std::string user = match[1];
        std::string new_pass = std::regex_search(body, match, new_pass_rx) ? match[1] : "";

        if (update_user_password(user, new_pass)) {
            send_response(client_sock, "200 OK", "Contraseña actualizada");
        } else {
            send_response(client_sock, "500 Internal Server Error", "Error actualizando contraseña");
        }
    } else {
        send_response(client_sock, "400 Bad Request", "Faltan campos user/new_pass");
    }

    close(client_sock);
}

void handle_client(int client_sock) {
    std::string request = read_request(client_sock);

    if (request.starts_with("GET ")) {
        handle_get_request(client_sock, request);
    } else if (request.starts_with("POST ")) {
        handle_post_request(client_sock, request);
    } else {
        send_response(client_sock, "405 Method Not Allowed", "Método no soportado");
        close(client_sock);
    }
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-debug") == 0) DEBUG_MODE = true;
        if (std::strcmp(argv[i], "-log") == 0) LOG_MODE = true;
    }
    load_or_create_config();
    if (LOG_MODE) log_file.open(get_log_path(), std::ios::app);

    if (!initialize_database()) {
        std::cerr << "Error inicializando la base de datos\n";
        return 1;
    }

    int server_fd, client_sock;
    sockaddr_in address{};
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    debug_log("Server listening on port " + std::to_string(PORT));

    while (true) {
        client_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        std::thread(handle_client, client_sock).detach();
    }

    close(server_fd);
    if (log_file.is_open()) log_file.close();
    return 0;
}