#ifndef TMUXCLIENT_HPP
#define TMUXCLIENT_HPP

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cerrno>

class TmuxSocketClient {
    
public:
    // int connect_to_tmux_socket(const std::string& sock_path = "/tmp/tmux-" + std::to_string(getuid()) + "/default") {
    //     int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    //     if (fd < 0) {
    //         throw std::runtime_error("TMUX socket() failed");
    //     }

    //     sockaddr_un addr{};
    //     addr.sun_family = AF_UNIX;
    //     if (sock_path.size() >= sizeof(addr.sun_path)) {
    //         ::close(fd);
    //         throw std::runtime_error("Socket path too long");
    //     }

    //     std::memset(addr.sun_path, 0, sizeof(addr.sun_path));
    //     std::memcpy(addr.sun_path, sock_path.c_str(), sock_path.size());

    //     if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    //         ::close(fd);
    //         throw std::runtime_error("connect() to tmux socket at " + sock_path + " failed: " + std::strerror(errno));
    //     }
    //     debug_log("CONNECTED TO TMUX SOCKET");
    //     return fd;
    // }

    // std::string read_tmux_pane(int fd, const std::string& target_pane) {
    //     debug_log("START READ TMUX PANE");
    //     if (target_pane.empty()){
    //         throw std::runtime_error("Nombre panel no definido...");
    //     }

    //     std::string command = "capture-pane -p -t " + target_pane + "\n";
    //     if (::write(fd, command.c_str(), command.size()) < 0) {
    //         ::close(fd);
    //         throw std::runtime_error("write() to tmux socket failed");
    //     }

    //     std::string result;
    //     char buf[4096];
    //     ssize_t n;
    //     while ((n = ::read(fd, buf, sizeof(buf))) > 0) {
    //         debug_log("READING TMUX PANE...");
    //         result.append(buf, n);
    //         if (result.find("%output") != std::string::npos) break;
    //     }

    //     if (n < 0) {
    //         ::close(fd);
    //         throw std::runtime_error("read() from tmux socket failed");
    //     }

    //     ::close(fd);
    //     debug_log("END READ TMUX PANE");
    //     return result;
    // }

    std::string read_tmux_pane_capture(const std::string& target_pane) {
    if (target_pane.empty()) {
        throw std::runtime_error("Target pane is empty");
    }

    std::string command = "tmux capture-pane -p -t \"" + target_pane + "\"";
    std::array<char, 4096> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("Failed to run tmux capture-pane");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}
};

#endif // TMUXCLIENT_HPP