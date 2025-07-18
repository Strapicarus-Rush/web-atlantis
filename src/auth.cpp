#include <iostream>
#include <random>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <sqlite3.h>
#include <iomanip>
// #include <vector>
#include <regex>
#include <ctime>
#include <stdexcept>
#include "utils.hpp"
#include "router.hpp"

static const std::string carus() {
    constexpr std::array<uint8_t, 34> mom = {
        0x1F, 0x36, 0x05, 0x11, 0x37, 0x23, 0x02, 0x0A, 0x2D,
        0x1E, 0x75, 0x2B, 0x15, 0x4F, 0x3B, 0x33, 0x5F, 0x42,
        0x38, 0x5C, 0x79, 0x52, 0x62, 0x0D, 0x7E, 0x65, 0x6B,
        0x2F, 0x78, 0x3E, 0x1A, 0x00, 0x4E, 0x7B
    };

    constexpr std::array<uint8_t, 7> libid = { 0x43, 0x52, 0x69, 0xB0, 0xD1, 0x5D, 0xF3 };

    char buffet[34];

    for (std::size_t i = 0; i < 34; ++i) {
        buffet[i] = static_cast<char>(mom[i] ^ libid[i % libid.size()]);
    }

    return std::string(buffet, 34);
}

static const std::string strapi = carus();

static uint32_t simple_hash(const std::string& str) {
    uint32_t h = 0x8F3C7BCE;
    for (char c : str) {
        h ^= (uint8_t)c;
        h *= 0x01C0F1F3;
    }
    return h;
}

static const std::vector<uint8_t> runtime_transform(const std::string& base) {
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

static void secure_zero(void* data, size_t len) {
    volatile uint8_t* ptr = reinterpret_cast<volatile uint8_t*>(data);
    while (len--) *ptr++ = 0;
}

static const std::string get_secret_key() {
    std::vector<uint8_t> raw = runtime_transform(strapi);

    std::string key(raw.begin(), raw.end());
    secure_zero(raw.data(), raw.size());

    return key;
}

static const std::string generate_salt(std::size_t length = 16) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<unsigned char> dis(0, 255);

    std::ostringstream oss;
    for (std::size_t i = 0; i < length; ++i){
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dis(gen));
    }
    return oss.str();
}

static const std::string sha256(const std::string& input) {
    std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx){
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 computation failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i){
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return oss.str();
}

static const std::string hmac_sha256(const std::string& key, const std::string& data) {
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

static const std::string hash_password(const std::string& password) {
    std::string salt = generate_salt();
    std::string hashed = sha256(salt + password);
    return salt + ":" + hashed;
}

static bool verify_password(const std::string& password, const std::string& stored) {
    auto sep = stored.find(':');
    if (sep == std::string::npos) {
        debug_log("ERROR: No se encontró separación en el hash");
        return false;
    }
    std::string salt = stored.substr(0, sep);
    std::string hash = stored.substr(sep + 1);
    return sha256(salt + password) == hash;
}

static const std::string generate_token(int& user_id) {
    sqlite3* db;
    if (sqlite3_open(get_db_file().c_str(), &db) != SQLITE_OK){
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
    try {
        if (sqlite3_open(get_db_file().c_str(), &db) != SQLITE_OK){
            throw std::runtime_error("Error abriendo base de datos");
        }

        const char* sql = R"(
            SELECT u.username, t.created_at, t.user_id
            FROM tokens t
            JOIN users u ON t.user_id = u.id
            WHERE t.token = ?
        )";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
            throw std::runtime_error("Error preparando consulta SQL");
        }

        if (sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC) != SQLITE_OK){
            throw std::runtime_error("Error vinculando token");
        }

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
                    if (time(nullptr) <= created_at + config.login_expiration) {
                        valid = true;
                    }
                } 
            } else {
                throw std::runtime_error("Formato del token inválido: se esperaban 3 partes, se recibieron " + std::to_string(parts.size()));
            }
        } else if (step_result == SQLITE_DONE) {
            debug_log("Token no encontrado en base de datos...");
        } else {
            throw std::runtime_error("sqlite3_step() error: " + std::string(sqlite3_errmsg(db)));
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return valid;
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Excepción en validate_token: ") + e.what());
        if (stmt) sqlite3_finalize(stmt);
        if (db) sqlite3_close(db);
        return false;
    }
}

