#ifndef THREAD_POOL_HPP_PCCS
#define THREAD_POOL_HPP_PCCS

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <iostream> // para debug

// Small-buffer no-heap function container
template <std::size_t Capacity>
class FixedFunction {
    alignas(void(*)(void*)) char buffer[Capacity];
    void (*invoke)(void*) = nullptr;
    void (*destroy)(void*) = nullptr;

public:
    FixedFunction() = default;

    template <typename Callable>
    FixedFunction(Callable&& c) {
        static_assert(sizeof(Callable) <= Capacity, "Callable too large");
        new (buffer) Callable(std::forward<Callable>(c));
        invoke = [](void* b) { (*reinterpret_cast<Callable*>(b))(); };
        destroy = [](void* b) { reinterpret_cast<Callable*>(b)->~Callable(); };
    }

    void operator()() {
        invoke(buffer);
    }

    ~FixedFunction() {
        if (destroy) {
            destroy(buffer);
        }
    }
};

class ThreadPool {
    using Task = FixedFunction<64>;

    std::vector<std::thread> workers;
    std::queue<Task> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition_task;
    std::condition_variable condition_space;

    std::atomic<bool> stop{false};
    std::size_t max_queue_size;

public:
    explicit ThreadPool(std::size_t threads, std::size_t max_queue = 256)
        : max_queue_size(max_queue) {
        for (std::size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this, i] {
                while (true) {
                    Task task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);

                        // Esperar tarea o shutdown, con timeout de 30s para debug
                        if (!condition_task.wait_for(lock, std::chrono::seconds(30), [this] {
                            return stop || !tasks.empty();
                        })) {
                            std::cerr << "[ThreadPool] Worker " << i << " timeout waiting for task\n";
                            continue; // seguir esperando
                        }

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                        condition_space.notify_one(); // Desbloquear productores si hay límite
                    }

                    try {
                        task();
                    } catch (...) {
                        std::cerr << "[ThreadPool] Uncaught exception in task\n";
                    }
                }
            });
        }
    }

    template <typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            // Esperar espacio si la cola está llena
            condition_space.wait(lock, [this] {
                return tasks.size() < max_queue_size || stop;
            });

            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks.emplace(std::forward<F>(f));
        }

        condition_task.notify_one();
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }

        condition_task.notify_all();
        condition_space.notify_all();

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

#endif // THREAD_POOL_HPP_PCCS


// #ifndef THREAD_POOL_HPP_PCCS
// #define THREAD_POOL_HPP_PCCS

// #include <vector>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <atomic>
// #include <stdexcept>
// #include <utility>

// // Small-buffer no-heap function container
// template <std::size_t Capacity>
// class FixedFunction {
//     alignas(void(*)(void*)) char buffer[Capacity];
//     void (*invoke)(void*) = nullptr;
//     void (*destroy)(void*) = nullptr;

// public:
//     FixedFunction() = default;

//     template <typename Callable>
//     FixedFunction(Callable&& c) {
//         static_assert(sizeof(Callable) <= Capacity, "Callable too large");
//         new (buffer) Callable(std::forward<Callable>(c));
//         invoke = [](void* b) { (*reinterpret_cast<Callable*>(b))(); };
//         destroy = [](void* b) { reinterpret_cast<Callable*>(b)->~Callable(); };
//     }

//     void operator()() {
//         invoke(buffer);
//     }

//     ~FixedFunction() {
//         if (destroy) {
//             destroy(buffer);
//         }
//     }
// };

// class ThreadPool {
//     using Task = FixedFunction<64>;

//     std::vector<std::thread> workers;
//     std::queue<Task> tasks;

//     std::mutex queue_mutex;
//     std::condition_variable condition;
//     std::atomic<bool> stop{false};

// public:
//     explicit ThreadPool(std::size_t threads) {
//         for (std::size_t i = 0; i < threads; ++i) {
//             workers.emplace_back([this] {
//                 while (true) {
//                     Task task;

//                     {
//                         std::unique_lock<std::mutex> lock(queue_mutex);
//                         condition.wait(lock, [this] {
//                             return stop.load(std::memory_order_acquire) || !tasks.empty();
//                         });

//                         if (stop.load(std::memory_order_acquire) && tasks.empty()) {
//                             return;
//                         }

//                         task = std::move(tasks.front());
//                         tasks.pop();
//                     }

//                     task();
//                 }
//             });
//         }
//     }

//     template <typename F>
//     void enqueue(F&& f) {
//         if (stop.load(std::memory_order_acquire)) {
//             throw std::runtime_error("enqueue on stopped ThreadPool");
//         }

//         {
//             std::lock_guard<std::mutex> lock(queue_mutex);
//             tasks.emplace(std::forward<F>(f));
//         }

//         condition.notify_one();
//     }

//     ~ThreadPool() {
//         {
//             std::lock_guard<std::mutex> lock(queue_mutex);
//             stop.store(true, std::memory_order_release);
//         }

//         condition.notify_all();

//         for (auto& worker : workers) {
//             if (worker.joinable()) {
//                 worker.join();
//             }
//         }
//     }
// };

// #endif // THREAD_POOL_HPP_PCCS

// #ifndef THREAD_POOL_HPP_PCCS
// #define THREAD_POOL_HPP_PCCS

// #include <vector>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <functional>
// #include <atomic>
// #include <stdexcept>

// class ThreadPool {
//     std::vector<std::thread> workers;
//     std::queue<std::function<void()>> tasks;

//     std::mutex queue_mutex;
//     std::condition_variable condition;
//     std::atomic<bool> stop{false};

// public:
//     explicit ThreadPool(std::size_t threads) {
//         for (std::size_t i = 0; i < threads; ++i) {
//             workers.emplace_back([this] {
//                 while (true) {
//                     std::function<void()> task;

//                     {
//                         std::unique_lock<std::mutex> lock(this->queue_mutex);
//                         this->condition.wait(lock, [this] {
//                             return this->stop || !this->tasks.empty();
//                         });

//                         if (this->stop && this->tasks.empty()) {
//                             return;
//                         }

//                         task = std::move(this->tasks.front());
//                         this->tasks.pop();
//                     }

//                     task();
//                 }
//             });
//         }
//     }

//     void enqueue(std::function<void()> task) {
//         {
//             std::lock_guard<std::mutex> lock(queue_mutex);
//             if (stop) {
//                 throw std::runtime_error("enqueue on stopped ThreadPool");
//             }
//             tasks.emplace(std::move(task));
//         }
//         condition.notify_one();
//     }

//     ~ThreadPool() {
//         stop = true;
//         condition.notify_all();
//         for (auto& worker : workers) {
//             if (worker.joinable()) {
//                 worker.join();
//             }
//         }
//     }
// };

// #endif // THREAD_POOL_HPP