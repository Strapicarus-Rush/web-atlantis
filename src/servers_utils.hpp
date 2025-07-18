#ifndef SERVER_UTILS_HPP
#define SERVER_UTILS_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <utility>
#include "TmuxManager.hpp"
#include "TmuxClient.hpp"
#include <regex>
#include <json.hpp>
#include "utils.hpp"
#include <thread>

using json = nlohmann::json;

static const std::regex count_regex(R"(There are (\d+) of a max of \d+ players online)");
static const std::regex list_regex(R"(Players online:\s*(.*))");

// struct ServerInstance;

// inline void set_instance_status(const ServerInstance& instance);

// ==============================
// Estatus general
// ==============================

struct GeneralStatus {
    int total_instances = 0;
    int running_instances = 0;
    size_t ram_used = 0;
    int cpu = 0;
    int total_players = 0;
};

// ==============================
// ServerInstance Struct
// ==============================
struct ServerInstance {
    bool has_run = false;
    bool has_jar = false;
    bool complete = false;
    bool is_active = false;
    int  ram_used = 0;
    int  cpu = 0;
    int  active_users = 0;
    std::string name;
    std::string path;
    std::string backup_path;    

    ServerInstance(const std::string& name, const std::string& path)
        : name(name), path(path) {
            // set_instance_status(*this);
        has_run = std::filesystem::exists(path + "/run.sh");
        has_jar = std::filesystem::exists(path + "/server.jar");
        is_active = TmuxManager::session_exists(sanitize_string(name));
    }

    ServerInstance() = default;

    bool is_valid() const {
        return has_run || has_jar;
    }

    // json to_json() const {
    // debug_log("instance to_json\n");

    //     return {
    //         {"name", name},
    //         {"has_run", has_run},
    //         {"has_jar", has_jar},
    //         {"status", is_active},
    //         {"complete", is_complete()},
    //         {"ram_used", ram_usage},
    //         {"cpu", cpu},
    //         {"player_count", active_users},
    //         {"backup"}, backup_path
    //     };
    // }
};

// ==============================
// Funciones utilitarias
// ==============================
bool is_complete(const ServerInstance& instance) {
    return instance.has_run && instance.has_jar;
}

inline json instance_status_json(const ServerInstance& s) {
    json j;
    j["name"] = s.name;
    j["has_run"] = s.has_run ? "Yes" : "No";
    j["has_jar"] = s.has_jar ? "Yes" : "No";
    j["status"] = s.is_active ? "running" : "stopped";
    j["complete"] = is_complete(s) ? "Yes" : "No";
    j["ram_used"] = s.ram_used;
    j["cpu"] = s.cpu;
    j["player_count"] = s.active_users;
    j["backup"] = s.backup_path;

    return j;
}

inline std::pair<bool,std::string> run_instance(const ServerInstance& instance) {
    bool success = false;
    std::string message = "Ejecución correcta.";
    if(!instance.is_valid()){
        message="Instancia no valida";
        return {success, message};
    }
    try{
        if (instance.has_jar)
        {
            success = TmuxManager::start_sh_server(sanitize_string(instance.name), instance.path);
        }else{
            success = TmuxManager::start_jar_server(sanitize_string(instance.name), instance.path);
        } 
        if(!success) message = "Algo pasó... Pero no era lo que se esperaba";
    }catch(std::runtime_error& e){
        message = e.what();
    }
   return {success, message};

}

inline bool send_command_instance(const std::string& session_name, const std::string& comando) {
    return TmuxManager::send_command(sanitize_string(session_name), comando);
}

inline std::string read_output_tmux(const std::string& session_name) {
    try {
        TmuxSocketClient client;
        // int fd = client.connect_to_tmux_socket();
        return client.read_tmux_pane_capture(sanitize_string(session_name));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("[ERROR] ") + e.what());
    }
}

inline std::pair<bool, std::string> stop_tmux_instance(const std::string& session_name) {
    if (!TmuxManager::session_exists(sanitize_string(session_name))) {
        debug_log("[INFO] La sesión '" + session_name + "' ya está detenida.\n");
        return {false, "[INFO] La sesión '" + session_name + "' ya está detenida"};
    }

    std::string comando = "tmux kill-session -t \"" + sanitize_string(session_name) + "\"";
    int result = std::system(comando.c_str());
    if (result != 0) {
        return {false, "[ERROR] No se pudo detener la sesión '" + session_name + "'"};
    }

    debug_log("[OK] Sesión '" + session_name + "' detenida.\n");
    return {true, "[OK] Sesión '" + session_name + "' detenida"};
}

