// #include <fstream>
// #include <sstream>
#include <unordered_map>
#include <filesystem>
#include <sqlite3.h>
#include <unistd.h>
#include <pwd.h>
#include "debug_log.hpp"

extern bool DEV_MODE;
extern std::string DB_PATH;
extern int PORT;
extern int TOKEN_EXPIRY;
extern int MAX_THREADS;

extern bool add_user(const std::string& username, const std::string& password);

const std::string style_css = R"ELCSS(:root {
  --bg-light: #ffffff;
  --bg-dark: #121212;
  --bg-gray: #2e2e2e;

  --text-light: #000000;
  --text-dark: #ffffff;
  --accent: #4a90e2;
  --error: #ff4c4c;
  --radius: 8px;
}

body {
  margin: 0;
  padding: 0;
  font-family: "Segoe UI", sans-serif;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 100vh;
  transition: background 0.3s, color 0.3s;
}

/* Tema claro por defecto */
body.light {
  background: var(--bg-light);
  color: var(--text-light);
}

body.dark {
  background: var(--bg-dark);
  color: var(--text-dark);
}

body.gray {
  background: var(--bg-gray);
  color: var(--text-dark);
}

.login-container {
  width: 90%;
  max-width: 400px;
  background: rgba(255, 255, 255, 0.05);
  backdrop-filter: blur(6px);
  padding: 2rem;
  border-radius: var(--radius);
  box-shadow: 0 4px 12px rgba(0,0,0,0.2);
}

.login-container h2 {
  text-align: center;
  margin-bottom: 1.5rem;
}

form {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

input {
  padding: 0.6rem;
  border: 1px solid #ccc;
  border-radius: var(--radius);
  font-size: 1rem;
}

button {
  padding: 0.7rem;
  background: var(--accent);
  color: white;
  border: none;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.3s;
}

button:hover {
  background: #357ac8;
}

.status-msg {
  margin-top: 1rem;
  text-align: center;
  color: var(--error);
}

.theme-selector {
  position: absolute;
  top: 1rem;
  right: 1rem;
  display: flex;
  gap: 0.5rem;
}

.theme-selector button {
  background: transparent;
  border: 1px solid #ccc;
  color: inherit;
  padding: 0.4rem 0.8rem;
  border-radius: var(--radius);
  cursor: pointer;
  font-size: 0.9rem;
  transition: background 0.2s, color 0.2s;
}

.theme-selector button:hover {
  background: var(--accent);
  color: white;
})ELCSS";

const std::string index_html = R"ELHTML(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Login</title>
  <link rel="stylesheet" href="style.css">
  <script>
    function getCookie(name) {
      const match = document.cookie.match(new RegExp('(^| )' + name + '=([^;]+)'));
      return match ? match[2] : null;
    }

    function applyInitialTheme() {
      const cookieTheme = getCookie("theme");
      const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
      document.body.className = cookieTheme || systemTheme;
    }

    applyInitialTheme();
  </script>
</head>
<body>
  <div class="theme-selector">
    <button onclick="setTheme('light')">Blanco</button>
    <button onclick="setTheme('dark')">Negro</button>
    <button onclick="setTheme('gray')">Gris</button>
  </div>

  <div class="login-container">
    <h2>Iniciar sesión</h2>
    <form id="login-form">
      <label for="user">Usuario</label>
      <input type="text" id="user" required>

      <label for="pass">Contraseña</label>
      <input type="password" id="pass" required>

      <button type="submit">Entrar</button>
    </form>
    <div id="status" class="status-msg"></div>
  </div>

  <script>
    function setTheme(theme) {
      document.body.className = theme;
      document.cookie = `theme=${theme}; path=/; max-age=31536000`;
    }

    document.getElementById("login-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      const user = document.getElementById("user").value;
      const pass = document.getElementById("pass").value;

      const res = await fetch("/login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ user, pass }),
      });

      const status = document.getElementById("status");
      if (res.ok) {
        const data = await res.json();
        localStorage.setItem("auth_token", data.token);
        window.location.href = "panel.html";
      } else {
        status.textContent = "Error: Usuario o contraseña incorrectos.";
      }
    });
  </script>
</body>
</html>
)ELHTML";

