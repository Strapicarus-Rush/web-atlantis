#ifndef TMUXCMDMANAGER_HPP
#define TMUXCMDMANAGER_HPP

#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include "utils.hpp"

class TmuxManager {

public:

    static bool session_exists(const std::string& session_name) {
        const std::string cmd = "tmux has-session -t \"" + session_name + "\" 2>/dev/null";
        const int code = std::system(cmd.c_str());
        
        if (code == -1) {
            // Error al lanzar el proceso
            return false;
        }

        // Extraer código de salida real
        int exit_code = WEXITSTATUS(code);
        return exit_code == 0;
    }

    static bool start_sh_server(const std::string& session_name, const std::string& path_to_instance) {
        if (session_exists(session_name)) {
            throw std::runtime_error("La instancia está ya existe.");
            // debug_log("[INFO] Sesión '" + session_name + "' ya existe.");
            // return false;
        }

        const std::string command = "tmux new-session -d -s \"" + session_name + "\" " + "\"cd \\\"" + path_to_instance + "\\\" && chmod +x run.sh && ./run.sh\"";

        const int result = std::system(command.c_str());

        if (result != 0) {
            debug_log("[ERROR] Falló al iniciar la sesión tmux para " + session_name);
            return false;
        }

        debug_log("[OK] Servidor '" + session_name + "' iniciado en tmux.");
        return true;
    }

    static bool start_jar_server(const std::string& session_name, const std::string& path_to_instance) {
        if (session_exists(session_name)) {
            throw std::runtime_error("La instancia está ya existe.");
            // debug_log("[INFO] Sesión '" + session_name + "' ya existe.");
            // return false;
        }

        const std::string command = "tmux new-session -d -s \"" + session_name + "\" " + "\"cd \\\"" + path_to_instance + "\\\" && chmod +x run.sh && ./run.sh\"";

        const int result = std::system(command.c_str());

        if (result != 0) {
            debug_log("[ERROR] Falló al iniciar la sesión tmux para " + session_name);
            return false;
        }

        debug_log("[OK] Servidor '" + session_name + "' iniciado en tmux.");
        return true;
    }

    static bool send_command(const std::string& session_name, const std::string& command) {
        if (!session_exists(session_name)) {
            debug_log("[ERROR] Sesión '" + session_name + "' no existe.");
            return false;
        }

        const std::string cmd = "tmux send-keys -t \"" + session_name + "\" \"" + escape_quotes(command) + "\" C-m";
        debug_log("EJECUTANDO:" +cmd);
        const int result = std::system(cmd.c_str());

        if (result != 0) {
            debug_log("[ERROR] Falló al enviar el comando a '" + session_name + "'.");
            return false;
        }

        debug_log("[OK] Comando enviado a '" + session_name + "'.");
        return true;
    }

private:
    static std::string escape_quotes(const std::string& input) {
        std::string escaped;
        for (char c : input) {
            if (c == '"'){
                escaped += "\\\"";
            } 
            else {
                escaped += c;
            }
        }
        return escaped;
    }
};

#endif // TMUXCMDMANAGER_HPP
