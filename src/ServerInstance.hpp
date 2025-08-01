#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP

#include <string>
#include <thread>
#include <chrono>
#include <utility>
// #include <regex>
#include <sys/wait.h>
#include "utils.hpp"

class ServerInstance {
public:

    std::atomic<bool> has_run{false};
    std::atomic<bool> has_jar{false};
    std::atomic<bool> complete{false};
    std::atomic<bool> valid{false};
    std::atomic<bool> running{false};
    std::atomic<bool> stopping{false};
    std::atomic<bool> booting{false};
    size_t ram_used = 0;
    int id = -1;
    int active_users = 0;
    double cpu = 0.0;

    std::string name;
    std::string session;
    std::string path;
    std::string users_list;

    ServerInstance() = default;

    ServerInstance(const std::string& name, const std::string& path, int id)
        : id(id), name(name), path(path) {
            session = sanitize_string(name);
            // backup_path = path + "/backup";
            update_status();
        }

    ServerInstance(const ServerInstance&) = delete; // evitar copias accidentales
    ServerInstance& operator=(const ServerInstance&) = delete;

    ServerInstance(ServerInstance&& other) noexcept
        : has_run(other.has_run.load()),
          has_jar(other.has_jar.load()),
          complete(other.complete.load()),
          valid(other.valid.load()),
          running(other.running.load()),
          stopping(other.stopping.load()),
          booting(other.booting.load()),
          ram_used(other.ram_used),
          id(other.id),
          active_users(other.active_users),
          cpu(other.cpu),
          name(std::move(other.name)),
          session(std::move(other.session)),
          path(std::move(other.path)),
          users_list(std::move(other.users_list)) {}

    ServerInstance& operator=(ServerInstance&& other) noexcept {
        if (this != &other) {
            has_run.store(other.has_run.load());
            has_jar.store(other.has_jar.load());
            complete.store(other.complete.load());
            valid.store(other.valid.load());
            running.store(other.running.load());
            stopping.store(other.stopping.load());
            booting.store(other.booting.load());
            ram_used = other.ram_used;
            id = other.id;
            active_users = other.active_users;
            cpu = other.cpu;
            name = std::move(other.name);
            session = std::move(other.session);
            path = std::move(other.path);
            users_list = std::move(other.users_list);
        }
        return *this;
    }

    // bool is_valid() const { return valid.load(); }

    std::string backup_path() {
        return path + "/backup";
    }

    bool is_valid() {
        return has_run.load() || has_jar.load();
    }

    bool is_complete() {
        return has_run.load() && has_jar.load();
    }

    // bool check_session_exists() {
    //     std::string cmd = "tmux has-session -t \"" + session + "\" 2>/dev/null";
    //     debug_log("COMMAND: " + cmd);
    //     int code = std::system(cmd.c_str());
    //     return (code != -1 && WEXITSTATUS(code) == 0);
    // }

