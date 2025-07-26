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

    std::string instance_name = req.json_body.value("name", "");
    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}

    json response_json = get_instance_status(instance_name);
    response_json["success"] = true;
    response_json["message"] = "Status de la sesión" + instance_name;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_console(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}

    json response_json;
    response_json["success"] = true;
    response_json["console"] = get_instance_console(instance_name);
    response_json["message"] = "Lectura de consola";

    if (response_json["console"].empty()) [[unlikely]]
    {
        response_json["success"] = false;
    }

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_run(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}

    json response_json;
    auto [success, message] = run_instance(instance_name);
    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_stop(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    
    json response_json;
    auto [success, message] = stop_instance_session(instance_name);
    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_reboot(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    
    json response_json;

    auto [success, message] = reboot_instance(instance_name);

    response_json["success"] = success;
    response_json["message"] = message;

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}


static bool session_player_list(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}

    json response_json;
    response_json["success"] = true;
    response_json["message"] = get_list_players(instance_name);

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_count(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}

    json response_json;
    response_json["success"] = true;
    response_json["message"] = get_list_players(instance_name);

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_kick(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("kick ") + player + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Jugador " + player + " Kicked" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_ban(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("ban ") + player + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Jugador " + player + " banned" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_pardon(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("pardon ") + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Jugador " + player + " unbanned" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_add_white_list(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("whitelist add ") + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Jugador " + player + " added to whitelist" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_remove_white_list(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("whitelist remove ") + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Jugador " + player + " removed from whitelist" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_white_list(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    // std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("whitelist list ");// + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Mira la consola para ver la whitelist";
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_op(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("op ") + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "Op para " + player;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_deop(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string player = req.json_body.value("player", "");
    // std::string reason = req.json_body.value("reason", "");

    if(instance_name.empty() || player.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("deop ") + player;// + std::string(" reason ") + reason;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]]
    {
        response_json["message"] = "DeOp para " + player;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_player_op_json(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    response_json["oplist"] = get_op_file(instance_name);
    response_json["success"] = true;
    response_json["message"] = "Op list json";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_plugins_json(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    response_json["plugins"] = get_installed_plugins_json(instance_name);
    response_json["success"] = true;
    response_json["message"] = "Plugin list json";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_mods_json(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    response_json["mods"] = get_installed_mods_json(instance_name);
    response_json["success"] = true;
    response_json["message"] = "Mods list json";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_plugins_reload(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("reload");
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]] {
        response_json["message"] = "Plugins reloaded" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_plugin_info(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    std::string plugin_name = req.json_body.value("plugin", "");

    if(instance_name.empty() || plugin_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("pl ") + plugin_name;
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]] {
        response_json["message"] = "Revisa la consola para ver la info" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_world_info(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std:: string instance_name = req.json_body.value("name", "");
    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    
    json response_json = get_world_info_json(instance_name);
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_set_world_spawn(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");
    std::string x = req.json_body.value("x","");
    std::string y = req.json_body.value("y","");
    std::string z = req.json_body.value("z","");
    if(instance_name.empty() || x.empty() || y.empty() || z.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;
    std::string command = std::string("setworldspawn") + " " + x + " " + y + " " + z; 
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]] {
        response_json["message"] = "World Spawn actualizado" ;
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_kill_all_entities(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;
    std::string command = std::string("kill @e[type=!player]"); 
    auto [success, message] = send_instance_command(instance_name, command);
    response_json["success"] = success;
    if (success) [[likely]] {
        response_json["message"] = "Todas las entidades eliminadas";
    }else{
        response_json["message"] = message;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_kill_animals(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;
    bool err = false;
    std::string msg = "";
    for(auto& animal : animals){
        std::string command = std::string("kill @e[type=minecraft:"+animal+"]");
        auto [success, message] = send_instance_command(instance_name, command);
        if(!success) [[unlikely]] {
            msg = message;
            err = true;
            break;
        }
    }
    response_json["success"] = err ? false : true;
    if (!err) [[likely]] {
        response_json["message"] = "Todas las entidades eliminadas";
    }else{
        response_json["message"] = msg;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_kill_monster(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;
    bool err = false;
    std::string msg = "";
    for(auto& monster : monsters){
        std::string command = std::string("kill @e[type=minecraft:"+monster+"]");
        auto [success, message] = send_instance_command(instance_name, command);
        if(!success) [[unlikely]] {
            msg = message;
            err = true;
            break;
        }
    }
    response_json["success"] = err ? false : true;
    if (!err) [[likely]] {
        response_json["message"] = "Todas las entidades eliminadas";
    }else{
        response_json["message"] = msg;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_kill_item(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("kill @e[type=item]");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Todas los itemss eliminados";
    }else{
        response_json["message"] = success;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_kill_entity(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");
    std::string entity_name = req.json_body.value("entity", "");

    if(instance_name.empty() || entity_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("kill @e[type=minecraft:"+ entity_name +"]");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Entidades " + entity_name + " eliminadas";
    }else{
        response_json["message"] = success;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_world_save(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("save-all");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Mundo guardado";
    }else{
        response_json["message"] = success;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_autosave_status(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("save-query");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Estado autoguardado";
    }else{
        response_json["message"] = success;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_autosave_on(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("save-on");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Autoguardado on";
    }else{
        response_json["message"] = success;
    }
    
    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

static bool session_autosave_off(const ClientRequest& req, HttpResponse& res) {

    if(req.client_sock != res.client_sock) [[unlikely]] {throw std::runtime_error("Req != Res");}

    std::string instance_name = req.json_body.value("name", "");

    if(instance_name.empty()) [[unlikely]] {throw std::runtime_error("instance name empty...");}
    json response_json;

    std::string command = std::string("save-off");
    auto [success, message] = send_instance_command(instance_name, command);

    if (success) [[likely]] {
        response_json["message"] = "Autoguardado off";
    }else{
        response_json["message"] = success;
    }
    
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

    //send_command_instance
    
    json response_json;
    
    // auto [success, message] = reboot_tmux_instance(instance);

    response_json["success"] = true;
    response_json["message"] = "Esta funcionalidad no está definida";

    res.set_status(200);
    res.set_json_body(response_json);

    return true;
}

// static AutoRouteRegisterBatch api_routes({
//     {"REPORT", "/api/status", server_status},
//     {"REPORT", "/api/instance/status", session_status},
//     {"REPORT", "/api/instance/console", session_console},
//     {"POST", "/api/instance/run", session_run},
//     {"POST", "/api/instance/stop", session_stop},
//     {"POST", "/api/instance/reboot", session_reboot},
//     {"POST", "/api/instance/reboot", session_no_definido},
// });


static AutoRouteRegisterBatch api_routes({
    {"REPORT", "/api/status", server_status},
    {"REPORT", "/api/instance/status", session_status},
    {"REPORT", "/api/instance/console", session_console},
    {"POST", "/api/instance/run", session_run},
    {"POST", "/api/instance/stop", session_stop},
    {"POST", "/api/instance/reboot", session_reboot},
    {"POST", "/api/player/list", session_player_list},
    {"POST", "/api/player/count", session_player_count},
    {"POST", "/api/player/kick", session_player_kick},
    {"POST", "/api/player/ban", session_player_ban},
    {"POST", "/api/player/unban", session_player_pardon},
    {"POST", "/api/player/whitelist/add", session_player_add_white_list},
    {"POST", "/api/player/whitelist/remove", session_player_remove_white_list},
    {"POST", "/api/player/whitelist/list", session_player_white_list},
    {"POST", "/api/player/op/add", session_player_op},
    {"POST", "/api/player/op/remove", session_player_deop},
    {"POST", "/api/player/op/list", session_player_op_json},

    {"POST", "/api/plugin/list", session_plugins_json},
    {"POST", "/api/plugin/mods/list", session_mods_json},
    {"POST", "/api/plugin/reload", session_plugins_reload},
    {"POST", "/api/plugin/info", session_plugin_info},
    // {"POST", "/api/plugin/toggle", session_plugin_toggle},
    {"POST", "/api/plugin/logs", session_no_definido},

    {"POST", "/api/world/info", session_world_info},
    {"POST", "/api/world/change-spawn", session_set_world_spawn},
    {"POST", "/api/world/regenerate-chunks", session_no_definido},
    {"POST", "/api/world/entities/clear/all", session_kill_all_entities},
    {"POST", "/api/world/entities/clear/animals", session_kill_animals},
    {"POST", "/api/world/entities/clear/monsters", session_kill_monster},
    {"POST", "/api/world/entities/clear/item", session_kill_item},
    {"POST", "/api/world/entities/clear/entity", session_kill_entity},
    {"POST", "/api/world/save", session_world_save},
    {"POST", "/api/world/autosave/status", session_autosave_status},
    {"POST", "/api/world/autosave/on", session_autosave_on},
    {"POST", "/api/world/autosave/off", session_autosave_off},

    {"POST", "/api/backup/full", session_no_definido},
    {"POST", "/api/backup/world-only", session_no_definido},
    {"POST", "/api/backup/list", session_no_definido},
    {"POST", "/api/backup/restore", session_no_definido},
    {"POST", "/api/backup/delete", session_no_definido},
    {"POST", "/api/backup/configure-path", session_no_definido},

    {"POST", "/api/lightning/simple", session_no_definido},
    {"POST", "/api/lightning/storm", session_no_definido},
    {"POST", "/api/lightning/punishment", session_no_definido},
    {"POST", "/api/lightning/blessing", session_no_definido},
    {"POST", "/api/lightning/area", session_no_definido},
    {"POST", "/api/lightning/follow", session_no_definido},

    {"POST", "/api/item/give/diamond_sword", session_no_definido},
    {"POST", "/api/item/give/diamond_pickaxe", session_no_definido},
    {"POST", "/api/item/give/diamond_armor", session_no_definido},
    {"POST", "/api/item/give/elytra", session_no_definido},
    {"POST", "/api/item/give/totem_of_undying", session_no_definido},
    {"POST", "/api/item/give/enchanted_golden_apple", session_no_definido},
    {"POST", "/api/item/give/netherite_sword", session_no_definido},
    {"POST", "/api/item/give/bow", session_no_definido},
    {"POST", "/api/item/give/arrow", session_no_definido},
    {"POST", "/api/item/give/ender_pearl", session_no_definido},

    {"POST", "/api/mob/summon-on-player", session_no_definido},
    {"POST", "/api/mob/execute-summon", session_no_definido},
    {"POST", "/api/mob/summon-multiple", session_no_definido},
    {"POST", "/api/mob/summon-with-effects", session_no_definido},
    {"POST", "/api/mob/clear-specific", session_no_definido},

    {"POST", "/api/player/tp/spawn", session_no_definido},
    {"POST", "/api/player/tp/coords", session_no_definido},
    {"POST", "/api/player/gamemode/creative", session_no_definido},
    {"POST", "/api/player/gamemode/survival", session_no_definido},
    {"POST", "/api/player/gamemode/adventure", session_no_definido},
    {"POST", "/api/player/gamemode/spectator", session_no_definido},
    {"POST", "/api/player/heal", session_no_definido},
    {"POST", "/api/player/feed", session_no_definido},
    {"POST", "/api/player/clear-inventory", session_no_definido},

    {"POST", "/api/world/time/day", session_no_definido},
    {"POST", "/api/world/time/night", session_no_definido},
    {"POST", "/api/world/time/noon", session_no_definido},
    {"POST", "/api/world/time/midnight", session_no_definido},
    {"POST", "/api/world/weather/clear", session_no_definido},
    {"POST", "/api/world/weather/rain", session_no_definido},
    {"POST", "/api/world/weather/thunder", session_no_definido},
    {"POST", "/api/world/difficulty/peaceful", session_no_definido},
    {"POST", "/api/world/difficulty/hard", session_no_definido},
    {"POST", "/api/world/gamerule/stop-daylight", session_no_definido},
    {"POST", "/api/world/gamerule/start-daylight", session_no_definido},
    {"POST", "/api/world/gamerule/disable-mob-spawn", session_no_definido},
    {"POST", "/api/world/gamerule/enable-mob-spawn", session_no_definido},
    {"POST", "/api/instance/nodefined", session_no_definido}
});