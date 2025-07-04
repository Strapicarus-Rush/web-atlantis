#ifndef SERVER_UTILS_HPP
#define SERVER_UTILS_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
// #include <iostream>
#include "debug_log.hpp"

#include "TmuxManager.hpp"
#include "TmuxClient.hpp"
#include <json.hpp>

using json = nlohmann::json;

// ==============================
// ServerInstance Struct
// ==============================
struct ServerInstance {
    std::string name;
    std::string path;
    bool has_run = false;
    bool has_jar = false;
    bool is_active = false;

    ServerInstance(const std::string& name, const std::string& path)
        : name(name), path(path) {
        has_run = std::filesystem::exists(path + "/run.sh");
        has_jar = std::filesystem::exists(path + "/server.jar");
        is_active = TmuxManager::session_exists(name);
    }

    ServerInstance() = default;

    bool is_valid() const {
        return has_run || has_jar;
    }

    bool is_complete() const {
        return has_run && has_jar;
    }

    json to_json() const {
        return {
            {"name", name},
            {"path", path},
            {"has_run", has_run},
            {"has_jar", has_jar},
            {"is_active", is_active}
        };
    }

    static ServerInstance from_json(const json& j) {
        ServerInstance s;
        s.name = j.value("name", "");
        s.path = j.value("path", "");
        s.has_run = j.value("has_run", false);
        s.has_jar = j.value("has_jar", false);
        s.is_active = j.value("is_active", false);
        return s;
    }
};

// ==============================
// Funciones utilitarias
// ==============================

inline std::vector<ServerInstance> cargar_instancias(const std::string& base_dir = "./instances") {
    std::vector<ServerInstance> instances;

    if (!std::filesystem::exists(base_dir)) {
        debug_log("[WARN] Directorio de instancias no existe: " + base_dir);
        return instances;
    }

    for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();
        std::string path = entry.path().string();

        ServerInstance instance(name, path);
        if (instance.is_valid())
            instances.push_back(instance);
    }

    return instances;
}

inline bool crear_instancia(const ServerInstance& instance) {
    if (!instance.has_run) {
        debug_log("[ERROR] Faltante run.sh en: " + instance.path);
        return false;
    }
    return TmuxManager::start_server(instance.name, instance.path);
}

inline bool enviar_comando_a_instancia(const std::string& session_name, const std::string& comando) {
    return TmuxManager::send_command(session_name, comando);
}

inline std::string leer_salida_panel(const std::string& session_name) {
    try {
        TmuxSocketClient client;
        int fd = client.connect_to_tmux_socket();
        return client.read_tmux_pane(fd, session_name);
    } catch (const std::exception& e) {
        return std::string("[ERROR] ") + e.what();
    }
}

inline bool detener_instancia(const std::string& session_name) {
    if (!TmuxManager::session_exists(session_name)) {
        std::cerr << "[INFO] La sesión '" << session_name << "' ya está detenida.\n";
        return false;
    }

    std::string comando = "tmux kill-session -t \"" + session_name + "\"";
    int result = std::system(comando.c_str());
    if (result != 0) {
        std::cerr << "[ERROR] No se pudo detener la sesión '" << session_name << "'.\n";
        return false;
    }

    std::cout << "[OK] Sesión '" << session_name << "' detenida.\n";
    return true;
}

inline bool reiniciar_instancia(const ServerInstance& instance) {
    detener_instancia(instance.name);
    return crear_instancia(instance);
}

// ==============================
// Persistencia del estado
// ==============================

inline void guardar_estado_instancias(const std::vector<ServerInstance>& instances, const std::string& archivo = "estado_instancias.json") {
    json j;
    for (const auto& inst : instances) {
        j.push_back(inst.to_json());
    }

    std::ofstream ofs(archivo);
    if (!ofs) {
        std::cerr << "[ERROR] No se pudo escribir " << archivo << '\n';
        return;
    }

    ofs << j.dump(4);
    std::cout << "[OK] Estado guardado en " << archivo << "\n";
}

inline std::vector<ServerInstance> cargar_estado_desde_archivo(const std::string& archivo = "estado_instancias.json") {
    std::vector<ServerInstance> instances;
    std::ifstream ifs(archivo);
    if (!ifs) {
        std::cerr << "[WARN] No se encontró el archivo de estado: " << archivo << "\n";
        return instances;
    }

    json j;
    try {
        ifs >> j;
        for (const auto& item : j) {
            instances.push_back(ServerInstance::from_json(item));
        }
    } catch (...) {
        std::cerr << "[ERROR] Error al leer o parsear el archivo de estado\n";
    }

    return instances;
}

#endif // SERVER_UTILS_HPP