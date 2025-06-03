#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <mutex>
#include "debug_log.hpp"

class DBManager {
public:
    explicit DBManager(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            std::cerr << "Failed to open DB: " << sqlite3_errmsg(db) << '\n';
            db = nullptr;
        }
    }

    ~DBManager() {
        if (db) sqlite3_close(db);
    }

    // INSERT / UPDATE / DELETE
    bool execute(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(db_mutex);
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            log_sqlite_error("Prepare failed");
            return false;
        }

        if (!bind_all(stmt, params)) {
            sqlite3_finalize(stmt);
            return false;
        }

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    // SELECT con callback por fila
    bool select(const std::string& sql,
                const std::vector<std::string>& params,
                const std::function<void(sqlite3_stmt*)>& row_callback) {
        std::lock_guard<std::mutex> lock(db_mutex);
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            log_sqlite_error("Prepare failed");
            return false;
        }

        if (!bind_all(stmt, params)) {
            sqlite3_finalize(stmt);
            return false;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            row_callback(stmt);
        }

        sqlite3_finalize(stmt);
        return true;
    }

    // Transacciones
    bool begin_transaction() {
        return execute("BEGIN TRANSACTION;");
    }

    bool commit_transaction() {
        return execute("COMMIT;");
    }

    bool rollback_transaction() {
        return execute("ROLLBACK;");
    }

private:
    sqlite3* db = nullptr;
    std::mutex db_mutex;

    bool bind_all(sqlite3_stmt* stmt, const std::vector<std::string>& params) {
        for (size_t i = 0; i < params.size(); ++i) {
            if (sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
                log_sqlite_error("Bind failed");
                return false;
            }
        }
        return true;
    }

    void log_sqlite_error(const char* context) {
        std::cerr << context << ": " << (db ? sqlite3_errmsg(db) : "no db") << '\n';
    }
};


// #include <sqlite3.h>
// #include <string>
// #include <vector>
// #include <functional>
// #include <mutex>
// #include <iostream>

// // No es cabecera, pero define una clase reutilizable
// class DBManager {
// public:
//     explicit DBManager(const std::string& db_path) {
//         if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
//             std::cerr << "Error al abrir la base de datos: " << sqlite3_errmsg(db) << '\n';
//             db = nullptr;
//         }
//     }

//     ~DBManager() {
//         if (db) sqlite3_close(db);
//     }

//     bool execute(const std::string& sql, const std::vector<std::string>& params = {}) {
//         std::lock_guard<std::mutex> lock(db_mutex);
//         sqlite3_stmt* stmt = nullptr;
//         if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
//             log_sqlite_error("Prepare");
//             return false;
//         }

//         if (!bind_all(stmt, params)) {
//             sqlite3_finalize(stmt);
//             return false;
//         }

//         bool success = (sqlite3_step(stmt) == SQLITE_DONE);
//         sqlite3_finalize(stmt);
//         return success;
//     }

//     bool select(const std::string& sql,
//                 const std::vector<std::string>& params,
//                 const std::function<void(sqlite3_stmt*)>& row_callback) {
//         std::lock_guard<std::mutex> lock(db_mutex);
//         sqlite3_stmt* stmt = nullptr;
//         if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
//             log_sqlite_error("Prepare");
//             return false;
//         }

//         if (!bind_all(stmt, params)) {
//             sqlite3_finalize(stmt);
//             return false;
//         }

//         while (sqlite3_step(stmt) == SQLITE_ROW) {
//             row_callback(stmt);
//         }

//         sqlite3_finalize(stmt);
//         return true;
//     }

//     bool begin_transaction() { return execute("BEGIN TRANSACTION;"); }
//     bool commit_transaction() { return execute("COMMIT;"); }
//     bool rollback_transaction() { return execute("ROLLBACK;"); }

// private:
//     sqlite3* db = nullptr;
//     std::mutex db_mutex;

//     bool bind_all(sqlite3_stmt* stmt, const std::vector<std::string>& params) {
//         for (size_t i = 0; i < params.size(); ++i) {
//             if (sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
//                 log_sqlite_error("Bind");
//                 return false;
//             }
//         }
//         return true;
//     }

//     void log_sqlite_error(const char* context) {
//         std::cerr << context << " error: " << (db ? sqlite3_errmsg(db) : "no db") << '\n';
//     }
// };

// // Exponemos una instancia global segura si se desea
// DBManager* get_db_manager_instance() {
//     static DBManager instance("mi_base.sqlite");
//     return &instance;
// }