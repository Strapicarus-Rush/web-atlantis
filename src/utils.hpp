#ifndef UTILS_HPP
#define UTILS_HPP

#include <unordered_map>
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <sqlite3.h>
#include <unistd.h>
#include <pwd.h>
// #include "debug_log.hpp"
// #include "config.hpp"

static constexpr size_t MAX_INSTANCES = 4;

struct alignas(16) GlobalConfig {

    uint8_t dev_mode    = true;
    uint8_t debug_mode  = true;
    uint8_t log_mode    = false;
    uint8_t padding     = 0; // para mantener alineación en memoria (experimental)
    int     port        = 8080;
    int     max_threads = 2;
    int     max_req_buf_size = 8192;  // 8kb para el buffer de lectura, si el tamaño de la petición es mayor se trunca.

    // ========================
    // Para la ruta de las instalaciones de los servidores minecraft.
    // Sin ruta absoluta se asume "/home/[currentUser]/[servers_paths]". la ruta absoluta requiere length<=31 caracteres.
    // ========================
    char servers_path[32] = "juegos";
    char config_file[16] = "server.config";
};

inline GlobalConfig config;

struct alignas(32) ServerInstance {
    uint64_t active_at = 0;     // 8 bytes
    char name[16] = {};         // 16 bytes
    uint8_t has_jar = false;    // 1 byte
    uint8_t has_run = false;    // 1 byte
    uint8_t active = false;     // 1 byte
    uint8_t users = 24;         // 1 byte
    uint8_t _padding[4] = {};   // padding explícito para llegar a 32B (para alineación en memoria)

    bool is_valid() const { return has_jar || has_run; }
    bool is_complete() const { return has_jar && has_run; }
};

inline ServerInstance serversPool[MAX_INSTANCES] = {};

// size_t count = list_minecraft_servers(config, serversPool, MAX_INSTANCES);

inline ServerInstance create_instance(const std::string& name, uint64_t active_at,
                               bool has_jar, bool has_run, bool active, uint8_t users) {
    ServerInstance inst{};
    inst.active_at = active_at;

    constexpr size_t max_len = sizeof(inst.name);
    int written = std::snprintf(inst.name, max_len, "%s", name.c_str());

    if (written < 0) {
        std::cerr << "Error: Fallo al copiar nombre al buffer.\n";
        inst.name[0] = '\0'; // fallback seguro
    } else if (static_cast<size_t>(written) >= max_len) {
        std::cerr << "Advertencia: Nombre truncado a " << max_len - 1 << " caracteres.\n";
    }

    inst.has_jar = has_jar;
    inst.has_run = has_run;
    inst.active = active;
    inst.users = users;
    return inst;
}

inline std::ofstream log_file;

inline void debug_log_internal(const std::string& msg, const char* file, int line, const char* func) {
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

inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \r\n\t");
    auto end = s.find_last_not_of(" \r\n\t");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline std::string get_mime_type(const std::string& raw_path) {
    const std::string path = to_lower(trim(raw_path));
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".txt")) return "text/plain; charset=utf-8";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
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

inline std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string s;
    while (std::getline(iss, s, delim))
        parts.push_back(s);
    return parts;
}

inline std::string get_base_path(const GlobalConfig& conf) {
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
    debug_log(app_path);
    return app_path;
}

inline std::string get_db_path() {
    return get_base_path(config) + "/atlantis.db";
}

inline std::string get_public_path() {
    return get_base_path(config) + "/public";
}

inline std::string get_log_path() {
    return get_base_path(config) + "/server.log";
}


inline void set_path(GlobalConfig& conf, const char* path) {
    constexpr std::size_t max_len = sizeof(conf.servers_path);

    int written = std::snprintf(conf.servers_path, max_len, "%s", path);

    if (written < 0) {
        std::cerr << "Error: Fallo al copiar SERVERS_PATH al buffer. Se deja vacio\n";
        conf.servers_path[0] = '\0'; // fallback seguro
    } else if (static_cast<std::size_t>(written) >= max_len) {
        std::cerr << "Advertencia: SERVERS_PATH fue truncado a "
                  << max_len - 1 << " caracteres.\n";
    }
}

