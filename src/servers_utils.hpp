#ifndef SERVER_UTILS_HPP
#define SERVER_UTILS_HPP

#include "InstanceManager.hpp"
#include <json.hpp>

using json = nlohmann::json;
std::mutex InstanceManager::instance_mutex;
std::vector<ServerInstance> InstanceManager::instances;

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
// Funciones utilitarias
// ==============================

inline json instance_status_json(ServerInstance* s) {
    json j;
    j["name"] = s->name;
    j["has_run"] = s->has_run ? "Yes" : "No";
    j["has_jar"] = s->has_jar ? "Yes" : "No";
    j["status"] = s->running ? "running" : "stopped";
    j["complete"] = s->complete ? "Yes" : "No";
    j["ram_used"] = s->ram_used;
    j["cpu"] = s->cpu;
    j["player_count"] = s->active_users;
    j["backup"] = s->backup_path();

    return j;
}

static void load_instances_if_needed() {
    std::lock_guard<std::mutex> lock(InstanceManager::instance_mutex);
    if (InstanceManager::instances.empty()) {
        debug_log("[INFO] Cargando instancias desde disco...");
        InstanceManager::load_instances_from_folder();
    }
}

inline std::pair<bool, std::string> send_instance_command(const std::string& instance_name, const std::string& command){
    load_instances_if_needed();
    bool success = false;
    std::string message = "Instancia no encontrada";
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return {success, message};
    }

    if (!instance->send_command(command)) [[unlikely]]
    {
        message = "Falló envio comando";
    }else{
        success = true;
        message = "Comando enviado";
    }

    return {success, message};
}

static std::vector<ServerInstance>& list_instances() {
    load_instances_if_needed();
    return InstanceManager::instances;
}

static std::vector<std::string> short_list_instances() {
    load_instances_if_needed();
    std::vector<std::string> names;
    for (const auto& instance : InstanceManager::instances) {
        names.push_back(instance.name);
    }
    return names;
}

inline GeneralStatus get_general_status() {
    GeneralStatus status;
    debug_log("get_general_status\n");

    auto& instances = list_instances();
    status.total_instances = static_cast<int>(instances.size());

    for (auto& instance : instances) {
        if (!instance.check_session_exists()) {
            continue;
        }
        if(instance.is_running()){
            instance.update_status();
            status.running_instances++;
            status.total_players += instance.active_users;
            status.ram_used += instance.ram_used;
            status.cpu += instance.cpu;
        } 
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

inline json get_instance_status(const std::string& instance_name) {
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        throw std::runtime_error("[ERROR] Instancia no encontrada: " + instance_name);
    }

    instance->update_status();

    return instance_status_json(instance);
}

inline std::pair<bool, std::string> run_instance(const std::string& instance_name) {
    load_instances_if_needed();

    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return {false, "Instancia no encontrada"};
    }

    if (!instance->is_valid()) [[unlikely]] {
        return {false, "Instancia no válida"};
    }

    return instance->start_server();
}

inline std::pair<bool, std::string> stop_instance_session(const std::string& instance_name) {
    load_instances_if_needed();
    auto instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        debug_log("[INFO] La instancia '" + instance_name + "' no existe");
        return {true, "[INFO] La instancia '" + instance_name + "' no existe"};
    }

    auto [result, message] = instance->stop_server();

    return {result, message};
}

inline std::pair<bool, std::string> reboot_instance(const std::string& instance_name) {
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return {false, "Instancia no encontrada"};
    }

    auto [stop_ok, stop_msg] = instance->stop_server();
    if (!stop_ok) [[unlikely]] {
        return {false, stop_msg};
    }

    return instance->start_server();
}

inline std::string get_instance_console(const std::string instance_name){
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return "Instancia no encontrada";
    }

    if (!instance->check_session_exists())
    {
        return "Instancia de tmux inexistente";
    }
    return instance->read_output();
}

inline int get_count_players(const std::string& instance_name){
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return -1;
    }
    if (!instance->is_running())
    {
        return -1;
    }
    instance->update_status();
    return instance->active_users;
}

inline std::string get_list_players(const std::string instance_name){
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return "Instancia no encontrada";
    }

    if (!instance->is_running())
    {
        return "";
    }
    instance->update_status();
    return instance->users_list;
}

// inline std::string get_white_list(const std::string instance_name{
//     load_instances_if_needed();
//     ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
//     if (!instance) {
//         return "Instancia no encontrada";
//     }

//     if (!instance->is_running())
//     {
//         return "";
//     }
//     instance->update_status();
//     return instance->users_list;
// })

inline std::string get_op_file(std::string instance_name){
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        return "Instancia no encontrada";
    }
    return instance->read_op_json_file();
}

json get_installed_plugins_json(const std::string& instance_name) {
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) [[unlikely]] {
        throw std::runtime_error("Instancia no encontrada");
    }

    json result;
    auto plugins = instance->read_plugins_dir();
    for (const auto& plugin : plugins) {
        result.push_back({
            {"plugin", plugin.name},
            {"size_mb", plugin.size_mb}
        });
    }
    return result;
}

json get_installed_mods_json(const std::string& instance_name) {
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) {
        throw std::runtime_error("Instancia no encontrada");
    }

    json result;
    auto mods = instance->read_mods_dir();
    for (const auto& mod : mods) {
        result.push_back({
            {"mod", mod.name},
            {"size_mb", mod.size_mb}
        });
    }
    return result;
}

json get_world_info_json(const std::string& instance_name) {
    load_instances_if_needed();
    ServerInstance* instance = InstanceManager::get_instance_by_name(instance_name);
    if (!instance) {
        throw std::runtime_error("Instancia no encontrada");
    }
    json result;
    result["seed"] = instance->seed_output() ;
    result["border"] = instance->border_output() ;
    result["time"] = instance->day_time_output() ;
    result["weather"] = instance->weather_output() ;
    result["difficulty"] = instance->difficulty_output() ;
    return result;
}

#endif // SERVER_UTILS_HPP