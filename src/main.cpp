#include <netinet/in.h>      // struct sockaddr_in, INADDR_ANY, htons(), ntohs(), IPPROTO_UDP, etc.
#include <netinet/tcp.h>     // TCP_NODELAY, struct tcphdr, opciones específicas del protocolo TCP
#include <sys/socket.h>      // socket(), bind(), listen(), accept(), connect(), shutdown(), SHUT_RDWR, struct sockaddr
#include <arpa/inet.h>       // inet_ntoa(), inet_pton(), inet_ntop(), ntohl(), htonl(), etc. (conversiones de IP y puertos)
#include <unistd.h>          // close(), read(), write(), access(), STDIN_FILENO, etc. (llamadas al sistema POSIX)
#include <cstring>           // std::memcpy(), std::memset(), std::strcmp(), std::strlen(), etc.
#include <iostream>          // std::cout, std::cerr, std::endl
// #include <fstream>           // std::ifstream, std::ofstream, std::fstream (manipulación de archivos)
#include <string>            // std::string, std::to_string(), std::getline(), etc.
#include <thread>            // std::thread, std::this_thread::sleep_for(), concurrencia básica
// #include <functional>        // std::function, std::bind, std::ref (para funciones y callbacks)
#include <cstdlib>           // std::atoi(), std::exit(), std::rand(), std::getenv(), std::system(), etc.
#include "thread_pool.hpp"
#include "db_manager.hpp"
#include "utils.hpp"

// ========================
// Configuración global
// ========================
static bool SHOW_HELP = false;
constexpr int MAX_SELECT_FAILURES = 100;
constexpr int ACCEPT_TIMEOUT_SEC = 5;

// ========================
// Declaraciones externas
// ========================
extern void handle_client(int client_sock);
extern bool is_ip_blocked(const std::string& ip);

// ========================
// Función de cierre seguro
// ========================
inline void fatal_error(const std::string& msg, const int& exit_code = 1) {
    if (log_file.is_open()) {
        log_file << "FATAL: " << msg << std::endl;
        log_file.close();
    }
    debug_log("ERROR FATAL: " + msg);
    std::exit(exit_code);
}

// ========================
// Evitar ejecución con root.
// ========================
inline void checkNotRoot() {
    if (geteuid() == 0) {
        fatal_error("Por seguridad este programa no debe ejecutarse como usuario privilegiado root.\n", EXIT_FAILURE);
    }
}

void print_help(const char* prog_name) {
    std::cout <<
        "\n== SERVIDOR - MODO DE USO ==\n\n"
        "  " << prog_name << " [opciones]\n\n"
        "Opciones:\n"
        "  -help                   Muestra esta ayuda y termina.\n"
        "  -logfile                Activa el modo de log a archivo (default: false).\n"
        "  -prod                   Activa el modo producción (default: dev_mode = true).\n"
        "  -nodebug                Desactiva la salida de depuración (default: debug_mode = true).\n"
        "  -port=[NUMERO]          Asigna el puerto de escucha (default: 8080).\n"
        "  -threads=[CANTIDAD]     Asigna el número máximo de hilos (default: 4).\n\n"
        "  -config=[PATH]          Asigna el nombre del archivo de configuración (default:server.config).\n\n"
        "  -loginexp=[SEGUNDOS]    Tiempo de expiración de sesión (default: 3600, min: 1800, max: 172800).\n"
        "  -dbname=[NOMBRE]        Nombre del archivo de base de datos (default: atlantis.db).\n"
        "  -logfile=[NOMBRE]       Ruta del archivo de log (default: atlantis.log).\n"
        "  -servers=[RUTA]         Ruta base para servidores (default: juegos).\n"
        "  -maxreqbuf=[BYTES]      Tamaño máximo del buffer de lectura TCP (default: 8192).\n"
        "Ejemplos:\n"
        "  " << prog_name << "\n"
        "  " << prog_name << " -logfile -prod -port=8080 -threads=2\n\n"
        "Configuración persistente:\n"
        "  Para que las opciones se mantengan entre ejecuciones, modifica el archivo:\n"
        "    • En modo producción : $HOME/.web-atlantis/server.config\n"
        "    • En modo desarrollo : [directorio del proyecto]/server.config\n"
        "  (La ubicación depende del valor de DEV_MODE)\n"
        << std::endl;

    std::exit(0);
}

