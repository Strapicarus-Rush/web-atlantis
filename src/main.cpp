#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <map>
#include <cstdlib>
#include "debug_log.hpp"

// ========================
// Configuración global
// ========================
inline int PORT = 8080;
bool DEV_MODE = true;
bool DEBUG_MODE = true;
bool LOG_MODE = false;
std::ofstream log_file;
int MAX_THREADS = 2;

// ========================
// Declaraciones externas
// ========================
extern std::string get_base_path();
extern void load_or_create_config();
extern std::string get_log_path();
extern bool initialize_database();
extern void handle_client(int& client_sock);

// ========================
// ThreadPool
// ========================
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};

public:
    ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i){
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });

                        if (this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (auto& worker : workers)
            if (worker.joinable())
                worker.join();
    }
};

// ========================
// Función de cierre seguro
// ========================
void fatal_error(const std::string& msg, int exit_code = 1) {
    debug_log(msg);
    if (log_file.is_open()) {
        log_file << "FATAL: " << msg << std::endl;
        log_file.close();
    }
    debug_log("ERROR FATAL");
    std::exit(exit_code);
}

void checkNotRoot() {
    if (geteuid() == 0) {
        fatal_error("Este programa no puede ejecutarse como root.\n");
        std::exit(EXIT_FAILURE);
    }
}

// ========================
// Función principal
// ========================
int main(int argc, char* argv[]) {
    checkNotRoot();
    // Procesamiento de argumentos
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-logfile") == 0) LOG_MODE = true;
        if (std::strcmp(argv[i], "-help") == 0) LOG_MODE = true;
        if (std::strcmp(argv[i], "-prod") == 0) DEV_MODE = false;
        if (std::strcmp(argv[i], "-nodebug") == 0) DEBUG_MODE = false;

    }
    // int client_sock;
    try {
        load_or_create_config();

        if (LOG_MODE) {
            log_file.open(get_log_path(), std::ios::app);
            if (!log_file){
                debug_log("No se pudo abrir el archivo de log.");
                fatal_error("No se pudo abrir el archivo de log.");
            }
        }

        if (!initialize_database()){
            debug_log("Error inicializando la base de datos.");
            fatal_error("Error inicializando la base de datos.");
        }
        // Crear socket servidor
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0){
            debug_log("Error crear socket");
            fatal_error("Error creando el socket.");
        }
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            debug_log("Error setsockopt");
            close(server_fd);
            fatal_error("Error en setsockopt.");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            debug_log("Error Bind");
            close(server_fd);
            fatal_error("Error en bind.");
        }

        if (listen(server_fd, 128) < 0) {
            debug_log("Error Listen");
            close(server_fd);
            fatal_error("Error en listen.");
        }

        debug_log("Servidor escuchando en el puerto " + std::to_string(PORT));

        // Crear ThreadPool
        std::size_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2;

        ThreadPool pool(num_threads);
        
        // Loop principal
        while (true) {
            sockaddr_in client_addr{};
            socklen_t addrlen = sizeof(client_addr);
            int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

            if (client_sock < 0) {
                debug_log("Fallo al aceptar conexión.");
                continue;
            }

            pool.enqueue([client_sock]() mutable {
                try {
                    handle_client(client_sock);
                } catch (const std::exception& e) {
                    std::cerr << "Excepción al manejar cliente: " << e.what() << "\n";
                } catch (...) {
                    std::cerr << "Excepción desconocida al manejar cliente.\n";
                }
                debug_log("Cerrando conexión cliente");
                std::cerr << "Cerrando cliente.\n";
                close(client_sock); // Siempre cerrar
            });
        }
        debug_log("Cerrando servidor");
        close(server_fd);
        if (log_file.is_open()) log_file.close();
        debug_log("Terminado...");
        return 0;

    } catch (const std::exception& e) {
        fatal_error(std::string("Excepción fatal: ") + e.what());
    } catch (...) {
        fatal_error("Excepción fatal desconocida.");
    }
}