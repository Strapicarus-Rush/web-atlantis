#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <functional>
#include <cstdlib>
#include "thread_pool.hpp"
#include "db_manager.hpp"
#include "utils.hpp"

// ========================
// Configuración global
// ========================
static bool SHOW_HELP = false;
// static std::string db_path = get_db_path();
DBManager db_manger(get_db_path());

// ========================
// Declaraciones externas
// ========================

// extern bool initialize_database();
extern void handle_client(int& client_sock);
extern bool is_ip_blocked(const std::string& ip);
extern void setup_routes();

// ========================
// Función de cierre seguro
// ========================
inline void fatal_error(const std::string& msg, const int& exit_code = 1) {
    debug_log(msg);
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
        // std::exit(EXIT_FAILURE);
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
        } 

        else if (std::strncmp(arg, "-port=", 6) == 0) [[likely]] {
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
                std::cerr << "Warning: config_file truncated to " << sizeof(config.config_file) - 1 << " characters.\n";
            }
        }

        else {
            std::cerr << "Advertencia: opción desconocida ignorada: " << arg << "\n";
            print_help(argv[0]);
        }
    }

    if (SHOW_HELP) {
        print_help(argv[0]);
    }
}

bool try_enqueue_client(int client_sock, ThreadPool& pool) {
    try {
        pool.enqueue([client_sock]() mutable {
            try {
                handle_client(client_sock);
            } catch (const std::exception& e) {
                debug_log(std::string("Excepción al manejar cliente: ") + e.what());
            } catch (...) {
                debug_log("Excepción desconocida al manejar cliente.");
            }
            debug_log("Cerrando conexión cliente");
            shutdown(client_sock, SHUT_RDWR);
            close(client_sock);
        });
        return true;
    } catch (const std::runtime_error& e) {
        debug_log("Rechazada conexión: Pool saturado");
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
        return false;
    }
}

// ========================
// Función principal
// ========================
int main(int argc, char* argv[]) {

    checkNotRoot();
    parse_arguments(argc, argv, config);

    try {
        // ========================
        // TODO: Revisar inicialización para limpiar.
        // ========================
        load_or_create_config(config);
        setup_routes();

        if (config.log_mode) [[unlikely]] {
            log_file.open(get_log_path(), std::ios::app);
            if (!log_file) [[unlikely]] {
                debug_log("No se pudo abrir el archivo de log.");
                fatal_error("No se pudo abrir el archivo de log.");
            }
        }

        if (!db_manger.m_initialize_database()) [[unlikely]] {
            debug_log("Error inicializando la base de datos.");
            fatal_error("Error inicializando la base de datos.");
        }

        // ========================
        // Crear socket servidor
        // ========================
        int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (server_fd < 0) [[unlikely]] {
            debug_log("Error crear socket");
            fatal_error("Error creando el socket.");
        }
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) [[unlikely]] {
            debug_log("Error setsockopt");
            close(server_fd);
            fatal_error("Error en setsockopt.");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config.port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) [[unlikely]] {
            debug_log("Error Bind");
            close(server_fd);
            fatal_error("Error en bind.");
        }

        if (listen(server_fd, 128) < 0) [[unlikely]] {
            debug_log("Error Listen");
            close(server_fd);
            fatal_error("Error en listen.");
        }

        debug_log("Servidor escuchando en el puerto " + std::to_string(config.port));

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
        constexpr int MAX_SELECT_FAILURES = 10;
        constexpr int ACCEPT_TIMEOUT_SEC = 5;
        

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
                    continue; // Señal, reiniciar loop
                }

                debug_log("Fallo select(): " + std::string(strerror(errno)));
                consecutive_select_failures++;

                if (consecutive_select_failures >= MAX_SELECT_FAILURES) {
                    debug_log("Demasiados fallos consecutivos de select(). Terminando servidor.");
                    break;
                }

                continue;
            }

            // Resetear contador si no hubo fallo
            consecutive_select_failures = 0;

            if (activity == 0) {
                // Timeout sin actividad. Puede usarse para hacer tareas periódicas o verificar flags.
                continue;
            }

            if (FD_ISSET(server_fd, &readfds)) {
                sockaddr_in client_addr{};
                socklen_t addrlen = sizeof(client_addr);
                int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

                if (client_sock < 0) {
                    debug_log("Fallo al aceptar conexión: " + std::string(strerror(errno)));
                    continue;
                }

                // Opciones de socket
                int opt = 1;
                setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
                setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
                setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
                
                // Obtener IP del cliente
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
                std::string client_ip = ip_str;

                if (db_manger.m_is_ip_blocked(client_ip)) [[unlikely]] {
                    shutdown(client_sock, SHUT_RDWR);
                    close(client_sock);
                    continue;
                }

                // Encolar en el ThreadPool
                if (!try_enqueue_client(client_sock, myThreadPool)) {
                    continue;
                }
            }
        }
        // while (true) {
        //     sockaddr_in client_addr{};
        //     socklen_t addrlen = sizeof(client_addr);
        //     int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

        //     if (client_sock < 0) [[unlikely]] {
        //         debug_log("Fallo al aceptar conexión.");
        //         continue;
        //     }
            
        //     // ========================
        //     // protección básica al socket
        //     // ========================
        //     int opt = 1;
        //     setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        //     setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

        //     // ========================
        //     // verificar ip cliente
        //     // ========================
        //     char ip_str[INET_ADDRSTRLEN];
        //     inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        //     std::string client_ip = ip_str;

        //     if (db_manger.m_is_ip_blocked(client_ip)) [[unlikely]] {// DROP inmediato
        //         // ==========================================================================
        //         // Se puede NO usar shutdown y close para dejar al cliente bloqueado y en espera
        //         // Pero esto es una arma de doble filo ya que ocupa recursos internos y al
        //         // no cerrar el socket la tabla FD crece.
        //         // ==========================================================================
        //         shutdown(client_sock, SHUT_RDWR);
        //         close(client_sock);
        //         continue;
        //     }

        //     if (!try_enqueue_client(client_sock, myThreadPool)) {
        //         continue;
        //     }

        //     // myThreadPool.enqueue([client_sock]() mutable {
        //     //     try {
        //     //         handle_client(client_sock);
        //     //     } catch (const std::exception& e) {
        //     //         debug_log(std::string("Excepción al manejar cliente: ") + e.what());
        //     //     } catch (...) {
        //     //         debug_log("Excepción desconocida al manejar cliente.");
        //     //     }
        //     //     debug_log("Cerrando conexión cliente");
        //     //     shutdown(client_sock, SHUT_RDWR);
        //     //     close(client_sock); // Siempre cerrar
        //     // });
        // }

        debug_log("Cerrando servidor...");
        close(server_fd);
        debug_log("Terminado...");
        if (log_file.is_open()) log_file.close();
        return 0;

    } catch (const std::exception& e) {
        fatal_error(std::string("Excepción fatal: ") + e.what());
    } catch (...) {
        fatal_error("Excepción fatal desconocida.");
    }
    debug_log("Cierre anormal...");
}