inline std::pair<bool,std::string> reboot_tmux_instance(const ServerInstance& instance) {
    stop_tmux_instance(instance.name);
    return run_instance(instance);
}

inline int get_players_online(const ServerInstance& instance){
    int player_count = 0;
    send_command_instance(instance.name, "list");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string output;
    try {
        output = read_output_tmux(instance.name);
    } catch (...) {
        debug_log("RETURN 0 PLAYERS");
        return 0;
    }
    std::smatch match;
    // Caso 1: "There are X of a max of Y players online"
    if (std::regex_search(output, match, count_regex)) {
        player_count = std::stoi(match[1].str());
    }
    // Caso 2: "Players online: name1, name2"
    else {
        if (std::regex_search(output, match, list_regex)) {
            std::string names = match[1];
            if (!names.empty()) {
                player_count = std::count(names.begin(), names.end(), ',') + 1;
            } else {
                player_count = 0;
            }
        }
    }
    return player_count;
}

inline int get_ram_usage(const ServerInstance& instance){
    int ram = 0;
    std::string cmd = "ps -o rss= -p $(tmux list-panes -t \"" + sanitize_string(instance.name) + "\" -F '#{pane_pid}') 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            ram = std::stoul(buffer);
        }
        pclose(pipe);
    }
    return ram;
}

inline int get_cpu_usage(const ServerInstance& instance) {
    int cpu = 0;
    std::string cmd = "ps -o %cpu= -p $(tmux list-panes -t \"" + sanitize_string(instance.name) + "\" -F '#{pane_pid}') 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            try {
                cpu = static_cast<int>(std::round(std::stod(buffer)));
            } catch (...) {
                cpu = 0;
            }
        }
        pclose(pipe);
    }
    return cpu;
}

inline void set_instance_status(ServerInstance& instance) {
    std::string run_f = instance.path + "/run.sh";
    std::string jar_f = instance.path + "/server.jar";

    instance.has_run = std::filesystem::exists(run_f);
    instance.has_jar = std::filesystem::exists(jar_f);
    instance.complete = is_complete(instance);
    instance.is_active = TmuxManager::session_exists(sanitize_string(instance.name));
    instance.backup_path = instance.path + "/backups";

    if (instance.is_active){
        instance.active_users = get_players_online(instance);
        instance.ram_used = get_ram_usage(instance);
        instance.cpu = get_cpu_usage(instance);
    }
}

inline json get_instance_status(const ServerInstance& instance) {
    ServerInstance status;
    debug_log("get_instance_status\n");

    status.name = instance.name;
    status.path = get_servers_path() + "/" + instance.name;
    set_instance_status(status);

    return instance_status_json(status);
}

static const std::vector<ServerInstance> list_instances() {
    std::vector<ServerInstance> instances;

    if (!std::filesystem::exists(get_servers_path())) {
        throw std::runtime_error("[WARN] Directorio de instancias no existe: " + get_servers_path());
    }

    for (const auto& entry : std::filesystem::directory_iterator(get_servers_path())) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();
        std::string path = entry.path().string();

        ServerInstance instance(name, path);
        if (instance.is_valid()){
            instances.push_back(instance);
        }
    }
    return instances;
}

static const std::vector<std::string> short_list_instances() {
    std::vector<std::string> instances;

    if (!std::filesystem::exists(get_servers_path())) {
        throw std::runtime_error("[WARN] Directorio de instancias no existe: " + get_servers_path());
    }

    for (const auto& entry : std::filesystem::directory_iterator(get_servers_path())) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();
        std::string path = entry.path().string();

        ServerInstance instance(name, path);
        if (instance.is_valid()){
            instances.push_back(name);
        }
    }
    return instances;
}

inline GeneralStatus get_general_status() {
    GeneralStatus status;
    debug_log("get_general_status\n");

    auto instances = list_instances();
    status.total_instances = static_cast<int>(instances.size());

    for (const auto& instance : instances) {
        if (!TmuxManager::session_exists(sanitize_string(instance.name))) continue;

        status.running_instances++;
        status.total_players += get_players_online(instance);
        status.ram_used += get_ram_usage(instance);
        status.cpu += get_cpu_usage(instance);
    }

    return status;
}

inline json general_status_json() {
    GeneralStatus status = get_general_status();
    debug_log("general_status_json\n");
    json j;
    j["status"] = {
        {"total", status.total_instances},
        {"running", status.running_instances},
        {"ram_used", status.ram_used},
        {"cpu", status.cpu},
        {"player_count", status.total_players},

    };
    j["servers"] = short_list_instances();

    return j;
}

#endif // SERVER_UTILS_HPP