#include <iostream>
#include <random>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <sqlite3.h>
#include <iomanip>
#include <vector>
#include <regex>
#include <ctime>
#include "debug_log.hpp"

inline std::string DB_PATH = "users.db";
inline int TOKEN_EXPIRY = 3600;

const std::string sing = "gsdSDGSD_:,$512512_12534-.,45@#)";

// class DBManager;
// DBManager* get_db_manager_instance();

uint32_t simple_hash(const std::string& str) {
    uint32_t h = 0x811C9DC5;
    for (char c : str) {
        h ^= (uint8_t)c;
        h *= 0x01000193;
    }
    return h;
}

std::vector<uint8_t> runtime_transform(const std::string& base) {
    uint32_t seed = simple_hash(base);
    std::vector<uint8_t> transformed(base.begin(), base.end());

    for (size_t i = 0; i < transformed.size(); ++i) {
        transformed[i] ^= (uint8_t)((seed >> (8 * (i % 4))) + i * 31);
        transformed[i] = (uint8_t)(((transformed[i] << 3) | (transformed[i] >> 5)) & 0xFF);
    }

    std::reverse(transformed.begin(), transformed.end());

    for (size_t i = 0; i < transformed.size(); ++i) {
        transformed[i] ^= (uint8_t)(0x5A + (i * 13) % 97);
    }

    return transformed;
}

void secure_zero(void* data, size_t len) {
    volatile uint8_t* ptr = reinterpret_cast<volatile uint8_t*>(data);
    while (len--) *ptr++ = 0;
}

std::string get_secret_key() {
    std::vector<uint8_t> raw = runtime_transform(sing);

    std::string key(raw.begin(), raw.end());
    secure_zero(raw.data(), raw.size());

    return key;
}

extern bool DEV_MODE;
extern void send_response(int& client_sock, const std::string& status, const std::vector<char>& body, const std::string& content_type, const std::string& extra_headers = "");
extern void send_error(int& client_sock, int code);
extern void close_client(int& client_sock);

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string s;
    while (std::getline(iss, s, delim))
        parts.push_back(s);
    return parts;
}

std::string generate_salt(std::size_t length = 16) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<unsigned char> dis(0, 255);

    std::ostringstream oss;
    for (std::size_t i = 0; i < length; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dis(gen));
    return oss.str();
}

std::string sha256(const std::string& input) {
    std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 computation failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

    return oss.str();
}

std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    if (!HMAC(EVP_sha256(),
              key.data(), key.size(),
              reinterpret_cast<const unsigned char*>(data.data()), data.size(),
              result, &len)) {
        throw std::runtime_error("HMAC failed");
    }

    std::ostringstream oss;
    for (unsigned int i = 0; i < 32; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(result[i]);

    return oss.str();
}

std::string hash_password(const std::string& password) {
    std::string salt = generate_salt();
    std::string hashed = sha256(salt + password);
    return salt + ":" + hashed;
}

bool verify_password(const std::string& password, const std::string& stored) {
    auto sep = stored.find(':');
    if (sep == std::string::npos) return false;
    std::string salt = stored.substr(0, sep);
    std::string hash = stored.substr(sep + 1);
    return sha256(salt + password) == hash;
}

std::string generate_token(int& user_id) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK){
        throw std::runtime_error("Failed to open database");
    }
    const char* get_user_sql = "SELECT username FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, get_user_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare statement for username");
    }

    sqlite3_bind_int(stmt, 1, user_id);
    std::string username;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        username = reinterpret_cast<const char*>(name);
    } else {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("User ID not found");
    }
    sqlite3_finalize(stmt);

    time_t now = time(nullptr);
    std::string data = username + ":" + std::to_string(now);
    std::string signature = hmac_sha256(get_secret_key(), data);
    std::string token = data + ":" + signature;

    const char* insert_sql = "INSERT INTO tokens (token, user_id, created_at) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare token insert");
    }

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("Failed to insert token");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    debug_log("Token generado: " + token);

    return token;
}

