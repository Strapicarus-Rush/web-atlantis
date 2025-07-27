#ifndef UTILS_HPP
#define UTILS_HPP

#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <unistd.h>
#include <pwd.h>
#include <vector>
#include <regex>
#include <cmath>


// ========================
// Regex para lectura de resultados de consola
// ========================
static inline std::regex regex_confirm_termination(R"(:~$|jaime)", std::regex::icase);
static inline std::regex regex_test_inicialization(R"(Deftones|btop|strapicarus)", std::regex::icase);
static inline std::vector<std::regex> regex_count_patterns = {
    std::regex(R"(There are (\d+) of a max of \d+ players online)", std::regex::icase),
    std::regex(R"(Hay (\d+) de un máximo de \d+ jugadores conectados)", std::regex::icase),
    std::regex(R"(Hay (\d+) jugadores de un máximo de (\d+) jugadores en linea)", std::regex::icase)
};
static inline std::vector<std::regex> regex_weather_patterns = {
    std::regex(R"(clear|rain|thunder)", std::regex::icase),
    std::regex(R"(despejado|lluvia|tormenta)", std::regex::icase)
};
static inline std::vector<std::regex> regex_list_patterns = {
    std::regex(R"(Players online:\s*(.*))", std::regex::icase),
    std::regex(R"(Jugadores conectados:\s*(.*))", std::regex::icase)
};
static inline std::vector<std::regex> regex_seed_patterns = {
    std::regex(R"(Seed: \[(-?\d+)\])", std::regex::icase),
    std::regex(R"(Semilla: \[(-?\d+)\])", std::regex::icase)
};
static inline std::vector<std::regex> regex_time_patterns = {
    std::regex(R"(The time is (\d+))", std::regex::icase),
    std::regex(R"(La hora es (\d+))", std::regex::icase)
};
static inline std::vector<std::regex> regex_difficulty_patterns = {
    std::regex(R"(The difficulty is (\w+))", std::regex::icase),   
    std::regex(R"(La dificultad es (\w+))", std::regex::icase)     
};
static inline std::vector<std::regex> regex_border_patterns = {
    std::regex(R"(The world border is currently (\d+(?:\.\d+)?) block\(s\) wide)", std::regex::icase),
    std::regex(R"(El borde del mundo actualmente mide (\d+(?:\.\d+)?) bloque(?:s)? de ancho)", std::regex::icase),
    std::regex(R"(The world border is currently (\d+(?:\.\d+)?) blocks? wide)", std::regex::icase),
    std::regex(R"(El límite mundial está actualmente en (\d+(?:\.\d+)?) bloques? de ancho)", std::regex::icase),
    std::regex(R"(World border is currently (\d+(?:\.\d+)?) blocks? wide)", std::regex::icase),
    std::regex(R"(Limite del mundo es (\d+(?:\.\d+)?) bloques? ancho)", std::regex::icase)
};


// ========================
// Entidades comunes
// ========================
static inline std::vector<std::string> animals = {"cow", "pig", "sheep", "chicken", "horse", "donkey", "mule", "llama"};
static inline std::vector<std::string> monsters = {"zombie", "skeleton", "creeper", "spider", "enderman", "witch"};


// ========================
// Configuraciñon
// ========================
struct alignas(16) GlobalConfig {

#ifdef DEV
    uint8_t dev_mode    = true;
#else
    uint8_t dev_mode    = false;
#endif
    uint8_t debug_mode  = true;
    uint8_t log_mode    = false;
    uint8_t padding     = 0;            // para mantener alineación en memoria (experimental)
    int     port        = 8080;
    int     max_threads = 2;
    int     max_req_buf_size = 8192;    // 8kb para el buffer de lectura, si el tamaño de la petición es mayor se trunca.
    int     login_expiration = 3600;    // Duración en segundos de la cookie de autorización 
    // ========================
    // Para la ruta de las instalaciones de los servidores minecraft.
    // Sin ruta absoluta se asume "/home/[currentUser]/[servers_paths]". la ruta absoluta requiere length<=31 caracteres.
    // Las otras rutas (config, db, log) se asumen en $HOME/.pccssystems/web-atlantis/*, o ruta del proyecto en desarrollo.
    // ========================
    char servers_path[32] = "juegos";
    char config_file[16] = "atlantis.config";
    char db_name[16] = "atlantis.db";
    char log_file[16] = "atlantis.log";
};

