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

extern bool add_user(const std::string& username, const std::string& password);

class DBManager {
public:
    explicit DBManager(const std::string db_file) 
    : db_path(db_file), db(nullptr)
    {
        debug_log("DB: " + db_path);
        if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
            debug_log(std::string("Failed to open DB: ") + sqlite3_errmsg(db));
            db = nullptr;
            throw std::runtime_error("Error al crear el objeto DBManager");
        }
        const char* real_file = sqlite3_db_filename(db, "main");
        debug_log("sqlite3: archivo real abierto: " + std::string(real_file ? real_file : "null"));
    }

    ~DBManager() {
        if (db) {
            sqlite3_close(db);
        }
    }
    void m_initialize_database() {
        // std::string db_path = m_get_db_path();
        debug_log("inicialización db:" + m_get_db_path());

        if (!db) {
            throw std::runtime_error("No se puede inicializar: la base de datos no está abierta");
        }
        debug_log("db: existe");
        if (m_is_schema_initialized()) {
            debug_log("Base de datos ya inicializada.");
            return;
        }
        debug_log("m_is_schema_initialized: existe");
        if (!m_begin_transaction()) {
            throw std::runtime_error("Error al iniciar la transacción de inicialización");
        }

        debug_log("m_begin_transaction: existe");
        std::vector<std::string> init_sql = {
            R"(CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );)",

            R"(CREATE TABLE IF NOT EXISTS tokens (
                token TEXT PRIMARY KEY,
                user_id INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(user_id) REFERENCES users(id)
            );)",

            R"(CREATE TABLE IF NOT EXISTS blocked (
                ip TEXT PRIMARY KEY,
                blocked_until INTEGER NOT NULL
            );)",

            R"(CREATE INDEX IF NOT EXISTS idx_blocked_until ON blocked(blocked_until);)",

            R"(CREATE TABLE IF NOT EXISTS hits (
                ip TEXT NOT NULL,
                timestamp INTEGER NOT NULL
            );)",

            R"(CREATE INDEX IF NOT EXISTS idx_hits_ip_time ON hits(ip, timestamp);)"
        };

        for (const auto& sql : init_sql) {
            if (!m_execute(sql)) {
                m_rollback_transaction();
                throw std::runtime_error("Fallo al ejecutar SQL de inicialización");
            }
        }

        if (!m_commit_transaction()) {
            throw std::runtime_error("Error al confirmar transacción");
        }
        debug_log("inicialización: finaliza");


        if (!add_user("admin", "admin")) debug_log("Error al inicializar usuario admin");

    }

    bool m_is_ip_blocked(const std::string& ip) {
        std::lock_guard<std::mutex> lock(db_mutex);
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT 1 FROM blocked WHERE ip = ? AND blocked_until > ?";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Error al iniciar la transacción de inicialización");
        }

        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, std::time(nullptr));

        bool blocked = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return blocked;
    }

    std::string m_get_db_path() const {
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

    bool m_execute(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(db_mutex);
        sqlite3_stmt* stmt = nullptr;
        debug_log("m_execute: starting \n" + sql);

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            log_sqlite_error("Prepare failed");
            return false;
        }

        if (!bind_all(stmt, params)) {
            log_sqlite_error("bind_all failed");
            sqlite3_finalize(stmt);
            return false;
        }

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        // if (sqlite3_step(stmt) != SQLITE_DONE) {
        //     debug_log("m_execute: Error");

        //     log_sqlite_error(("sqlite3_step falló para SQL: " + sql).c_str());
        // }
        std::string f = success ? "true" : "false";
        debug_log("m_execute: sqlite3_step " + f);
        return success;
    }

    bool m_is_schema_initialized() {
        const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='users';";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Error al verificar la inicialización");
        }

        bool initialized = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        std::string f = initialized ? "true" : "false";
        debug_log("m_is_schema_initialized: initialized " + f);
        return initialized;
    }

    bool m_begin_transaction() {
        return m_execute("BEGIN TRANSACTION;");
    }

    bool m_commit_transaction() {
        return m_execute("COMMIT;");
    }

    bool m_rollback_transaction() {
        return m_execute("ROLLBACK;");
    }

    void log_sqlite_error(const char* context) {
        std::cerr << context << ": " << (db ? sqlite3_errmsg(db) : "no db") << '\n';
    }
};

#endif // DBMANAGER_HPP