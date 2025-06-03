#pragma once

#include <string>
#include <iostream>
#include <fstream>

extern bool DEBUG_MODE;
extern bool LOG_MODE;
extern std::ofstream log_file;

inline void debug_log_internal(const std::string& msg, const char* file, int line, const char* func) {
    std::string full = "[DEBUG] " + std::string(file) + ":" + std::to_string(line) +
                       " (" + func + ") - " + msg;

    if (DEBUG_MODE)
        std::cerr << full << '\r' << '\n';

    if (LOG_MODE && log_file.is_open())
        log_file << full << '\r' << '\n';
}

#define debug_log(msg) debug_log_internal((msg), __FILE__, __LINE__, __func__)