inline GlobalConfig config;

inline std::ofstream log_file;

struct PluginInfo {
    std::string name;
    double size_mb;
};

struct ModInfo {
    std::string name;
    double size_mb;
};

static inline void debug_log_internal(const std::string& msg, const char* file, int line, const char* func) {
    const std::string full = "[DEBUG] " + std::string(file) + ":" + std::to_string(line) + " (" + func + ") - " + msg;

    if (config.debug_mode)
    {
        std::cerr << full << '\r' << '\n';
    }
    if (config.log_mode && log_file.is_open()){
        log_file << full << '\r' << '\n';
    }
}

#define debug_log(msg) debug_log_internal((msg), __FILE__, __LINE__, __func__)

static inline void trim_header_line(std::string& s) {
    const char* ws = " \t\r\n";
    const auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    const auto end = s.find_last_not_of(ws);
    s.erase(end + 1);
    s.erase(0, start);
}

static inline std::string trim_string(const std::string& str) {
    size_t start = str.find_first_not_of(" \n\r\t");
    size_t end = str.find_last_not_of(" \n\r\t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static inline std::string replace_spaces_with_underscore(std::string str) {
    std::replace(str.begin(), str.end(), ' ', '_');
    return str;
}

static inline std::string sanitize_string(std::string str) {
    const std::string forbidden_chars = " -,'\".:;/\\+*&%·#$|@=?!`ç´{}[]";

    std::replace_if(str.begin(), str.end(),
        [&](char c) {
            return forbidden_chars.find(c) != std::string::npos;
        },
        '_'
    );
    return str;
}

static inline bool has_dot(const std::string& str) {
    return str.find('.') != std::string::npos;
}

static inline std::string get_mime_type(const std::string& path) {
    if(!has_dot(path)) return "Error";
    // const std::string path = to_lower(trim_copy(raw_path));
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    // if (path.ends_with(".txt")) return "text/plain; charset=utf-8";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    // if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    // if (path.ends_with(".gif")) return "image/gif";
    // if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".ico")) return "image/x-icon";
    // if (path.ends_with(".woff")) return "font/woff";
    // if (path.ends_with(".woff2")) return "font/woff2";
    // if (path.ends_with(".ttf")) return "font/ttf";
    // if (path.ends_with(".otf")) return "font/otf";
    // if (path.ends_with(".mp3")) return "audio/mpeg";
    // if (path.ends_with(".wav")) return "audio/wav";
    // if (path.ends_with(".mp4")) return "video/mp4";
    // if (path.ends_with(".zip")) return "application/zip";
    // if (path.ends_with(".rar")) return "application/vnd.rar";
    // if (path.ends_with(".exe")) return "application/octet-stream";
    // if (path.ends_with(".bin")) return "application/octet-stream";
    return "Error";
}

static inline std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string s;
    while (std::getline(iss, s, delim))
        parts.push_back(s);
    return parts;
}

static inline std::string normalize_servers_path(const GlobalConfig& conf) {
    // Si la ruta ya es absoluta, no hay nada que hacer
    if (conf.servers_path[0] == '/') {
        return std::string(conf.servers_path); 
    }

    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }

    std::string full_path = std::string(home) + "/" + std::string(conf.servers_path);

    return full_path;
}

static inline std::string get_base_path(const GlobalConfig& conf) {
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }

    std::string app_path;
    if (conf.dev_mode)
    {
      // Ruta del ejecutable
      std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");

      // Subir un nivel desde el directorio build a la raiz del proyecto para tener acceso a public
      std::filesystem::path basePath = exePath.parent_path().parent_path();
      app_path = basePath;      
    }else{
      app_path = std::string(home) + "/.pccssystems/web-atlantis";
    }
    return app_path;
}

static inline std::string get_db_path() {
    return get_base_path(config) + "/data";
}

static inline std::string get_db_file() {
    return get_base_path(config) + "/data/" + config.db_name;
}

static inline std::string get_public_path() {
    return get_base_path(config) + "/public";
}

static inline std::string get_log_path() {
    return get_base_path(config) + "/log/";
}

static inline std::string get_log_file() {
    return get_base_path(config) + "/log/" + config.log_file;
}

static inline std::string get_config_path() {
    return get_base_path(config) + "/config/" ;
}

static inline std::string get_config_file() {
    return get_base_path(config) + "/config/" + config.config_file ;
}

