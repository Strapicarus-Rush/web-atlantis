// #ifndef CONFIG_HPP
// #define CONFIG_HPP

// #include <cstdint>
// #include <cstdio>
// #include <iostream>

// struct alignas(4) GlobalConfig {

//     uint8_t dev_mode    = true;
//     uint8_t debug_mode  = true;
//     uint8_t log_mode    = false;
//     uint8_t padding     = 0; // para mantener alineación en memoria (experimental, no entiendo como funciona aún.)
//     int     port        = 8080;
//     int     max_threads = 2;
//     int     max_req_buf_size = 8192  // 8kb para el buffer de lectura, si el tamaño de la petición es mayor se trunca.

//     // ========================
//     // Para la ruta de las instalaciones de los servidores minecraft.
//     // Sin ruta absoluta se asume "/home/user/[servers_paths]". la ruta absoluta requiere length<=31 caracteres.
//     // ========================
//     char servers_paths[32] = "juegos"
// };

// inline std::string get_base_path(const GlobalConfig& conf) {
//     const char* home = getenv("HOME");
//     if (!home) {
//         home = getpwuid(getuid())->pw_dir;
//     }

//     std::string app_path;
//     if (conf.dev_mode)
//     {
//       // Ruta del ejecutable, se asume la carpeta build del projecto durante el desarrollo
//       std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");

//       // Subir un nivel desde el directorio build a la raiz del proyecto para tener acceso a public
//       std::filesystem::path basePath = exePath.parent_path().parent_path();
//       app_path = basePath;      
//     }else{
//       app_path = std::string(home) + "/.web-atlantis";
//     }
//     debug_log(app_path);
//     return app_path;
// }

// inline std::string get_public_path() {
//     return get_base_path() + "/public";
// }

// inline std::string get_log_path() {
//     return get_base_path() + "/server.log";
// }


// inline void set_path(GlobalConfig& conf, const char* path) {
//     constexpr std::size_t max_len = sizeof(conf.servers_path);

//     int written = std::snprintf(conf.servers_path, max_len, "%s", path);

//     if (written < 0) {
//         std::cerr << "Error: Fallo al copiar SERVERS_PATH al buffer.\n";
//     } else if (static_cast<std::size_t>(written) >= max_len) {
//         std::cerr << "Advertencia: SERVERS_PATH fue truncado a "
//                   << max_len - 1 << " caracteres.\n";
//     }
// }

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

// // void load_or_create_config(GlobalConfig& conf);
// // inline std::vector<std::string> split(const std::string& str, char delim);

// #endif // CONFIG_HPP