static void parse_arguments(int argc, char* argv[], GlobalConfig& config) {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "-help") == 0) [[unlikely]] {
            SHOW_HELP = true;
        } else if (std::strcmp(arg, "-logfile") == 0) [[unlikely]] {
            config.log_mode = true;
        } else if (std::strcmp(arg, "-prod") == 0) [[likely]] {
            config.dev_mode = false;
        } else if (std::strcmp(arg, "-nodebug") == 0) [[unlikely]] {
            config.debug_mode = false;
        } else if (std::strncmp(arg, "-port=", 6) == 0) [[likely]] {
            int vport = std::atoi(arg + 6);

            if (vport >= 1024 && vport <= 65535) [[likely]] {
                config.port = vport;
            } else [[unlikely]] {
                debug_log("PORT inválido. Usando: " + std::to_string(config.port));
            }

        } else if (std::strncmp(arg, "-threads=", 9) == 0) [[unlikely]] {
            config.max_threads = std::atoi(arg + 9);

        } else if (std::strncmp(arg, "-config=", 8) == 0) [[unlikely]] {
            int written = std::snprintf(config.config_file, sizeof(config.config_file), "%s", arg + 8);

            if (written < 0) [[unlikely]] {
                std::cerr << "Error: snprintf failed.\n";
                std::snprintf(config.config_file, sizeof(config.config_file), "%s", "server.config");
            } else if (static_cast<size_t>(written) >= sizeof(config.config_file)) [[likely]] {
                debug_log("WARN: Archivo configuración truncado a  " + std::to_string(sizeof(config.config_file) - 1) + " characters.\n");
            }
        } else if (std::strncmp(arg, "-loginexp=", 10) == 0) {
            int value = std::atoi(arg + 10);
            if (value >= 1800 && value <= 172800) {
                config.login_expiration = value;
            } else {
                debug_log("LOGIN_EXPIRATION inválido. Usando: " + std::to_string(config.login_expiration));
            }

        } else if (std::strncmp(arg, "-dbname=", 8) == 0) {
            set_path(config.db_name, sizeof(config.db_name), arg + 8, "DB_NAME");

        } else if (std::strncmp(arg, "-logfile=", 9) == 0) {
            set_path(config.log_file, sizeof(config.log_file), arg + 9, "LOG_FILE");

        } else if (std::strncmp(arg, "-servers=", 9) == 0) {
            set_path(config.servers_path, sizeof(config.servers_path), arg + 9, "SERVERS_PATH");

        } else if (std::strncmp(arg, "-maxreqbuf=", 11) == 0) {
            int value = std::atoi(arg + 11);
            if (value >= 1024 && value <= 65536) { // Rango razonable
                config.max_req_buf_size = value;
            } else {
                debug_log("MAX_REQ_BUF_SIZE inválido. Usando: " + std::to_string(config.max_req_buf_size));
            }
        } else {
            std::cerr << "Advertencia: opción desconocida ignorada: " << arg << "\n";
            print_help(argv[0]);
        }
    }

    if (SHOW_HELP) {
        print_help(argv[0]);
    }
}

void try_enqueue_client(int client_sock, ThreadPool& pool) {
    try {
        //////////////////////////////////////////////////
        // TODO: por alguna razón desconocida, pasar ip al 
        //       pool ocasiona una violación de segmento.
        /////////////////////////////////////////////////

        // pool.enqueue([client_sock,ip]() mutable {
        pool.enqueue([client_sock]() mutable {
            try {
                handle_client(client_sock);
            } catch (const std::runtime_error& e) {
                debug_log(e.what());
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
            } catch (...) {
                debug_log("Excepción desconocida al manejar cliente.");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
            }
        });
    } catch (const std::runtime_error& e) {
        debug_log(std::string("Rechazada conexión: Pool saturado...:") + e.what());
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
    }
}