bool validate_user(const std::string& user, const std::string& pass, int& out_user_id) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    // Abrir conexión
    if (sqlite3_open(get_db_file().c_str(), &db) != SQLITE_OK){
        debug_log("No se abrió la db...");
        return false;
    }
    // Preparar consulta
    std::string query = "SELECT id, password FROM users WHERE username = ?";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        debug_log("Error al sqlite3 prepare query...");
        return false;
    }

    // Bind del parámetro
    if (sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        debug_log("Error en sqlite3 bind parámetro...");
        return false;
    }

    // Ejecutar consulta
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* stored = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (stored && verify_password(pass, stored)) {
            out_user_id = id;
            ok = true;
        } else {
            debug_log("Algo pasó...");
        }
    } else {
        debug_log("No se encontró user...");
    }

    // Limpiar
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}

// --------------------- /login ---------------------
void handle_login(const ClientRequest& req, HttpResponse& res) {
    if(req.validated) debug_log("USER ALREADY LOGGED IN...");
        int out_user_id;
        std::string user = req.json_body.value("user", "");
        std::string pass = req.json_body.value("pass", "");

        if (validate_user(user, pass, out_user_id)) {
            std::string token = generate_token(out_user_id);
            json response_json = {
                {"success", true},
                {"login", "ok"}
            };
            res.set_status(200);
            res.set_json_body(response_json);
            std::string cookie = "Authorization=" + token + "; Path=/; Max-Age=3600; HttpOnly; SameSite=Strict";
            // if (!config.dev_mode) {
            //     cookie += "; Secure";
            // }
            res.add_cookie(cookie);
        } else {
            res.set_status(403);
        }
}

bool update_user_password(const std::string& username, const std::string& password){
    sqlite3* db;
    if (sqlite3_open(get_db_file().c_str(), &db) != SQLITE_OK) {
        return false;
    }

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
// void handle_edit_user(const ClientRequest& req, HttpResponse& res) {
//     // std::smatch match;
//     // std::regex token_rx("Authorization:\\s*(.*?)\r\n");

//     // if (!std::regex_search(request, match, token_rx)) {
//     //     send_error(client_sock, 403);
//     //     close_client(client_sock);
//     //     return;
//     // }

//     // std::string token = match[1];
//     // if (!validate_token(token)) {
//     //     send_error(client_sock, 403);
//     //     close_client(client_sock);
//     //     return;
//     // }

//     // std::regex user_rx("\"user\"\\s*:\\s*\"(.*?)\"");
//     // std::regex pass_rx("\"pass\"\\s*:\\s*\"(.*?)\"");
//     // std::regex new_pass_rx("\"new_pass\"\\s*:\\s*\"(.*?)\"");

//     // if (std::regex_search(body, match, user_rx)) {
//     //     std::string user = match[1];
//     //     std::string new_pass = std::regex_search(body, match, new_pass_rx) ? std::string(match[1]) : "";

//     //     if (update_user_password(user, new_pass)) {
//     //         std::string json = "{\"token\":\"" + token + "\"}";
//     //         std::vector<char> body(json.begin(), json.end());
//     //         send_response(client_sock, "200 OK", body, "application/json");
//     //     } else {
//     //         send_error(client_sock, 500);
//     //     }
//     // } else {
//     //     send_error(client_sock, 404);
//     // }

//     // close_client(client_sock);
// }

void add_user(const std::string& username, const std::string& password) {
    sqlite3* db;
    if (sqlite3_open(get_db_file().c_str(), &db) != SQLITE_OK) {
        std::runtime_error("No se pudo abrir el archivo db");
    }

    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        std::runtime_error("No se pudo crear el usuario" + username);
    }

    std::string hashed_pass = hash_password(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_pass.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (success) [[likely]]
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    } else {
        std::runtime_error("Falló step_result");
    }
}

static AutoRouteRegisterBatch auth_routes({
    {"POST", "/login", handle_login},
    // {"POST", "/user/create", create_user},
    // {"POST", "/user/password", update_password},
});