bool validate_token(const std::string& token) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;
    debug_log("Validating TOKEN");
    try {
        if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK)
            throw std::runtime_error("Error abriendo base de datos");

        const char* sql = R"(
            SELECT u.username, t.created_at, t.user_id
            FROM tokens t
            JOIN users u ON t.user_id = u.id
            WHERE t.token = ?
        )";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error("Error preparando consulta SQL");

        if (sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)
            throw std::runtime_error("Error vinculando token");

        bool valid = false;

        int step_result = sqlite3_step(stmt);
        if (step_result == SQLITE_ROW) {
            const char* username_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            time_t created_at = static_cast<time_t>(sqlite3_column_int64(stmt, 1));
            std::string username = username_cstr ? username_cstr : "";

            auto parts = split(token, ':');
            if (parts.size() == 3) {
                std::string data = username + ":" + std::to_string(created_at);
                std::string expected_sig = hmac_sha256(get_secret_key(), data);

                if (parts[2] == expected_sig) {
                    if (time(nullptr) <= created_at + TOKEN_EXPIRY) {
                        valid = true;
                        debug_log("Valid TOKEN");
                    } else {
                        debug_log("Token expirado");
                    }
                } else {
                    debug_log("Firma inválida");
                }
            } else {
                debug_log("Formato del token inválido: se esperaban 3 partes, se recibieron " + std::to_string(parts.size()));
            }
        } else if (step_result == SQLITE_DONE) {
            debug_log("Token no encontrado en base de datos");
        } else {
            throw std::runtime_error("sqlite3_step() error: " + std::string(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return valid;
    }
    catch (const std::exception& e) {
        debug_log(std::string("Excepción en validate_token: ") + e.what());
        if (stmt) sqlite3_finalize(stmt);
        if (db) sqlite3_close(db);
        return false;
    }
}

bool validate_user(const std::string& user, const std::string& pass, int& out_user_id) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* query = "SELECT id, password FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    bool ok = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* stored = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (verify_password(pass, stored)) {
            out_user_id = id;
            ok = true;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}
// --------------------- /login ---------------------
void handle_login(int& client_sock, const std::string& body) {
    std::smatch match;
    std::regex user_pass("\"user\"\\s*:\\s*\"(.*?)\".*?\"pass\"\\s*:\\s*\"(.*?)\"");

    if (std::regex_search(body, match, user_pass)) {
        std::string user = match[1];
        std::string pass = match[2];
        int out_user_id;
        if (validate_user(user, pass, out_user_id)) {
            std::string token = generate_token(out_user_id);
            std::string json = "{\"login\":\"ok\"}";
            std::vector<char> body(json.begin(), json.end());
            std::string cookie_header =
                "Set-Cookie: Authorization=" + token +
                "; Path=/; Max-Age=3600; HttpOnly; SameSite=Strict";
            if (!DEV_MODE) {
                cookie_header += "; Secure";
            }
            cookie_header += "\r\n";
            send_response(client_sock, "200 OK", body, "application/json", cookie_header);
        } else {
            send_error(client_sock, 403);
        }
    } else {
        send_error(client_sock, 400);
    }

    close_client(client_sock);
}

bool update_user_password(const std::string& username, const std::string& password){
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* sql = "UPDATE users SET password = ? WHERE username = ?;";
    std::string hashed_pass = hash_password(password);
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        debug_log("User not created...");
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_pass.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}

// --------------------- /edit_user ---------------------
void handle_edit_user(int& client_sock, const std::string& request, const std::string& body) {
    std::smatch match;
    std::regex token_rx("Authorization:\\s*(.*?)\r\n");

    if (!std::regex_search(request, match, token_rx)) {
        send_error(client_sock, 403);
        close_client(client_sock);
        return;
    }

    std::string token = match[1];
    if (!validate_token(token)) {
        send_error(client_sock, 403);
        close_client(client_sock);
        return;
    }

    std::regex user_rx("\"user\"\\s*:\\s*\"(.*?)\"");
    std::regex pass_rx("\"pass\"\\s*:\\s*\"(.*?)\"");
    std::regex new_pass_rx("\"new_pass\"\\s*:\\s*\"(.*?)\"");

    if (std::regex_search(body, match, user_rx)) {
        std::string user = match[1];
        std::string new_pass = std::regex_search(body, match, new_pass_rx) ? std::string(match[1]) : "";

        if (update_user_password(user, new_pass)) {
            std::string json = "{\"token\":\"" + token + "\"}";
            std::vector<char> body(json.begin(), json.end());
            send_response(client_sock, "200 OK", body, "application/json");
        } else {
            send_error(client_sock, 500);
        }
    } else {
        send_error(client_sock, 404);
    }

    close_client(client_sock);
}

bool add_user(const std::string& username, const std::string& password) {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return false;

    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        debug_log("User not created...");
        return false;
    }

    std::string hashed_pass = hash_password(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_pass.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}