// ========================
// Función principal
// ========================
int main(int argc, char* argv[]) {

    checkNotRoot();

    try {

        load_or_create_config(config);

        parse_arguments(argc, argv, config);

        DBManager db_manger(get_db_file());

        if (config.log_mode) [[likely]] {
            log_file.open(get_log_file(), std::ios::app);
            if (!log_file) [[unlikely]] {
                fatal_error("No se pudo abrir el archivo de log.");
            }
        }

        db_manger.m_initialize_database();

        // ========================
        // Crear socket servidor
        // ========================
        int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (server_fd < 0) [[unlikely]] {
            fatal_error("Error creando el socket.");
        }
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) [[unlikely]] {
            close(server_fd);
            fatal_error("Error en setsockopt.");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config.port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) [[unlikely]] {
            close(server_fd);
            fatal_error("Error en bind.");
        }

        if (listen(server_fd, 128) < 0) [[unlikely]] {
            close(server_fd);
            fatal_error("Error en listen.");
        }

        // debug_log("\n\n MODO DEV: " + std::to_string(config.dev_mode));
        debug_log("\n\n MODO: " + std::string(config.dev_mode ? "DEV" : "PROD"));
        debug_log("\n\nServidor escuchando en el puerto " + std::to_string(config.port));


        // ========================
        // Crear ThreadPool
        // ========================
        std::size_t num_threads = config.max_threads ? config.max_threads : std::thread::hardware_concurrency();
        if (num_threads == 0) [[unlikely]] {
            num_threads = 2;
        }

        ThreadPool myThreadPool(num_threads);
        
        // Loop principal
        int consecutive_select_failures = 0;
                

        while (true) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);

            struct timeval timeout;
            timeout.tv_sec = ACCEPT_TIMEOUT_SEC;
            timeout.tv_usec = 0;

            int activity = select(server_fd + 1, &readfds, nullptr, nullptr, &timeout);

            if (activity < 0) {
                if (errno == EINTR) {
                    debug_log("Error: " + std::string(strerror(errno)) + " | " + std::to_string(EINTR));
                    continue; // Señal, reiniciar loop
                }

                debug_log("Fallo select(): " + std::string(strerror(errno)));
                consecutive_select_failures++;

                if (consecutive_select_failures >= MAX_SELECT_FAILURES) {
                    fatal_error("Demasiados fallos consecutivos de select(). Terminando servidor.");
                    break;
                }
                continue;
            }
            

            if (activity == 0) {
                // Timeout sin actividad. Puede usarse para hacer tareas periódicas.
                continue;
            }
            
            if (FD_ISSET(server_fd, &readfds)) {
                sockaddr_in client_addr{};
                socklen_t addrlen = sizeof(client_addr);
                int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

                if (client_sock < 0) {
                    debug_log("Fallo al aceptar conexión: " + std::string(strerror(errno)));
                    consecutive_select_failures++;
                    if (consecutive_select_failures >= MAX_SELECT_FAILURES) {
                        fatal_error("Demasiados fallos consecutivos de select(). Terminando servidor.");
                        break;
                    }
                    continue;
                }

                // Resetear contador si no hubo fallo
                consecutive_select_failures = 0;

                // Opciones de socket
                int opt = 1;
                setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
                setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
                setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
                
                // Obtener IP del cliente
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
                std::string client_ip = ip_str; // Esto estará ocacionando la violación de segmento?

                if (db_manger.m_is_ip_blocked(client_ip)) [[unlikely]] {
                    debug_log("Ip baneado:" + client_ip);
                    // shutdown(client_sock, SHUT_RDWR);
                    close(client_sock);
                    continue;
                }
                debug_log("IP CLIEnTE: " + client_ip);

                ///////////////////////////////////////////////////
                // El tamaño máximo del lambda con ip es 40 bytes, así que eso no es
                // lo que ocaciona la violación de segmento.
                //////////////////////////////////////////////////
                // auto lambda = [client_sock]() mutable {};
                // std::cout << "Tamaño del lambda: " << sizeof(decltype(lambda)) << " bytes\n";  

                // Encolar en el ThreadPool
                try_enqueue_client(client_sock, myThreadPool);
            }
        }

        debug_log("Cerrando servidor...");
        close(server_fd);
        debug_log("Terminado...");
        if (log_file.is_open()) {
            log_file.close();
        }
        return 0;
    } catch (const std::runtime_error& e) {
        fatal_error(std::string("ERROR fatal: ") + e.what());
    } catch (const std::exception& e) {
        fatal_error(std::string("Excepción fatal: ") + e.what());
    } catch (...) {
        fatal_error("Excepción fatal desconocida.");
    }
    std::cerr << "Cierre anormal...";
}

__attribute__((section(".rodata.pccssystems.version"), used))
const char app_version[] = "0.3.1";
__attribute__((section(".rodata.pccssystems.author"), used))
const char app_author[] = "Strapicarus";