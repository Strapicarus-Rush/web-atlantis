#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <optional>

using handler_func = std::function<void(int&, const std::string&, const bool&)>; //, const std::string&

extern void handle_login(int& client_sock, const std::string& body, const bool& validated);

struct RouteNode {
    std::unordered_map<std::string, RouteNode> children;
    handler_func handler = nullptr;

    bool is_terminal() const {
        return handler != nullptr;
    }
};


static std::unordered_map<std::string, RouteNode> route_tree_map;

static void register_route(const std::string& method, const std::string& path, handler_func func) {
    auto& root = route_tree_map[method];
    std::istringstream stream(path);
    std::string segment;
    RouteNode* current = &root;

    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            current = &current->children[segment];
        }
    }
    current->handler = func;
}

bool dispatch_route(int& client_sock, const std::string& method, const std::string& path, const std::string& body, const bool& validated) {
    auto method_it = route_tree_map.find(method);
    if (method_it == route_tree_map.end()) return false;

    RouteNode* current = &method_it->second;
    std::istringstream stream(path);
    std::string segment;

    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            auto it = current->children.find(segment);
            if (it == current->children.end()) return false;
            current = &it->second;
        }
    }

    if (current->is_terminal()) {
        current->handler(client_sock, body, validated);
        return true;
    }
    return false;
}

void setup_routes() {
    register_route("POST", "/login", handle_login);
    // register_route("POST", "/api/player/kick", handle_kick_player);
    // register_route("POST", "/api/player/list", [](int& sock, const std::string&, const std::string&) {
    //     debug_log("Player list route");
    //     send_response(sock, "200 OK", std::vector<char>(), "application/json");
    // });
    // Puedes agregar muchas más rutas aquí
}