    bool check_session_exists() {
        pid_t pid = fork();

        if (pid < 0) {
            // Error al hacer fork
            return false;
        } else if (pid == 0) {
            // Proceso hijo: ejecuta tmux directamente
            execlp("tmux", "tmux", "has-session", "-t", session.c_str(), (char*)nullptr);
            _exit(127); // Si execlp falla, salimos con código de error
        } else {
            // Proceso padre: espera al hijo
            int status = 0;
            if (waitpid(pid, &status, 0) == -1) {
                return false; // Error al esperar
            }
            // Verifica que el hijo haya salido normalmente y con código 0
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
    }

    bool is_running() {
        if(!send_command("list")) [[unlikely]] {
            debug_log("List fail");
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
        // if (config.dev_mode && std::regex_search(output, match, regex_test_inicialization)) {
        //     debug_log("test initialization complete");
        //     return true;
        // }
        for(auto& pattern : regex_count_patterns){
            if (std::regex_search(output, match, pattern)) {
                debug_log("List pattern found");
                running = true;
                booting = false;
                stopping = false;
                return running;
            }
        }
        debug_log("List pattern not found");
        return false;
    }

    bool is_stopped() {
        if (!check_session_exists()) [[unlikely]]
        {
            debug_log("session does exists");
            running = false;
            booting = false;
            stopping = false;
            return true;
        }
        running = false;
        booting = false;
        stopping = true;
        // std::this_thread::sleep_for(std::chrono::milliseconds(150));
        // std::string output;
        // try {
        //     output = read_output();
        // } catch (...) {
        //     debug_log("ERROR at read_output_tmux");
        //     return false;
        // }
        // std::smatch match;
        // if (config.dev_mode && std::regex_search(output, match, regex_confirm_termination)) [[likely]] {
        //     debug_log("Stop complete ");
        //     return true;
        // }

        return false;
    }

    std::pair<bool, std::string> start_server() {
        std::string cmd = "";
        std::string file_to_run = has_run ? "chmod +x run.sh && ./run.sh" : "java -jar server.jar";
        std::string message = "El servidor ya está en ejecución";
        if (check_session_exists()) [[unlikely]] {
            if(is_running()){
                return {true, message};
            }else [[likely]] {
                cmd = "tmux send-keys -t \"" + session + "\" "
                    "\"cd \\\"" + path + "\\\" && " + file_to_run + "\"";
            }
        }else [[likely]] {
            cmd = "tmux new-session -d -s \"" + session + "\" " "\"cd \\\"" + path + "\\\" && " + file_to_run + "\"";
        }

        debug_log("COMMAND: " + cmd);
        int result = std::system(cmd.c_str());

        if (result != 0) [[unlikely]] {
            return {false, "[ERROR] Falló al iniciar el servidor"};
        }

        std::thread([this]() {
            running = wait_for_initialization();
            debug_log("Estado del servidor: " + name + std::string(running ? "iniciado" : " inicio fallido"));
        }).detach();

        message = "El servidor está inicializando.";
        return {true, message};
    }

    std::pair<bool, std::string> stop_server() {
        bool success = true;
        std::string message = "Deteniendo servidor";
        std::string cmd = "stop";
        if (check_session_exists()) [[likely]] {
            if(is_running()) [[likely]] {
                if(send_command(cmd)) [[likely]] {
                    std::thread([this]() {
                        wait_for_termination();
                        debug_log("Estado del servidor: " + name + std::string(running ? "iniciado" : stopping ? "Deteniendose" : "Detenido"));
                    }).detach();
                    stopping = true;
                    debug_log("command sent " + cmd);

                    // std::this_thread::sleep_for(std::chrono::seconds(29)); //29 segundo tarda en cerrar el servidor aprox.
                    // if(wait_for_termination()){
                    //         message = "Servidor detenido correctamente";
                    // } else [[unlikely]] {
                    //     message = check_session_exists() ? "Servidor detenido correctamente":"No se pudo confirmar la detención.";
                    // }
                } else [[unlikely]] {
                    message = !check_session_exists() ? "Servidor detenido correctamente" : "No se pudo enviar el comando stop.";
                }
            } 
            // else [[unlikely]] {
            //     running = false;
            //     std::string cmd = "tmux kill-session -t \"" + session + "\"";
            //     int result = std::system(cmd.c_str());
            //     if (result != 0) [[unlikely]] {
            //         message = "Se Detuvo la sesión de tmux, el servidor no estaba activo";
            //     }else{
            //     message = "No se pudo enviar el comando stop.";
            //     }
            // }
        }
        success = check_session_exists();
        running = success ? false : true;

        return {success, message};
    }

    // bool send_command(const std::string& command) {
    //     if (!check_session_exists()) [[unlikely]] {
    //         debug_log("[ERROR] Sesión '" + name + "' no existe.");
    //         return false;
    //     }

    //     std::string cmd = "tmux send-keys -t \"" + session + "\" " + escape_quotes(command) + " C-m";
    //     debug_log("COMMAND: " + cmd);
    //     int result = std::system(cmd.c_str());

    //     if (result != 0) [[unlikely]] {
    //         debug_log("[ERROR] Falló al enviar el comando a '" + name + "'.");
    //         return false;
    //     }

    //     debug_log("[OK]'" + cmd + "'.");
    //     return true;
    // }

    bool send_command(const std::string& command) {
        if (!check_session_exists()) [[unlikely]] {
            debug_log("[ERROR] Sesión '" + name + "' no existe.");
            return false;
        }

        std::vector<std::string> args = {
            "tmux", "send-keys", "-t", session, command, "C-m"
        };

        // Construimos argv para execvp
        std::vector<char*> argv;
        for (auto& arg : args){
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            debug_log("[ERROR] Fallo en fork().");
            return false;
        } else if (pid == 0) {
            // Proceso hijo
            execvp("tmux", argv.data());
            // Si exec falla
            std::perror("execvp");
            _exit(127);
        } else {
            // Proceso padre
            int status = 0;
            if (waitpid(pid, &status, 0) == -1) {
                debug_log("[ERROR] waitpid falló.");
                return false;
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                debug_log("[OK] Comando enviado a '" + name + "'.");
                return true;
            } else {
                debug_log("[ERROR] Falló al enviar el comando a '" + name + "'.");
                return false;
            }
        }
    }

    // bool send_raw_key(const std::string& command) {
    //     if (!check_session_exists()) [[unlikely]] {
    //         debug_log("[ERROR] Sesión '" + name + "' no existe.");
    //         return false;
    //     }

    //     std::string cmd = "tmux send-keys -t \"" + session + "\" " + escape_quotes(command);
    //     int result = std::system(cmd.c_str());

    //     if (result != 0) [[unlikely]] {
    //         debug_log("[ERROR] Falló al enviar el comando a '" + name + "'.");
    //         return false;
    //     }

    //     debug_log("[OK]'" + cmd + "'.");
    //     return true;
    // }

    bool send_raw_key(const std::string& command) {
        if (!check_session_exists()) [[unlikely]] {
            debug_log("[ERROR] Sesión '" + name + "' no existe.");
            return false;
        }

        // Comando como vector de strings
        std::vector<std::string> args = {
            "tmux", "send-keys", "-t", session, command
        };

        // Convertir a `char* const argv[]`
        std::vector<char*> argv;
        for (auto& arg : args){
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr); // terminador

        pid_t pid = fork();
        if (pid < 0) {
            debug_log("[ERROR] fork() falló.");
            return false;
        } else if (pid == 0) {
            // Proceso hijo: ejecutar tmux
            execvp("tmux", argv.data());
            // Si exec falla
            perror("execvp");
            _exit(127);
        } else {
            // Proceso padre: esperar al hijo
            int status = 0;
            if (waitpid(pid, &status, 0) == -1) {
                debug_log("[ERROR] waitpid() falló.");
                return false;
            }

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                debug_log("[OK] Comando enviado a '" + name + "'.");
                return true;
            } else {
                debug_log("[ERROR] El comando falló para '" + name + "'.");
                return false;
            }
        }
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
        std::string names = "No hay jugadores conectados";
        if(!send_command("list")) [[unlikely]] {
            return "No hay jugadores conectados";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        std::string output;
        try {
            output = read_output();
        } catch (...) {
            return "No hay jugadores conectados";
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

    bool is_booting(){
        if(check_session_exists()) [[likely]] {
            if(is_running()){
                return false;
            } else if (!stopping) {
                booting = true;
                return booting;
            } else {
                booting = false;
                return false;
            }
        } else {
            return false;
        }
    }

    void update_resource_usage() {
        ram_used = get_ram_usage();
        cpu = get_cpu_usage();
        active_users = get_count_players_online();
        users_list = get_list_players_online();
    }

    void reset_resource_usage() {
        running = false;
        booting = false;
        stopping = false;
        ram_used = 0;
        cpu = 0;
        active_users = 0;
        users_list = "No hay jugadores conectados";
    }

    void update_status() {
        has_run = std::filesystem::exists(path + "/run.sh");
        has_jar = std::filesystem::exists(path + "/server.jar");
        valid = is_valid();
        complete = is_complete();
        if (check_session_exists()) [[likely]]
        {
            if (is_running()) [[likely]]
            {
                update_resource_usage();
                debug_log(name + " Inicializado y ejecutandose");
            }else if(is_booting()) {
                update_resource_usage();
                debug_log(name + "El servidor está inicializando");
            }
        }else [[unlikely]] {
            reset_resource_usage();
            debug_log(name + " Sesión no existe en tmux");
        }
    }

    // std::string read_output() {
    //     std::string cmd = "tmux capture-pane -p -t \"" + session + "\"";
    //     std::array<char, 4096> buffer;
    //     std::string result;

    //     std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));

    //     if (!pipe) [[unlikely]] {
    //         throw std::runtime_error("Failed to run tmux capture-pane");
    //     }

    //     while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    //         std::string line(buffer.data());
    //         trim_console(line);
    //         result += line;
    //     }

    //     return result;
    // }
    std::string read_output() {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            throw std::runtime_error("pipe() failed");
        }

        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("fork() failed");
        }

        if (pid == 0) {
            // Proceso hijo
            close(pipefd[0]); // Cerrar lectura

            // Redirigir stdout al pipe de escritura
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // Comando tmux capture-pane -p -t "session"
            std::vector<char*> args = {
                const_cast<char*>("tmux"),
                const_cast<char*>("capture-pane"),
                const_cast<char*>("-p"),
                const_cast<char*>("-t"),
                const_cast<char*>(session.c_str()),
                nullptr
            };

            execvp("tmux", args.data());

            // Si exec falla
            _exit(127);
        }

        // Proceso padre
        close(pipefd[1]); // Cerrar escritura

        std::ostringstream output;
        std::array<char, 4096> buffer;
        ssize_t count;
        while ((count = read(pipefd[0], buffer.data(), buffer.size())) > 0) {
            output.write(buffer.data(), count);
        }

        close(pipefd[0]);

        // Esperar al hijo
        int status = 0;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            throw std::runtime_error("tmux capture-pane failed or exited abnormally");
        }

        return output.str();
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
    bool wait_for_initialization(int max_wait_seconds = 120, int interval_seconds = 10) {
        auto start_time = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(160));
        while (std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - start_time).count() < max_wait_seconds) {
            if (is_running()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        }

        return false; // timeout
    }
    bool wait_for_termination(int max_wait_seconds = 50, int interval_seconds = 10) {
        auto start_time = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(30));
        while (std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - start_time).count() < max_wait_seconds) {
            if(check_session_exists()){
                if (is_stopped()) {
                    running = false;
                    booting = false;
                    stopping = false;
                    return true;
                }
                running = false;
                booting = false;
                stopping = true;
            }else{
                stopping = false;
                running = false;
                booting = false;
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        }

        return false; // timeout
    }

