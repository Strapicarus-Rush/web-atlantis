#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP

#include <string>
#include <thread>
#include <chrono>
#include <utility>
// #include <regex>
#include "utils.hpp"

class ServerInstance {
public:
    bool has_run = false;
    bool has_jar = false;
    bool complete = false;
    bool valid = false;
    bool running = false;
    bool booting = false;
    size_t ram_used = 0;
    int id = -1;
    int active_users = 0;
    double cpu = 0.0;

    std::string name;
    std::string session;
    std::string path;
    // std::string backup_path;
    std::string users_list = "";

    ServerInstance() = default;

    ServerInstance(const std::string& name, const std::string& path, const int& id)
        : id(id), name(name), path(path)  {
        session = sanitize_string(name);
        // backup_path = path + "/backup";
        update_status();
    }

    std::string backup_path() {
        return path + "/backup";
    }

    bool is_valid() {
        return has_run || has_jar;
    }

    bool is_complete() {
        return has_run && has_jar;
    }

    bool check_session_exists() {
        std::string cmd = "tmux has-session -t \"" + session + "\" 2>/dev/null";
        int code = std::system(cmd.c_str());
        return (code != -1 && WEXITSTATUS(code) == 0);
    }

    bool is_running() {
        if(!send_command("list")) [[unlikely]] {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        debug_log("is_running");
        std::string output;
        try {
            output = read_output();
        } catch (...) {
            debug_log("ERROR at read_output_tmux");
            return false;
        }
        std::smatch match;
        if (config.dev_mode && std::regex_search(output, match, regex_test_inicialization)) {
            debug_log("test initialization complete");
            return true;
        }
        for(auto& pattern : regex_count_patterns){
            if (std::regex_search(output, match, pattern)) {
                return true;
            }
        }
        return false;
    }

    bool is_stoped() {
        if (!check_session_exists()) [[unlikely]]
        {
            debug_log("session does0 n exists");
            return true;
        }
        if(!send_command("whoami")) [[unlikely]] {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::string output;
        try {
            output = read_output();
        } catch (...) {
            debug_log("ERROR at read_output_tmux");
            return false;
        }
        std::smatch match;
        if (config.dev_mode && std::regex_search(output, match, regex_confirm_termination)) [[likely]] {
            debug_log("Stop complete ");
            return true;
        }

        return false;
    }

    std::pair<bool, std::string> start_server() {
        std::string cmd = "";
        std::string file_to_run = has_jar ? "java -jar server.jar" : "chmod +x run.sh && ./run.sh";
        std::string message = "El servidor ya está en ejecución";
        if (check_session_exists()) [[unlikely]] {
            if(is_running()){
                return {true, message};
            }else{
                cmd = "tmux send-keys -t \"" + session + "\" "
                    "\"cd \\\"" + path + "\\\" && " + file_to_run + "\"";
            }
        }else{
            cmd = "tmux new-session -d -s \"" + session + "\" "
                    "\"cd \\\"" + path + "\\\" && " + file_to_run + "\"";
        }

        int result = std::system(cmd.c_str());

        if (result != 0) [[unlikely]] {
            return {false, "[ERROR] Falló al iniciar el servidor"};
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(40000)); // 40 segundos de espera antes de confirmar estado         

        running = wait_for_initialization();
        message = running ? "El servidor está ejecutandose." : "Algo pasó pero no era lo que se esperaba";
        return {running, message};
    }

    std::pair<bool, std::string> stop_server() {
        bool success = true;
        std::string message = "[ERROR] La sesión no existe";
        std::string cmd = "stop";
        if (check_session_exists()) [[likely]] {
            if(is_running()) [[likely]] {
                if(config.dev_mode) cmd = "q"; // para pruebas en dev sin servidores ejecutandose. q para salir del reproductor de musica o btop
                bool sent = config.dev_mode ? send_raw_key(cmd) : send_command(cmd);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if(sent){
                    debug_log("command sent");
                    if(wait_for_termination()){
                        std::string cmd = "tmux kill-session -t \"" + session + "\"";
                        int result = std::system(cmd.c_str());
                        if (result != 0) {
                            running = false;
                            message = "Detenido antes del comando";
                        }else{
                            running = false;
                            message = "Servidor detenido correctamente";
                        }
                    }else [[unlikely]] {
                        success = false;
                        message = "No se pudo confirmar la detención.";
                    }
                }else [[unlikely]] {
                    success = false;
                    message = "No se pudo enviar el comando stop.";
                }
            }
        }
        return {success, message};
    }

    bool send_command(const std::string& command) {
        if (!check_session_exists()) [[unlikely]] {
            debug_log("[ERROR] Sesión '" + name + "' no existe.");
            return false;
        }

        std::string cmd = "tmux send-keys -t \"" + session + "\" " + escape_quotes(command) + " C-m";
        int result = std::system(cmd.c_str());

        if (result != 0) [[unlikely]] {
            debug_log("[ERROR] Falló al enviar el comando a '" + name + "'.");
            return false;
        }

        debug_log("[OK]'" + cmd + "'.");
        return true;
    }

    bool send_raw_key(const std::string& command) {
        if (!check_session_exists()) [[unlikely]] {
            debug_log("[ERROR] Sesión '" + name + "' no existe.");
            return false;
        }

        std::string cmd = "tmux send-keys -t \"" + session + "\" " + escape_quotes(command);
        int result = std::system(cmd.c_str());

        if (result != 0) [[unlikely]] {
            debug_log("[ERROR] Falló al enviar el comando a '" + name + "'.");
            return false;
        }

        debug_log("[OK]'" + cmd + "'.");
        return true;
    }

    int get_count_players_online(){
        int player_count = -1;
        if(!send_command("list")) [[unlikely]] {
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        std::string output;
        try {
            output = read_output();
        } catch (...) {
            return -1;
        }
        std::smatch match;
        
        for (const auto& pattern : regex_count_patterns) {
            if (std::regex_search(output, match, pattern)) {
                player_count = std::stoi(match[1]);
                break;
            }
        }
        return player_count;
    }

    std::string get_list_players_online(){
        std::string names = "-1";
        if(!send_command("list")) [[unlikely]] {
            return "-1";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        std::string output;
        try {
            output = read_output();
        } catch (...) {
            return "-1";
        }
        std::smatch match;
        for (const auto& pattern : regex_list_patterns) {
            if (std::regex_search(output, match, pattern)) {
                names = match[1];
                break;
            }
        }
        return names;
    }

    void update_resource_usage() {
        ram_used = get_ram_usage();
        cpu = get_cpu_usage();
        active_users = get_count_players_online();
        users_list = get_list_players_online();
    }

    void update_status() {
        has_run = std::filesystem::exists(path + "/run.sh");
        has_jar = std::filesystem::exists(path + "/server.jar");
        valid = is_valid();
        complete = is_complete();
        if (check_session_exists()) [[likely]]
        {
            running = is_running();
            if (running) [[likely]]
            {
                update_resource_usage();
                debug_log(name + " Inicializado y ejecutandose");
            }else [[unlikely]] {
                debug_log(name + " La sesión existe pero no está en ejecución");
            }
        }else [[unlikely]] {
            debug_log(name + "Sesión no existe en tmux");
        }
    }

    std::string read_output() {
        std::string cmd = "tmux capture-pane -p -t \"" + session + "\"";
        std::array<char, 4096> buffer;
        std::string result;

        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));

        if (!pipe) [[unlikely]] {
            throw std::runtime_error("Failed to run tmux capture-pane");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            std::string line(buffer.data());
            trim_console(line);
            result += line;
        }

        return result;
    }

    std::string read_last_output_line() {
        std::string cmd = "tmux capture-pane -p -t \"" + session + "\"";
        std::array<char, 4096> buffer;
        std::string last_line;

        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        if (!pipe) [[unlikely]] {
            throw std::runtime_error("Failed to run tmux capture-pane");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            std::string line(buffer.data());
            trim_console(line);
            last_line = std::move(line);
        }
        return last_line;
    }

    std::string read_op_json_file() {
        std::string tfp = path+"/ops.json";
        return get_file_op(tfp);
    }

    std::vector<PluginInfo> read_plugins_dir() {
        std::string tdp = path+"/plugins";
        return get_installed_plugins(tdp);
    }

    std::vector<ModInfo> read_mods_dir() {
        std::string tdp = path+"/mods";
        return get_installed_mods(tdp);
    }

    std::string seed_output(){
        if(!send_command("seed")) [[unlikely]] {
            return "Error al enviar comando";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result = "";
        std::string output = read_output();
        std::smatch match;
        for (const auto& pattern : regex_seed_patterns) {
            if (std::regex_search(output, match, pattern)) {
                result = match[1];
            }
        }
        return result;
    }

    std::string border_output(){
        if(!send_command("worldborder get")) [[unlikely]] {
            return "Error al enviar comando";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result = "";
        std::string output = read_output();
        std::smatch match;
        for (const auto& pattern : regex_border_patterns) {
            if (std::regex_search(output, match, pattern)) {
                result = match[1];
                break;
            }
        }
        return result;
    }

    std::string day_time_output(){
        if(!send_command("time query daytime")) [[unlikely]] {
            return "Error al enviar comando";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result = "";
        std::string output = read_output();
        std::smatch match;
        for (const auto& pattern : regex_time_patterns) {
            if (std::regex_search(output, match, pattern)) {
                result = match[1];
                break;
            }
        }

        return result;
    }

    std::string weather_output(){
        if(!send_command("weather query")) [[unlikely]] {
            return "Error al enviar comando";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result = "";
        std::string output = read_output();
        std::smatch match;
        for (const auto& pattern : regex_weather_patterns) {
            if (std::regex_search(output, match, pattern)) {
                result = match[1];
                break;
            }
        }
        return result;
    }

    std::string difficulty_output(){
        if(!send_command("difficulty")) [[unlikely]] {
            return "Error al enviar comando";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result = "";
        std::string output = read_output();
        std::smatch match;
        for (const auto& pattern : regex_difficulty_patterns) {
            if (std::regex_search(output, match, pattern)) {
                result = match[1];
                break;
            }
        }
        return output;
    }

private:
    bool wait_for_initialization(int max_wait_seconds = 40, int interval_seconds = 5) {
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - start_time).count() < max_wait_seconds) {

            if (is_running()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        }

        return false; // timeout
    }
    bool wait_for_termination(int max_wait_seconds = 30, int interval_seconds = 5) {
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - start_time).count() < max_wait_seconds) {
            if(check_session_exists()){
                if (is_stoped()) {
                    return true;
                }
            }else{
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        }

        return false; // timeout
    }

    size_t get_ram_usage() {
        std::string pane_pid = get_pane_pid();
        if (pane_pid.empty()) return 0.0;

        std::string child_cmd = "pgrep -P " + pane_pid + " 2>/dev/null";
        std::unique_ptr<FILE, int(*)(FILE*)> child_pipe(popen(child_cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        std::array<char, 128> child_buf;

        if (!child_pipe) [[unlikely]] {
            throw std::runtime_error("Failed to capture child_pid");
        }
        std::string child_pid;

        if(fgets(child_buf.data(), child_buf.size(), child_pipe.get()) != nullptr) {
            child_pid = std::string(child_buf.data());
        } else{
            throw std::runtime_error("Failed to capture child_pid");
        }

        std::string cmd = "ps -o rss= -p " + child_pid + " 2>/dev/null";
        // FILE* pipe = popen(cmd.c_str(), "r");
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        size_t ram = 0;
        
        if (pipe) {
            std::array<char, 128> buffer;
            if (fgets(buffer.data(), buffer.size(), pipe.get())) {
                ram = std::stoul(buffer.data());
            }
        }
        return ram;
    }

    double get_cpu_usage() {
        std::string pane_pid = get_pane_pid();
        if (pane_pid.empty()) return 0.0;

        std::string child_cmd = "pgrep -P " + pane_pid + " 2>/dev/null";
        std::unique_ptr<FILE, int(*)(FILE*)> child_pipe(popen(child_cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        std::array<char, 128> child_buf;

        if (!child_pipe) [[unlikely]] {
            throw std::runtime_error("Failed to capture child_pid");
        }
        std::string child_pid;

        if(fgets(child_buf.data(), child_buf.size(), child_pipe.get()) != nullptr) {
            child_pid = std::string(child_buf.data());
        } else{
            throw std::runtime_error("Failed to capture child_pid");
        }

        std::string cpu_cmd = "ps -o %cpu= -p " + child_pid + " 2>/dev/null";
        std::unique_ptr<FILE, int(*)(FILE*)> cpu_pipe(popen(cpu_cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        double cpu_val = 0.0;
        if (cpu_pipe) {
            std::array<char, 128> cpu_buf;
            if (fgets(cpu_buf.data(), cpu_buf.size(), cpu_pipe.get())) {
                try {
                    cpu_val = std::stod(cpu_buf.data());
                } catch (...) {
                    cpu_val = 0.0;
                }
            }
        }

        return cpu_val;
    }

    std::string get_pane_pid() {
        std::string cmd = "tmux list-panes -t \"" + session + "\" -F '#{pane_pid}' 2>/dev/null";

        std::array<char, 256> buffer;
        std::string result;

        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
        if (!pipe) {
            throw std::runtime_error("Error al obtener pid: " + cmd);
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        return trim_string(result);
    }

    void trim_console(std::string& s) const {
        const char* ws = " \t\r";
        const auto start = s.find_first_not_of(ws);
        if (start == std::string::npos) {
            s.clear();
            return;
        }
        const auto end = s.find_last_not_of(ws);
        s.erase(end + 1);
        s.erase(0, start);
    }

    std::string escape_quotes(const std::string& input) const {
        std::string escaped;
        for (char c : input) {
            if (c == '"') {
                escaped += "\\\"";
            } else {
                escaped += c;
            }
        }
        return escaped;
    }
};

#endif // SERVERINSTANCE_HPP