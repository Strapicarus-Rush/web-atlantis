#include "debug_log.hpp"
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \r\n\t");
    auto end = s.find_last_not_of(" \r\n\t");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string get_mime_type(const std::string& raw_path) {
    std::string path = to_lower(trim(raw_path));
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".ico")) return "image/x-icon";
    if (path.ends_with(".woff")) return "font/woff";
    if (path.ends_with(".woff2")) return "font/woff2";
    if (path.ends_with(".ttf")) return "font/ttf";
    if (path.ends_with(".otf")) return "font/otf";
    if (path.ends_with(".mp3")) return "audio/mpeg";
    if (path.ends_with(".wav")) return "audio/wav";
    if (path.ends_with(".mp4")) return "video/mp4";
    if (path.ends_with(".zip")) return "application/zip";
    if (path.ends_with(".rar")) return "application/vnd.rar";
    if (path.ends_with(".exe")) return "application/octet-stream";
    if (path.ends_with(".bin")) return "application/octet-stream";
    return "Error";
}