    // size_t get_ram_usage() {
    //     std::string pane_pid = get_pane_pid();
    //     if (pane_pid.empty()) return 0.0;

    //     std::string child_cmd = "pgrep -P " + pane_pid + " 2>/dev/null";
    //     std::unique_ptr<FILE, int(*)(FILE*)> child_pipe(popen(child_cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
    //     std::array<char, 128> child_buf;

    //     if (!child_pipe) [[unlikely]] {
    //         throw std::runtime_error("Failed to capture child_pid");
    //     }
    //     std::string child_pid;

    //     if(fgets(child_buf.data(), child_buf.size(), child_pipe.get()) != nullptr) {
    //         child_pid = std::string(child_buf.data());
    //     } else{
    //         throw std::runtime_error("Failed to capture child_pid");
    //     }

    //     std::string cmd = "ps -o rss= -p " + child_pid + " 2>/dev/null";
    //     // FILE* pipe = popen(cmd.c_str(), "r");
    //     std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
    //     size_t ram = 0;
        
    //     if (pipe) {
    //         std::array<char, 128> buffer;
    //         if (fgets(buffer.data(), buffer.size(), pipe.get())) {
    //             ram = std::stoul(buffer.data());
    //         }
    //     }
    //     return ram;
    // }

    size_t get_ram_usage() {
        std::string pane_pid = get_pane_pid();
        if (pane_pid.empty()) return 0;

        std::vector<std::string> pids;
        std::vector<std::string> stack = { pane_pid };

        while (!stack.empty()) {
            std::string pid = stack.back();
            stack.pop_back();
            pids.push_back(pid);

            std::ifstream children_file("/proc/" + pid + "/task/" + pid + "/children");
            if (!children_file.is_open()) continue;

            std::string line;
            if (std::getline(children_file, line)) {
                std::istringstream iss(line);
                std::string child_pid;
                while (iss >> child_pid) {
                    stack.push_back(child_pid);
                }
            }
        }

        size_t total_ram_kb = 0;
        for (const auto& pid : pids) {
            std::ifstream status_file("/proc/" + pid + "/status");
            std::string line;
            while (std::getline(status_file, line)) {
                if (line.rfind("VmRSS:", 0) == 0) {
                    std::istringstream iss(line);
                    std::string key;
                    size_t value;
                    std::string unit;
                    iss >> key >> value >> unit;
                    total_ram_kb += value;
                    break;
                }
            }
        }

        return total_ram_kb;
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

    // std::string get_pane_pid() {
    //     std::string cmd = "tmux list-panes -t \"" + session + "\" -F '#{pane_pid}' 2>/dev/null";

    //     std::array<char, 256> buffer;
    //     std::string result;

    //     std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
    //     if (!pipe) {
    //         throw std::runtime_error("Error al obtener pid: " + cmd);
    //     }

    //     while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    //         result += buffer.data();
    //     }

    //     return trim_string(result);
    // }

    std::string get_pane_pid() {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            throw std::runtime_error("Error al crear pipe");
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            throw std::runtime_error("Error al hacer fork");
        }

        if (pid == 0) {
            // Proceso hijo
            dup2(pipefd[1], STDOUT_FILENO); // redirige stdout al pipe
            close(pipefd[0]); // cerramos lectura en hijo
            close(pipefd[1]);

            execlp("tmux", "tmux", "list-panes", "-t", session.c_str(), "-F", "#{pane_pid}", (char*)nullptr);
            _exit(127); // solo si execlp falla
        }

        // Proceso padre
        close(pipefd[1]); // cerramos escritura
        std::string result;
        char buffer[256];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            result.append(buffer, count);
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

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