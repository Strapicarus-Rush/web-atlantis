#include "router.hpp"
#include <unordered_map>
#include <string>
#include <sstream>

struct RouteNode {
    std::unordered_map<std::string, RouteNode> children;
    handler_func handler = nullptr;

    bool is_terminal() const {
        return handler != nullptr;
    }
};

static std::unordered_map<std::string, RouteNode> route_tree_map;

void register_route(const std::string& method, const std::string& path, handler_func func) {
    auto& root = route_tree_map[method];
    std::istringstream stream(path);
    std::string segment;
    RouteNode* current = &root;

    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            current = &current->children[segment];
        }
    }
    current->handler = std::move(func);
}

bool dispatch_route(const ClientRequest& req) {
    auto method_it = route_tree_map.find(req.method);
    if (method_it == route_tree_map.end()) {
        send_error(req.client_sock, 405);
        return false;
    }

    RouteNode* current = &method_it->second;
    std::istringstream stream(req.path);
    std::string segment;

    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            auto it = current->children.find(segment);
            if (it == current->children.end()) {
                debug_log("Ruta no existe: " + segment);
                send_error(req.client_sock, 404);
                return false;
            }
            current = &it->second;
        }
    }

    if (!current->is_terminal()) {
        debug_log("Ruta encontrada pero no terminal.");
        send_error(req.client_sock, 404);
        return false;
    }

    HttpResponse res;
    res.client_sock = req.client_sock;

    try {
        current->handler(req, res);
    } catch (const std::exception& e) {
        throw std::runtime_error("Excepción en handler: " + std::string(e.what()));
        // send_error(req.client_sock, 500, true);
        // return false;
    } catch (...) {
        throw std::runtime_error("Excepción desconocida en handler.");
        // send_error(req.client_sock, 500, true);
        // return false;
    }

    if (res.status_code == 200) {
        res.send(config.dev_mode);
        return true;
    } else {
        throw std::runtime_error("Handler retornó error: " + std::to_string(res.status_code));
        // send_error(req.client_sock, res.status_code, false);
        // return false;
    }
}
