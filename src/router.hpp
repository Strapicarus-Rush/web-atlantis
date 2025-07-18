#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "request.hpp"
#include <functional>
#include <string>
#include <unordered_map>
// #include <vector>

using handler_func = std::function<void(const ClientRequest&, HttpResponse&)>;

void register_route(const std::string& method, const std::string& path, handler_func handler);
bool dispatch_route(const ClientRequest& req);

// struct AutoRouteRegister {
//     AutoRouteRegister(const std::string& method, const std::string& path, handler_func handler) {
//         register_route(method, path, std::move(handler));
//     }
// };

struct AutoRouteRegisterBatch {
    AutoRouteRegisterBatch(const std::vector<std::tuple<std::string, std::string, handler_func>>& routes) {
        for (const auto& [method, path, handler] : routes) {
            register_route(method, path, handler);
        }
    }
};

#endif