// // ========================
// // Para la ruta de las instalaciones de los servidores minecraft
// // ========================
// inline std::string normalize_server_path(const GlobalConfig& conf) {
//     // Si la ruta ya es absoluta, no hay nada que hacer
//     if (conf.servers_path[0] == '/') {
//         return conf.servers_path; 
//     }

//     std::string full_path = get_base_path() + "/" + conf.servers_path;

//     return full_path;
// }



//===================================================================
//  TODO: Revisar inicializaciones de arichivos base, utilizar directorio public y no strings en el binario.
//===================================================================

// void ensure_public_files(const std::string& base_path) {
//     const std::string public_dir = base_path + "/public";
//     // const std::string panel_dir = public_dir + "/panel";
//     // const std::string error_dir = public_dir + "/error";
//     // const std::string css_dir = public_dir + "/css";
//     // const std::string assets = public_dir + "/assets";
//     // const std::string js = public_dir + "/js";

//     // std::filesystem::create_directories(public_dir);
//     // std::filesystem::create_directories(panel_dir);
//     // std::filesystem::create_directories(error_dir);
//     // std::filesystem::create_directories(css_dir);

//     // const std::string index_file = public_dir + "/index.html";
//     // const std::string panel_file = panel_dir + "/index.html";
//     // const std::string style_file = error_dir + "/style.css";

//     // if (!std::filesystem::exists(index_file)) {
//     //     std::ofstream(index_file) << index_html;
//     // }

//     // if (!std::filesystem::exists(panel_file)) {
//     //     std::ofstream(panel_file) << panel_html;
//     // }

//     // if (!std::filesystem::exists(style_file)) {
//     //     std::ofstream(style_file) << style_css;
//     // }
// }

//===================================================================
//  TODO: mover a config.hpp?
//===================================================================
inline void load_or_create_config(GlobalConfig& conf) {
    const std::string base_path = get_base_path(conf);
    const std::string config_path = base_path + "/" + conf.config_file;

    std::filesystem::create_directories(base_path);
    // ensure_public_files(base_path);

    std::ifstream infile(config_path);

    //===================================================================
    //  Si el archivo de configuración [config_path] no existe se crea aquí con valores de GlobalConfig.
    //===================================================================
    if (!infile.is_open()) {
        std::ofstream outfile(config_path);
        outfile << "# Default DEV_MODE=1" << "\n";
        outfile << "DEV_MODE=" << static_cast<int>(conf.dev_mode) << "\n";
        outfile << "# Default DEBUG_MODE=1" << "\n";
        outfile << "DEBUG_MODE=" << static_cast<int>(conf.debug_mode) << "\n";
        outfile << "# Default LOG_MODE=0" << "\n";
        outfile << "LOG_MODE=" << static_cast<int>(conf.log_mode) << "\n";
        outfile << "# Default PORT=8080" << "\n";
        outfile << "PORT=" << conf.port << "\n";
        outfile << "# Default MAX_THREADS=2 (minimo 2, máximo 8)" << "\n";
        outfile << "MAX_THREADS=" << conf.max_threads << "\n";
        outfile << "# Default SERVERS_PATH=juegos (Sin ruta absoluta se asume \"/home/user/[servers_paths]\". la ruta absoluta length<=31 caracteres.)" << "\n";
        outfile << "SERVERS_PATH=" << conf.servers_path << "\n";
        outfile << "# Default MAX_REQ_BUF_SIZE=8192 (8kb para el buffer de lectura de peticiones TCP)" << "\n";
        outfile << "MAX_REQ_BUF_SIZE=" << conf.max_req_buf_size << "\n"; 
        outfile.close();
        debug_log("Archivo de configuración por defecto creado...");
        return;
    }

    //===================================================================
    //  El archivo de configuración [config_path] se lee aquí.
    //===================================================================
    std::unordered_map<std::string, std::string> config;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') { continue; }
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
        }
    }

    //===================================================================
    //  Se cargan las configuraciones a GlobalConfig conf.
    //===================================================================
    if (config.count("DEV_MODE")) {
        conf.dev_mode = static_cast<uint8_t>(std::stoi(config["DEV_MODE"]) != 0);
    }
    if (config.count("DEBUG_MODE")) {
        conf.debug_mode = static_cast<uint8_t>(std::stoi(config["DEBUG_MODE"]) != 0);
    }
    if (config.count("LOG_MODE")) {
        conf.log_mode = static_cast<uint8_t>(std::stoi(config["LOG_MODE"]) != 0);
    }

    if (config.count("PORT")) {
        int port = std::stoi(config["PORT"]);
        if (port >= 1024 && port <= 65535) {
            conf.port = port;
        } else {
            debug_log("PORT inválido. Usando: " + std::to_string(conf.port));
        }
    }

    if (config.count("MAX_THREADS")) {
        int threads = std::stoi(config["MAX_THREADS"]);
        if (threads > 0 && threads <= 8) {
            conf.max_threads = threads;
        } else {
            debug_log("MAX_THREADS inválido. Usando: " + std::to_string(conf.max_threads));
        }
    }

    if (config.count("SERVERS_PATH")) {
        set_path(conf, config["SERVERS_PATH"].c_str());
    }
    debug_log("Archivo/configuración cargado...");
}