static inline std::string get_servers_path() {
    return normalize_servers_path(config);
}

static inline void set_path(char* dest, std::size_t max_len, const char* path, const char* label) {
    int written = std::snprintf(dest, max_len, "%s", path);

    if (written < 0) {
        std::cerr << "Error: Fallo al copiar " << label << " al buffer. Se deja vacío.\n";
        dest[0] = '\0'; // fallback seguro
    } else if (static_cast<std::size_t>(written) >= max_len) {
        std::cerr << "Advertencia: " << label << " fue truncado a "
                  << max_len - 1 << " caracteres.\n";
    }
}

static inline std::string get_file_op(const std::string& path) {
    std::ifstream infile(path, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("Error: No se pudo abrir el archivo: " + path);
    }

    infile.seekg(0, std::ios::end);
    std::streamsize file_size = infile.tellg();
    if (file_size < 0) {
        throw std::runtime_error("Error: No se pudo determinar el tamaño del archivo.");
    }

    if (static_cast<std::size_t>(file_size) > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("Error: archivo demasiado grande para ser leído en memoria.");
    }

    infile.seekg(0, std::ios::beg);

    std::string content(static_cast<std::size_t>(file_size), '\0');
    if (!infile.read(content.data(), file_size)) {
        throw std::runtime_error("Error: No se pudo leer el contenido completo del archivo.");
    }

    return content;
}

static inline void load_or_create_config(GlobalConfig& conf) {

    std::filesystem::create_directories(get_base_path(conf));
    std::filesystem::create_directories(get_db_path());
    std::filesystem::create_directories(get_config_path());
    std::filesystem::create_directories(get_log_path());

    std::ifstream infile(get_config_file());

    //===================================================================
    //  Si el archivo de configuración [config_path] no existe se crea aquí con valores de GlobalConfig.
    //===================================================================
    if (!infile.is_open()) {
        std::ofstream outfile(get_config_file());
        outfile << "# Default DEV_MODE=1" << "\n";
        outfile << "DEV_MODE=" << static_cast<int>(conf.dev_mode) << "\n\n";

        outfile << "# Default DEBUG_MODE=1" << "\n";
        outfile << "DEBUG_MODE=" << static_cast<int>(conf.debug_mode) << "\n\n";

        outfile << "# Default LOG_MODE=0" << "\n";
        outfile << "LOG_MODE=" << static_cast<int>(conf.log_mode) << "\n\n";

        outfile << "# Default MAX_THREADS=2 (minimo 2, máximo 8)" << "\n";
        outfile << "MAX_THREADS=" << conf.max_threads << "\n\n";

        outfile << "# Default PORT=8080" << "\n";
        outfile << "PORT=" << conf.port << "\n\n";

        outfile << "# Default MAX_REQ_BUF_SIZE=8192 (8kb para el buffer de lectura de peticiones TCP)" << "\n";
        outfile << "MAX_REQ_BUF_SIZE=" << conf.max_req_buf_size << "\n\n"; 

        outfile << "# Default SERVERS_PATH=juegos (Sin ruta absoluta se asume \"$HOME/[servers_paths]\". la ruta absoluta length<=31 caracteres.)" << "\n";
        outfile << "SERVERS_PATH=" << conf.servers_path << "\n\n";

        outfile << "Default LOGIN_EXPIRATION=3600 (60 minutos) Min: 1800 (30 min), Max: 172800(48hrs)" << "\n";
        outfile << "LOGIN_EXPIRATION=" << conf.login_expiration << "\n\n";

        outfile << "#Default DB_NAME=atlantis.db" << "\n";
        outfile << "DB_NAME=" << conf.db_name << "\n\n";

        outfile << "Default LOG_FILE=atlantis.log" << "\n";
        outfile << "LOG_FILE=" << conf.log_file << "\n\n";

        outfile.close();
        debug_log("Archivo de configuración por defecto creado...");
        return;
    }

    //===================================================================
    //  El archivo de configuración [config_path] se lee.
    //===================================================================
    std::unordered_map<std::string, std::string> config_file_data;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') { continue; }
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config_file_data[key] = value;
        }
    }

    //===================================================================
    //  Se cargan las configuraciones a GlobalConfig conf.
    //===================================================================
    if (config_file_data.count("DEV_MODE")) {
        conf.dev_mode = static_cast<uint8_t>(std::stoi(config_file_data["DEV_MODE"]) != 0);
    }

    if (config_file_data.count("DEBUG_MODE")) {
        conf.debug_mode = static_cast<uint8_t>(std::stoi(config_file_data["DEBUG_MODE"]) != 0);
    }

    if (config_file_data.count("LOG_MODE")) {
        conf.log_mode = static_cast<uint8_t>(std::stoi(config_file_data["LOG_MODE"]) != 0);
    }

    if (config_file_data.count("PORT")) {
        int port = std::stoi(config_file_data["PORT"]);
        if (port >= 1024 && port <= 65535) {
            conf.port = port;
        } else {
            debug_log("PORT inválido. Usando: " + std::to_string(conf.port));
        }
    }

    if (config_file_data.count("MAX_THREADS")) {
        int threads = std::stoi(config_file_data["MAX_THREADS"]);
        if (threads > 1 && threads <= 8) {
            conf.max_threads = threads;
        } else {
            debug_log("MAX_THREADS inválido. Usando: " + std::to_string(conf.max_threads));
        }
    }

    if (config_file_data.count("LOGIN_EXPIRATION")) {
        int login_expiration = std::stoi(config_file_data["LOGIN_EXPIRATION"]);
        if (login_expiration >= 1800 && login_expiration <= 172800) {
            conf.login_expiration = login_expiration;
        } else {
            debug_log("login_expiration inválido. Usando: " + std::to_string(conf.login_expiration));
        }
    }

    if (config_file_data.count("SERVERS_PATH")) {
        set_path(conf.servers_path, sizeof(conf.servers_path), config_file_data["SERVERS_PATH"].c_str(), "SERVERS_PATH");
    }

    if (config_file_data.count("DB_PATH")) {
        set_path(conf.db_name, sizeof(conf.db_name), config_file_data["DB_NAME"].c_str(), "DB_NAME");
    }

    if (config_file_data.count("LOG_PATH")) {
        set_path(conf.log_file, sizeof(conf.log_file), config_file_data["LOG_PATH"].c_str(), "LOG_PATH");
    }

    if (config_file_data.count("MAX_REQ_BUF_SIZE")) {
        int max_req_buf_size = std::stoi(config_file_data["MAX_REQ_BUF_SIZE"]);
        if (max_req_buf_size >= 1800 && max_req_buf_size <= 172800) {
            conf.max_req_buf_size = max_req_buf_size;
        } else {
            debug_log("max_req_buf_size inválido. Usando: " + std::to_string(conf.max_req_buf_size));
        }
    }

    debug_log("Archivo de configuración cargado...");
}