const std::string panel_html = R"HTML(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <title>Panel</title>
</head>
<body>
  <h2>Enviar comando a screen</h2>
  <form id="cmd-form">
    <label>Comando: <input type="text" id="cmd" required></label>
    <button type="submit">Ejecutar</button>
  </form>
  <div id="status"></div>

  <script>
    const token = localStorage.getItem("auth_token");
    if (!token) {
      alert("No autenticado. Redirigiendo al login.");
      window.location.href = "index.html";
    }

    document.getElementById("cmd-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      const cmd = document.getElementById("cmd").value;

      const res = await fetch("/run", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Authorization": token
        },
        body: JSON.stringify({ cmd }),
      });

      const status = document.getElementById("status");
      if (res.ok) {
        status.innerText = "Comando enviado con éxito.";
      } else {
        status.innerText = "Error al enviar comando.";
      }
    });
  </script>
</body>
</html>
)HTML";

bool initialize_database() {
    if (std::filesystem::exists(DB_PATH)) return true;

    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) {
        debug_log("Error al crear el archivo sqlite3");
        return false;
    };

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";

    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        debug_log(err);
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return add_user("admin", "admin");
}

std::string get_base_path() {
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }

    std::string app_path;
    if (DEV_MODE)
    {
      namespace fs = std::filesystem;
      // Ruta del ejecutable
      fs::path exePath = fs::canonical("/proc/self/exe");
      // Subir un nivel desde el directorio del ejecutable
      fs::path basePath = exePath.parent_path().parent_path();
      app_path = basePath;      
    }else{
      app_path = std::string(home) + "/.web-atlantis";
    }
    debug_log(app_path);
    return app_path;
}

void ensure_public_files(const std::string& base_path) {
    const std::string public_dir = base_path + "/public";
    // const std::string panel_dir = public_dir + "/panel";
    // const std::string error_dir = public_dir + "/error";
    // const std::string css_dir = public_dir + "/css";
    // const std::string assets = public_dir + "/assets";
    // const std::string js = public_dir + "/js";

    // std::filesystem::create_directories(public_dir);
    // std::filesystem::create_directories(panel_dir);
    // std::filesystem::create_directories(error_dir);
    // std::filesystem::create_directories(css_dir);

    // const std::string index_file = public_dir + "/index.html";
    // const std::string panel_file = panel_dir + "/index.html";
    // const std::string style_file = error_dir + "/style.css";

    // if (!std::filesystem::exists(index_file)) {
    //     std::ofstream(index_file) << index_html;
    // }

    // if (!std::filesystem::exists(panel_file)) {
    //     std::ofstream(panel_file) << panel_html;
    // }

    // if (!std::filesystem::exists(style_file)) {
    //     std::ofstream(style_file) << style_css;
    // }
}

void load_or_create_config() {
    const std::string base_path = get_base_path();
    const std::string config_path = base_path + "/server.config";

    std::filesystem::create_directories(base_path);
    ensure_public_files(base_path);

    std::ifstream infile(config_path);
    if (!infile.is_open()) {
        std::ofstream outfile(config_path);
        outfile << "DB_PATH=" << DB_PATH << "\n";
        outfile << "PORT=" << PORT << "\n";
        outfile << "TOKEN_EXPIRY=" << TOKEN_EXPIRY << "\n";
        outfile << "MAX_THREADS=" << MAX_THREADS << "\n";
        outfile.close();
        debug_log("Archivo de configuración por defecto creado...");
        return;
    }

    std::unordered_map<std::string, std::string> config;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
        }
    }

    // Asignación con validación
    if (config.count("DB_PATH")) {
        DB_PATH = config["DB_PATH"];
    }

    if (config.count("PORT")) {
        int port = std::stoi(config["PORT"]);
        //evitar puertos privilegiados.
        if (port >= 1024 && port <= 65535) {
            PORT = port;
        } else {
            debug_log("Valor inválido para PORT. Usando por defecto: " + std::to_string(PORT) + "\n");
        }
    }

    if (config.count("TOKEN_EXPIRY")) {
        int expiry = std::stoi(config["TOKEN_EXPIRY"]);
        if (expiry > 0) {
            TOKEN_EXPIRY = expiry;
        } else {
            debug_log("TOKEN_EXPIRY inválido. Usando por defecto: " + std::to_string(TOKEN_EXPIRY) + "\n");
        }
    }
}

std::string get_public_path() {
    return get_base_path() + "/public";
}

std::string get_log_path() {
    return get_base_path() + "/server.log";
}