// inline std::string get_public_path() {
//     return get_base_path(config) + "/public";
// }

// inline std::string get_log_path() {
//     return get_base_path(config) + "/server.log";
// }



inline size_t list_minecraft_servers(const GlobalConfig& conf, size_t max_count) {
    size_t count = 0;

    std::filesystem::path root = conf.servers_path[0] == '/' 
        ? std::filesystem::path(conf.servers_path)
        : std::filesystem::path(get_base_path(conf)) / conf.servers_path;

    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        debug_log("Directorio raíz inválido: " + root.string());
        return 0;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (count >= max_count) {
            break;
        }

        if (!entry.is_directory()) {
            continue;
        }

        const std::string name = entry.path().filename().string();
        if (name.size() >= sizeof(serversPool[count].name)) {
            debug_log("Nombre demasiado largo: " + name);
            continue;
        }

        // SAFE INIT: no memset, just value-initialize the struct
        serversPool[count] = ServerInstance{};
        ServerInstance& inst = serversPool[count];

        std::memcpy(inst.name, name.c_str(), name.size());
        inst.has_jar = std::filesystem::exists(entry.path() / "server.jar");
        inst.has_run = std::filesystem::exists(entry.path() / "run.sh");
        inst.active_at = std::time(nullptr);

        if (inst.is_valid()) {
            ++count;
        }
    }

    return count;
}
// inline size_t list_minecraft_servers(const GlobalConfig& conf, size_t max_count) {

//     size_t count = 0;

//     std::filesystem::path root = conf.servers_path[0] == '/' ? std::filesystem::path(conf.servers_path)
//                                                 : std::filesystem::path(get_base_path(conf)) / conf.servers_path;

//     if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
//         debug_log("Directorio raíz inválido: " + root.string());
//         return 0;
//     }

//     for (const auto& entry : std::filesystem::directory_iterator(root)) {
//         if (count >= max_count) {
//             break;
//         }

//         if (!entry.is_directory()) {
//             continue;
//         }

//         const std::string name = entry.path().filename().string();
//         if (name.size() >= sizeof(out[count].name)) {
//             debug_log("Nombre demasiado largo: " + name);
//             continue;
//         }

//         ServerInstance& inst = out[count];
//         std::memset(&inst, 0, sizeof(ServerInstance));

//         std::memcpy(inst.name, name.c_str(), name.size());
//         inst.has_jar = std::filesystem::exists(entry.path() / "server.jar");
//         inst.has_run = std::filesystem::exists(entry.path() / "run.sh");
//         inst.active_at = std::time(nullptr);

//         if (inst.is_valid()) {
//             ++count;
//         }
//     }

//     return count;
// }


#endif // UTILS_HPP