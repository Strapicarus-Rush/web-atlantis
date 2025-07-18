#include "router.hpp"
#include "servers_utils.hpp"

static bool server_status(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}
    json response_json = general_status_json(); 
	response_json["success"] = true;
    response_json["message"] = "Status de Servidores";
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_status(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    ServerInstance instance;
    instance.name = req.json_body.value("name", "");
    if(instance.name.empty()) throw std::runtime_error("instance name empty...");

    json response_json = get_instance_status(instance);
    response_json["success"] = true;
    response_json["message"] = "Status de la sesión";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_console(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    ServerInstance instance;
    instance.name = req.json_body.value("name", "");

    if(instance.name.empty()) throw std::runtime_error("instance name empty...");

    instance.path = get_servers_path() + "/" + instance.name;
    set_instance_status(instance);

    json response_json;
    response_json["success"] = true;
    response_json["console"] = read_output_tmux(instance.name);
    response_json["message"] = "Lectura de consola";

    if (response_json["console"].empty())
    {
        response_json["success"] = false;
    }

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_run(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    ServerInstance instance;
    instance.name = req.json_body.value("name", "");
    if(instance.name.empty()) throw std::runtime_error("instance name empty...");
    instance.path = get_servers_path() + "/" + instance.name;
    set_instance_status(instance);
    
    json response_json;
    auto [success, message] = instance.is_active ? std::pair<bool, std::string>{instance.is_active, "La sesión ya está activa"} : run_instance(instance);
    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_stop(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    ServerInstance instance;
    instance.name = req.json_body.value("name", "");
    if(instance.name.empty()) throw std::runtime_error("instance name empty...");
    instance.path = get_servers_path() + "/" + instance.name;
    set_instance_status(instance);
    
    json response_json;
    auto [success, message] = !instance.is_active ? std::pair<bool, std::string>{instance.is_active, "La sesión no está activa"} : stop_tmux_instance(instance.name);
    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_reboot(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    ServerInstance instance;
    instance.name = req.json_body.value("name", "");
    if(instance.name.empty()) throw std::runtime_error("instance name empty...");
    instance.path = get_servers_path() + "/" + instance.name;
    set_instance_status(instance);
    
    json response_json;

    auto [success, message] = reboot_tmux_instance(instance);

    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_no_definido(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    // ServerInstance instance;
    // instance.name = req.json_body.value("name", "");
    // if(instance.name.empty()) throw std::runtime_error("instance name empty...");
    // instance.path = get_servers_path() + "/" + instance.name;
    // set_instance_status(instance);
    
    json response_json;
    
    // auto [success, message] = reboot_tmux_instance(instance);

    response_json["success"] = true;
    response_json["message"] = "Esta funcionalidad no está definida";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}


static AutoRouteRegisterBatch api_routes({
    {"REPORT", "/api/status", server_status},
    {"REPORT", "/api/instance/status", session_status},
    {"REPORT", "/api/instance/console", session_console},
    {"POST", "/api/instance/run", session_run},
    {"POST", "/api/instance/stop", session_stop},
    {"POST", "/api/instance/reboot", session_reboot},
    {"POST", "/api/instance/reboot", session_no_definido},
});