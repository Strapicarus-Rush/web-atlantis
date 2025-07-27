#ifndef INSTANCEMANAGER_HPP
#define INSTANCEMANAGER_HPP

#include <vector>
#include "ServerInstance.hpp"

class InstanceManager {
public:
    static std::vector<ServerInstance> instances;
    static std::mutex instance_mutex;

    static void load_instances_from_folder() {
        instances.clear();
        uint8_t id = 0;
        for (auto& entry : std::filesystem::directory_iterator(get_servers_path())) {
            if (entry.is_directory()) {
                id++;
                ServerInstance instance(entry.path().filename().string(), entry.path().string(), id);
                if (instance.is_valid()) {
                    instances.push_back(instance);
                }
            }
        }
    }

    static ServerInstance* get_instance_by_name(const std::string& name) {
        for (auto& inst : instances) {
            if (to_lower(inst.session) == to_lower(name)) return &inst;
        }
        return nullptr;
    }
};

#endif // INSTANCEMANAGER_HPP
