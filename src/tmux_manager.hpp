#ifndef TMUXCMDMANAGER_HPP
#define TMUXCMDMANAGER_HPP

#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include "debug_log.hpp"

class TmuxManager {

public:

    static bool session_exists(const std::string& session_name) {
        const std::string cmd = "tmux has-session -t \"" + session_name + "\" 2>/dev/null";
        const int code = std::system(cmd.c_str());
        return code == 0;
    }

    static bool start_server(const std::string& session_name, const std::string& path_to_instance) {
        if (session_exists(session_name)) {
            debug_log("[INFO] Sesión '" + session_name + "' ya existe.");
            return false;
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
            if (c == '"') escaped += "\\\"";
            else escaped += c;
        }
        return escaped;
    }
};

#endif // TMUXCMDMANAGER_HPP

// #ifndef TMUXCLIENT_HPP
// #define TMUXCLIENT_HPP

// #include <sys/socket.h>
// #include <sys/un.h>
// #include <unistd.h>
// #include <stdexcept>
// #include <string>
// #include <cstring>
// #include <cerrno>

// class TmuxSocketClient {

// public:

//     struct ServerInstance {
//         std::string name;
//         uint8_t has_jar = false;
//         uint8_t has_run = false;

//         bool is_valid() const {
//             return has_jar || has_run;
//         }

//         bool is_complete() const {
//             return has_jar && has_run;
//         }
//     };

//     int connect_to_tmux_socket() { // aquí se puede optar por un pasar una variable opcional para el socket con fallback a std::to_string(getuid()).
//         const std::string sock_path = "/tmp/tmux-" + std::to_string(getuid()) + "/default";
//         const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
//         if (fd < 0) {
//             throw std::runtime_error("TMUX socket() failed");
//         }

//         sockaddr_un addr{};
//         addr.sun_family = AF_UNIX;
//         if (sock_path.size() >= sizeof(addr.sun_path)) {
//             ::close(fd);
//             throw std::runtime_error("Socket path too long");
//         }
//         std::memset(addr.sun_path, 0, sizeof(addr.sun_path));
//         std::memcpy(addr.sun_path, sock_path.c_str(), sock_path.size());

//         if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
//             ::close(fd);
//             throw std::runtime_error("connect() to tmux socket at " + sock_path + " failed: " + std::strerror(errno));

//         }

//         return fd;
//     }

//     std::string read_tmux_pane(int fd, const std::string& target_pane) {
//         if(target_pane.empty()) throw std::runtime_error("Nombre panel no definido...");
//         std::string command = "capture-pane -p -t " + target_pane + "\\n";
//         if (::write(fd, command.c_str(), command.size()) < 0) {
//             ::close(fd);
//             throw std::runtime_error("write() to tmux socket failed");
//         }

//         std::string result;
//         char buf[4096];
//         ssize_t n;
//         while ((n = ::read(fd, buf, sizeof(buf))) > 0) {
//             result.append(buf, n);
//             if (result.find("%output") != std::string::npos) {
//                 break;
//             }
//         }

//         if (n < 0) {
//             ::close(fd);
//             throw std::runtime_error("read() from tmux socket failed");
//         }

//         ::close(fd);
//         return result;
//     }
// };

// #endif // TMUXCCLIENTMANAGER_HPP