static inline std::vector<PluginInfo> get_installed_plugins(const std::string& plugin_dir_path) {
    std::vector<PluginInfo> plugins;

    std::filesystem::path plugin_dir(plugin_dir_path);

    if (!std::filesystem::exists(plugin_dir)) {
        throw std::runtime_error("Directorio no existe: " + plugin_dir_path);
    }

    if (!std::filesystem::is_directory(plugin_dir)) {
        throw std::runtime_error("Path no es un directorio: " + plugin_dir_path);
    }

    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jar") {
            std::string file_name = entry.path().filename().string();
            std::uintmax_t size_bytes = entry.file_size();
            double size_mb = std::round((static_cast<double>(size_bytes) / (1024.0 * 1024.0)) * 100.0) / 100.0;

            plugins.push_back(PluginInfo{file_name, size_mb});
        }
    }

    return plugins;
}

static inline std::vector<ModInfo> get_installed_mods(const std::string& mods_dir_path) {
    std::vector<ModInfo> mods;

    std::filesystem::path mods_dir(mods_dir_path);

    if (!std::filesystem::exists(mods_dir)) {
        throw std::runtime_error("Directorio no existe: " + mods_dir_path);
    }

    if (!std::filesystem::is_directory(mods_dir)) {
        throw std::runtime_error("Path no es un directorio: " + mods_dir_path);
    }

    for (const auto& entry : std::filesystem::directory_iterator(mods_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jar") {
            std::string file_name = entry.path().filename().string();
            std::uintmax_t size_bytes = entry.file_size();
            double size_mb = std::round((static_cast<double>(size_bytes) / (1024.0 * 1024.0)) * 100.0) / 100.0;

            mods.push_back(ModInfo{file_name, size_mb});
        }
    }

    return mods;
}

#endif // UTILS_HPP