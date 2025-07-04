#ifndef DBMANAGER_HPP
#define DBMANAGER_HPP

#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <mutex>
#include <ctime>
#include <filesystem>

#include "utils.hpp"

// #define DB_PATH "main.db"

class DBManager {
public:
    explicit DBManager(const std::string& db_path) 
    : db_path(db_path), db(nullptr)
    {
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            debug_log(std::string("Failed to open DB: ") + sqlite3_errmsg(db));
            db = nullptr;
        }
    }

    ~DBManager() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool m_execute(const std::string& sql, const std::vector<std::string>& params = {}) {
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

    // bool m_select(const std::string& sql,
    //             const std::vector<std::string>& params,
    //             const std::function<void(sqlite3_stmt*)>& row_callback) {
    //     std::lock_guard<std::mutex> lock(db_mutex);
    //     sqlite3_stmt* stmt = nullptr;

    //     if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    //         log_sqlite_error("Prepare failed");
    //         return false;
    //     }

    //     if (!bind_all(stmt, params)) {
    //         sqlite3_finalize(stmt);
    //         return false;
    //     }

    //     while (sqlite3_step(stmt) == SQLITE_ROW) {
    //         row_callback(stmt);
    //     }

    //     sqlite3_finalize(stmt);
    //     return true;
    // }

    bool m_begin_transaction() {
        return m_execute("BEGIN TRANSACTION;");
    }

    bool m_commit_transaction() {
        return m_execute("COMMIT;");
    }

    bool m_rollback_transaction() {
        return m_execute("ROLLBACK;");
    }

    bool m_is_ip_blocked(const std::string& ip) {
        std::lock_guard<std::mutex> lock(db_mutex);
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT 1 FROM blocked WHERE ip = ? AND blocked_until > ?";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, std::time(nullptr));

        bool blocked = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return blocked;
    }

    bool m_initialize_database() {
        std::string db_path = get_db_path();
        if (std::filesystem::exists(db_path)) {
            return true;
        }

        if (!m_begin_transaction()) {
            debug_log("Error al iniciar la transacción de inicialización");
            return false;
        }

        std::vector<std::string> init_sql = {
            R"(CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            ))",

            R"(CREATE TABLE IF NOT EXISTS tokens (
                token TEXT PRIMARY KEY,
                user_id INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(user_id) REFERENCES users(id)
            ))",

            R"(CREATE TABLE IF NOT EXISTS blocked (
                ip TEXT PRIMARY KEY,
                blocked_until INTEGER NOT NULL
            ))",

            R"(CREATE INDEX IF NOT EXISTS idx_blocked_until ON blocked(blocked_until))",

            R"(CREATE TABLE IF NOT EXISTS hits (
                ip TEXT NOT NULL,
                timestamp INTEGER NOT NULL
            ))",

            R"(CREATE INDEX IF NOT EXISTS idx_hits_ip_time ON hits(ip, timestamp))"
        };

        for (const auto& sql : init_sql) {
            if (!m_execute(sql)) {
                debug_log("Fallo al ejecutar SQL de inicialización");
                m_rollback_transaction();
                return false;
            }
        }

        if (!m_commit_transaction()) {
            debug_log("Error al confirmar transacción");
            return false;
        }

        return add_user("admin", "admin");
    }

    bool add_user(const std::string& username, const std::string& password) {
        return m_execute("INSERT OR IGNORE INTO users (username, password) VALUES (?, ?)", {username, password});
    }

    std::string get_db_path() const {
        return db_path;
    }

private:
    std::string db_path;
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

#endif // DBMANAGER_HPP