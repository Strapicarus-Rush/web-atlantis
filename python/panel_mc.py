import subprocess
import os
import time
import psutil# type: ignore
import re
import json
import shutil
import zipfile
from datetime import datetime
from rich.console import Console# type: ignore
from rich.prompt import Prompt, IntPrompt, Confirm# type: ignore
from rich.table import Table# type: ignore
from rich.panel import Panel# type: ignore
from rich.progress import Progress, SpinnerColumn, TextColumn# type: ignore
from rich.layout import Layout# type: ignore
from rich.live import Live # type: ignore
import math
import random
import threading
from difflib import get_close_matches

console = Console()

#region Variables globales
SERVER_PATH = ""
SCREEN_NAME = ""
BACKUP_PATH = ""
CONFIG_FILE = "config.json"
#endregion

#region Listas de items y mobs
def load_items():
    try:
        with open('items.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        console.print("[yellow]Archivo items.json no encontrado, usando lista básica[/yellow]")
        return get_basic_items()
def load_mobs():
    try:
        with open('mobs.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        console.print("[yellow]Archivo mobs.json no encontrado, usando lista básica[/yellow]")
        return get_basic_mobs()
def get_basic_items():
    return {
        "minecraft:diamond_sword": "Espada de Diamante",
        "minecraft:diamond_pickaxe": "Pico de Diamante",
        "minecraft:diamond_axe": "Hacha de Diamante",
        "minecraft:diamond_shovel": "Pala de Diamante",
        "minecraft:diamond_hoe": "Azada de Diamante",
        "minecraft:diamond_helmet": "Casco de Diamante",
        "minecraft:diamond_chestplate": "Pechera de Diamante",
        "minecraft:diamond_leggings": "Pantalones de Diamante",
        "minecraft:diamond_boots": "Botas de Diamante",
        "minecraft:netherite_sword": "Espada de Netherita",
        "minecraft:netherite_pickaxe": "Pico de Netherita",
        "minecraft:netherite_axe": "Hacha de Netherita",
        "minecraft:netherite_shovel": "Pala de Netherita",
        "minecraft:netherite_hoe": "Azada de Netherita",
        "minecraft:netherite_helmet": "Casco de Netherita",
        "minecraft:netherite_chestplate": "Pechera de Netherita",
        "minecraft:netherite_leggings": "Pantalones de Netherita",
        "minecraft:netherite_boots": "Botas de Netherita",
        "minecraft:bow": "Arco",
        "minecraft:crossbow": "Ballesta",
        "minecraft:arrow": "Flecha",
        "minecraft:spectral_arrow": "Flecha Espectral",
        "minecraft:tipped_arrow": "Flecha con Efecto",
        "minecraft:shield": "Escudo",
        "minecraft:elytra": "Élitros",
        "minecraft:totem_of_undying": "Tótem de la Inmortalidad",
        "minecraft:golden_apple": "Manzana Dorada",
        "minecraft:enchanted_golden_apple": "Manzana Dorada Encantada",
        "minecraft:ender_pearl": "Perla de Ender",
        "minecraft:ender_eye": "Ojo de Ender",
        "minecraft:blaze_rod": "Vara de Blaze",
        "minecraft:nether_star": "Estrella del Nether",
        "minecraft:dragon_egg": "Huevo de Dragón",
        "minecraft:beacon": "Faro",
        "minecraft:conduit": "Conducto",
        "minecraft:heart_of_the_sea": "Corazón del Mar",
        "minecraft:nautilus_shell": "Concha de Nautilo",
        "minecraft:trident": "Tridente",
        "minecraft:mending": "Libro de Reparación",
        "minecraft:sharpness": "Libro de Filo",
        "minecraft:protection": "Libro de Protección",
        "minecraft:efficiency": "Libro de Eficiencia",
        "minecraft:unbreaking": "Libro de Irrompibilidad",
        "minecraft:fortune": "Libro de Fortuna",
        "minecraft:silk_touch": "Libro de Toque Sedoso",
        "minecraft:fire_aspect": "Libro de Aspecto Ígneo",
        "minecraft:knockback": "Libro de Empuje",
        "minecraft:looting": "Libro de Botín",
        "minecraft:power": "Libro de Poder",
        "minecraft:punch": "Libro de Golpe",
        "minecraft:flame": "Libro de Llama",
        "minecraft:infinity": "Libro de Infinidad",
        "minecraft:thorns": "Libro de Espinas",
        "minecraft:respiration": "Libro de Respiración",
        "minecraft:aqua_affinity": "Libro de Afinidad Acuática",
        "minecraft:depth_strider": "Libro de Agilidad Acuática",
        "minecraft:frost_walker": "Libro de Paso Helado",
        "minecraft:feather_falling": "Libro de Caída de Pluma",
        "minecraft:blast_protection": "Libro de Protección contra Explosiones",
        "minecraft:projectile_protection": "Libro de Protección contra Proyectiles",
        "minecraft:fire_protection": "Libro de Protección contra Fuego",
        "minecraft:soul_speed": "Libro de Velocidad del Alma",
        "minecraft:swift_sneak": "Libro de Sigilo Rápido",
        "minecraft:loyalty": "Libro de Lealtad",
        "minecraft:impaling": "Libro de Empalamiento",
        "minecraft:riptide": "Libro de Corriente",
        "minecraft:channeling": "Libro de Canalización",
        "minecraft:multishot": "Libro de Disparo Múltiple",
        "minecraft:quick_charge": "Libro de Carga Rápida",
        "minecraft:piercing": "Libro de Perforación"
    }
def get_basic_mobs():
    return {
        "minecraft:allay": "Allay",
        "minecraft:axolotl": "Ajolote",
        "minecraft:bat": "Murciélago",
        "minecraft:bee": "Abeja",
        "minecraft:blaze": "Blaze",
        "minecraft:cat": "Gato",
        "minecraft:cave_spider": "Araña de Cueva",
        "minecraft:chicken": "Pollo",
        "minecraft:cod": "Bacalao",
        "minecraft:cow": "Vaca",
        "minecraft:creeper": "Creeper",
        "minecraft:dolphin": "Delfín",
        "minecraft:donkey": "Burro",
        "minecraft:drowned": "Ahogado",
        "minecraft:elder_guardian": "Guardián Anciano",
        "minecraft:ender_dragon": "Dragón del End",
        "minecraft:enderman": "Enderman",
        "minecraft:endermite": "Endermite",
        "minecraft:evoker": "Invocador",
        "minecraft:fox": "Zorro",
        "minecraft:frog": "Rana",
        "minecraft:ghast": "Ghast",
        "minecraft:glow_squid": "Calamar Luminoso",
        "minecraft:goat": "Cabra",
        "minecraft:guardian": "Guardián",
        "minecraft:hoglin": "Hoglin",
        "minecraft:horse": "Caballo",
        "minecraft:husk": "Momia",
        "minecraft:iron_golem": "Golem de Hierro",
        "minecraft:llama": "Llama",
        "minecraft:magma_cube": "Cubo de Magma",
        "minecraft:mooshroom": "Champiñaca",
        "minecraft:mule": "Mula",
        "minecraft:ocelot": "Ocelote",
        "minecraft:panda": "Panda",
        "minecraft:parrot": "Loro",
        "minecraft:phantom": "Fantasma",
        "minecraft:pig": "Cerdo",
        "minecraft:piglin": "Piglin",
        "minecraft:piglin_brute": "Piglin Bruto",
        "minecraft:pillager": "Saqueador",
        "minecraft:polar_bear": "Oso Polar",
        "minecraft:pufferfish": "Pez Globo",
        "minecraft:rabbit": "Conejo",
        "minecraft:ravager": "Devastador",
        "minecraft:salmon": "Salmón",
        "minecraft:sheep": "Oveja",
        "minecraft:shulker": "Shulker",
        "minecraft:silverfish": "Lepisma",
        "minecraft:skeleton": "Esqueleto",
        "minecraft:skeleton_horse": "Caballo Esqueleto",
        "minecraft:slime": "Slime",
        "minecraft:snow_golem": "Golem de Nieve",
        "minecraft:spider": "Araña",
        "minecraft:squid": "Calamar",
        "minecraft:stray": "Esqueleto Glacial",
        "minecraft:strider": "Zancudo",
        "minecraft:tadpole": "Renacuajo",
        "minecraft:trader_llama": "Llama Comerciante",
        "minecraft:tropical_fish": "Pez Tropical",
        "minecraft:turtle": "Tortuga",
        "minecraft:vex": "Vex",
        "minecraft:villager": "Aldeano",
        "minecraft:vindicator": "Vindicador",
        "minecraft:wandering_trader": "Comerciante Ambulante",
        "minecraft:warden": "Guardián",
        "minecraft:witch": "Bruja",
        "minecraft:wither": "Wither",
        "minecraft:wither_skeleton": "Esqueleto Wither",
        "minecraft:wolf": "Lobo",
        "minecraft:zoglin": "Zoglin",
        "minecraft:zombie": "Zombie",
        "minecraft:zombie_horse": "Caballo Zombie",
        "minecraft:zombie_villager": "Aldeano Zombie",
        "minecraft:zombified_piglin": "Piglin Zombificado"
    }
def create_default_files():
    """Crea los archivos JSON por defecto si no existen"""
    if not os.path.exists('items.json'):
        items_data = get_basic_items()
        with open('items.json', 'w', encoding='utf-8') as f:
            json.dump(items_data, f, indent=2, ensure_ascii=False)
        console.print("[green]Archivo items.json creado con items básicos de Minecraft 1.20.2[/green]")
    
    if not os.path.exists('mobs.json'):
        mobs_data = get_basic_mobs()
        with open('mobs.json', 'w', encoding='utf-8') as f:
            json.dump(mobs_data, f, indent=2, ensure_ascii=False)
        console.print("[green]Archivo mobs.json creado con mobs básicos de Minecraft 1.20.2[/green]")
#endregion

#region busqueda
def search_items_or_mobs(query, items_dict, max_results=10):
    """Busca items o mobs que coincidan con la consulta"""
    if not query:
        return []
    
    query = query.lower()
    matches = []
    
    # Buscar coincidencias exactas en nombres
    for item_id, name in items_dict.items():
        if query in name.lower() or query in item_id.lower():
            matches.append((item_id, name))
    
    # Si no hay muchas coincidencias, usar coincidencias aproximadas
    if len(matches) < max_results:
        all_names = list(items_dict.values())
        close_matches = get_close_matches(query, all_names, n=max_results-len(matches), cutoff=0.3)
        for match in close_matches:
            for item_id, name in items_dict.items():
                if name == match and (item_id, name) not in matches:
                    matches.append((item_id, name))
                    break
    
    return matches[:max_results]
def select_item_or_mob(items_dict, item_type="item"):
    """Función mejorada para seleccionar items o mobs con búsqueda y autocompletado"""
    console.print(f"[cyan]Seleccionar {item_type}:[/cyan]")
    console.print("[yellow]Puedes:[/yellow]")
    console.print("[yellow]- Escribir un número para seleccionar de la lista[/yellow]")
    console.print("[yellow]- Escribir parte del nombre para buscar[/yellow]")
    console.print("[yellow]- Escribir 'list' para ver todos[/yellow]")
    console.print("[yellow]- Escribir 'back' para volver[/yellow]")
    
    while True:
        choice = Prompt.ask(f"Buscar/seleccionar {item_type}")
        
        if choice.lower() == 'back':
            return None
        elif choice.lower() == 'list':
            show_paginated_list(items_dict, item_type)
            continue
        elif choice.isdigit():
            # Si es un número, mostrar lista paginada y permitir selección
            return select_from_paginated_list(items_dict, int(choice), item_type)
        else:
            # Buscar coincidencias
            matches = search_items_or_mobs(choice, items_dict)
            if not matches:
                console.print(f"[red]No se encontraron {item_type}s que coincidan con '{choice}'[/red]")
                continue
            
            if len(matches) == 1:
                item_id, name = matches[0]
                if Confirm.ask(f"¿Seleccionar {name}?"):
                    return item_id
            else:
                # Mostrar opciones encontradas
                table = Table(title=f"Resultados de búsqueda para '{choice}'")
                table.add_column("N°", style="cyan")
                table.add_column("Nombre", style="green")
                table.add_column("ID", style="yellow")
                
                for i, (item_id, name) in enumerate(matches, 1):
                    table.add_row(str(i), name, item_id)
                
                console.print(table)
                
                selection = IntPrompt.ask("Selecciona una opción (0 para nueva búsqueda)", default=0)
                if selection == 0:
                    continue
                elif 1 <= selection <= len(matches):
                    return matches[selection - 1][0]
                else:
                    console.print("[red]Selección inválida[/red]")
def show_paginated_list(items_dict, item_type, page_size=20):
    """Muestra una lista paginada de items o mobs"""
    items_list = list(items_dict.items())
    total_pages = math.ceil(len(items_list) / page_size)
    current_page = 1
    
    while True:
        start_idx = (current_page - 1) * page_size
        end_idx = start_idx + page_size
        page_items = items_list[start_idx:end_idx]
        
        table = Table(title=f"{item_type.title()}s - Página {current_page}/{total_pages}")
        table.add_column("N°", style="cyan")
        table.add_column("Nombre", style="green")
        table.add_column("ID", style="yellow")
        
        for i, (item_id, name) in enumerate(page_items, start_idx + 1):
            table.add_row(str(i), name, item_id)
        
        console.print(table)
        
        console.print(f"[cyan]Página {current_page} de {total_pages}[/cyan]")
        console.print("[yellow]Comandos: [n]ext, [p]rev, [g]oto, [s]elect, [q]uit[/yellow]")
        
        cmd = Prompt.ask("Comando").lower()
        
        if cmd == 'n' and current_page < total_pages:
            current_page += 1
        elif cmd == 'p' and current_page > 1:
            current_page -= 1
        elif cmd == 'g':
            page = IntPrompt.ask(f"Ir a página (1-{total_pages})", default=current_page)
            if 1 <= page <= total_pages:
                current_page = page
        elif cmd == 's':
            selection = IntPrompt.ask("Seleccionar número", default=0)
            if 1 <= selection <= len(items_list):
                return items_list[selection - 1][0]
        elif cmd == 'q':
            return None
def select_from_paginated_list(items_dict, selection, item_type):
    """Selecciona un item específico por número de la lista completa"""
    items_list = list(items_dict.items())
    if 1 <= selection <= len(items_list):
        item_id, name = items_list[selection - 1]
        console.print(f"[green]Seleccionado: {name}[/green]")
        return item_id
    else:
        console.print(f"[red]Número inválido. Debe estar entre 1 y {len(items_list)}[/red]")
        return None
#endregion

#region FUNCIONES DEL SERVIDOR
def get_ram_usage(screen_name):
    for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'memory_info']):
        if 'SCREEN' in proc.info['name'].upper():
            cmdline = ' '.join(proc.info['cmdline'])
            if screen_name in cmdline:
                mem = proc.info['memory_info'].rss / 1024 / 1024  # En MB
                return round(mem, 2)
    return None
def get_cpu_usage(screen_name):
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        if 'SCREEN' in proc.info['name'].upper():
            cmdline = ' '.join(proc.info['cmdline'])
            if screen_name in cmdline:
                try:
                    process = psutil.Process(proc.info['pid'])
                    return round(process.cpu_percent(interval=1), 2)
                except:
                    return None
    return None
def is_server_running(screen_name):
    result = subprocess.run("screen -list", shell=True, capture_output=True, text=True)
    return screen_name in result.stdout
def send_command(screen_name, command):
    full_command = f'screen -S {screen_name} -X stuff "{command}\\n"'
    subprocess.run(full_command, shell=True)
    console.print(f"[green]Comando enviado:[/green] {command}")
def start_server(screen_name, server_path):
    run_sh_path = os.path.join(server_path, "run.sh")

    if not os.path.isfile(run_sh_path):
        console.print(f"[red]❌ El archivo 'run.sh' no se encuentra en {server_path}.[/red]")
        return
    if not os.access(run_sh_path, os.X_OK):
        console.print(f"[red]❌ El archivo 'run.sh' no tiene permisos de ejecución. Usa 'chmod +x run.sh'.[/red]")
        return

    result = subprocess.run("screen -list", shell=True, capture_output=True, text=True)
    if screen_name in result.stdout:
        console.print(f"[cyan]✅ La screen '{screen_name}' ya está activa. Ejecutando './run.sh'...[/cyan]")
        send_command(screen_name, "./run.sh")
    else:
        console.print(f"[green]🚀 Creando nueva screen '{screen_name}' y ejecutando './run.sh'...[/green]")
        command = f"screen -dmS {screen_name} bash -c 'cd \"{server_path}\" && ./run.sh; exec bash'"
        subprocess.run(command, shell=True)
        console.print(f"[green]🟢 Screen '{screen_name}' creada y servidor iniciado.[/green]")
def stop_server():
    console.print("[yellow]Deteniendo servidor...[/yellow]")
    send_command(SCREEN_NAME, "stop")
def is_server_running(screen_name):
    """Verifica si el servidor está respondiendo a comandos"""
    try:
        # Enviamos un comando simple para ver si responde
        output = get_command_output(screen_name, "list", 3)
        return "There are" in output or "players online" in output
    except:
        return False
def restart_server():
    """Reinicia el servidor de manera segura verificando el estado real"""
    console.print("[yellow]🔄 Iniciando proceso de reinicio...[/yellow]")
    
    # 1. Detener el servidor
    console.print("[yellow]⏳ Enviando comando stop al servidor...[/yellow]")
    send_command(SCREEN_NAME, "stop")
    
    # 2. Esperar y verificar que se haya detenido
    max_wait = 60  # 1 minuto máximo de espera
    wait_interval = 5
    attempts = max_wait // wait_interval
    
    console.print("[cyan]🔍 Verificando que el servidor se haya detenido...[/cyan]")
    
    for i in range(attempts):
        time.sleep(wait_interval)
        
        if not is_server_running(SCREEN_NAME):
            console.print("[green]✅ Servidor detenido correctamente[/green]")
            break
        
        console.print(f"[yellow]⌛ Esperando que el servidor se detenga ({i+1}/{attempts})...[/yellow]")
    else:
        console.print("[red]❌ El servidor no respondió al comando stop[/red]")
        console.print("[yellow]⚠️ Intentando reinicio forzado...[/yellow]")
    
    # 3. Iniciar el servidor nuevamente
    console.print("[green]🚀 Iniciando servidor...[/green]")
    start_server(SCREEN_NAME, SERVER_PATH)
    
    # 4. Verificar que el servidor esté activo
    console.print("[cyan]🔍 Verificando estado del servidor...[/cyan]")
    
    for i in range(attempts):
        time.sleep(wait_interval)
        
        if is_server_running(SCREEN_NAME):
            console.print("[green]✅ Servidor reiniciado correctamente y listo[/green]")
            return True
        
        console.print(f"[yellow]⌛ Esperando que el servidor inicie ({i+1}/{attempts})...[/yellow]")
    
    console.print("[red]❌ El servidor no respondió después del reinicio[/red]")
    return False
def safe_print(message, style="white"):
    """Función auxiliar para imprimir mensajes de forma segura escapando markup"""
    # Escapar corchetes que pueden causar problemas de markup
    safe_message = str(message).replace('[', '\\[').replace(']', '\\]')
    console.print(safe_message, style=style)
def send_command(screen_name, command):
    """Versión mejorada de send_command con manejo seguro de errores"""
    full_command = f'screen -S {screen_name} -X stuff "{command}\\n"'
    try:
        subprocess.run(full_command, shell=True)
        console.print(f"Comando enviado: {command}", style="green")
    except Exception as e:
        # Usar safe_print para evitar errores de markup
        safe_print(f"Error al enviar comando: {e}", "red")
#endregion

#region Comandos rápidos
def quick_commands_menu():
    while True:
        table = Table(title="Comandos Rápidos Mejorados")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        submenu_table = Table(title="Categorías de Comandos")
        submenu_table.add_column("Opción", style="cyan")
        submenu_table.add_column("Categoría", style="green")
        
        submenu_table.add_row("1", "🌍 Comandos de Mundo (tiempo, clima, dificultad)")
        submenu_table.add_row("2", "🎁 Comandos de Items (con búsqueda mejorada)")
        submenu_table.add_row("3", "👤 Comandos de Jugador (tp, gamemode)")
        submenu_table.add_row("4", "👹 Comandos de Mobs (con búsqueda mejorada)")
        submenu_table.add_row("5", "⚡ Comandos Especiales (efectos, partículas)")
        submenu_table.add_row("6", "⚡ Comandos de Rayos Avanzados")
        submenu_table.add_row("0", "⬅ Regresar")
        
        console.print(submenu_table)
        category = Prompt.ask("Selecciona una categoría o escribe 'back' para volver")
        
        if category == "0":
            break
        elif category == "1":
            world_commands_menu()
        elif category == "2":
            improved_item_commands_menu()
        elif category == "3":
            player_commands_menu()
        elif category == "4":
            improved_mob_commands_menu()
        elif category == "5":
            special_commands_menu()
        elif category == "6":
            lightning_commands_menu()
        elif category.lower() != "back":
            console.print("[red]Categoría no válida.[/red]")
def world_commands_menu():
    while True:
        table = Table(title="🌍 Comandos de Mundo")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        commands = {
            "1": ('time set day', "Establecer el día"),
            "2": ('time set night', "Establecer la noche"),
            "3": ('time set noon', "Establecer mediodía"),
            "4": ('time set midnight', "Establecer medianoche"),
            "5": ('weather clear', "Clima despejado"),
            "6": ('weather rain', "Activar lluvia"),
            "7": ('weather thunder', "Activar tormenta"),
            "8": ('difficulty peaceful', "Dificultad pacífica"),
            "9": ('difficulty easy', "Dificultad fácil"),
            "10": ('difficulty normal', "Dificultad normal"),
            "11": ('difficulty hard', "Dificultad difícil"),
            "12": ('gamerule doDaylightCycle false', "Detener ciclo día/noche"),
            "13": ('gamerule doDaylightCycle true', "Activar ciclo día/noche"),
            "14": ('gamerule doMobSpawning false', "Desactivar spawn de mobs"),
            "15": ('gamerule doMobSpawning true', "Activar spawn de mobs"),
        }

        for k, (cmd, desc) in commands.items():
            table.add_row(k, desc)

        console.print(table)
        sel = Prompt.ask("Selecciona un comando o escribe 'back' para volver")

        if sel in commands:
            send_command(SCREEN_NAME, commands[sel][0])
        elif sel.lower() != "back":
            console.print("[red]Comando no válido.[/red]")
        elif sel.lower() == "back":
            break
def improved_item_commands_menu():
    """Menú mejorado de comandos de items con búsqueda y autocompletado"""
    items_dict = load_items()
    
    console.print("[cyan]🎁 Comandos de Items Mejorados[/cyan]")
    
    # Seleccionar item
    item_id = select_item_or_mob(items_dict, "item")
    if not item_id:
        return
    
    # Seleccionar jugador
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    players_table = Table(title="Jugadores Conectados")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)

    choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if choice == 0:
        return
    if 1 <= choice <= len(players):
        player = players[choice - 1]
        quantity = IntPrompt.ask("Cantidad", default=1)
        send_command(SCREEN_NAME, f"give {player} {item_id} {quantity}")
    else:
        console.print("[red]Selección inválida[/red]")
def improved_mob_commands_menu():
    """Menú mejorado de comandos de mobs combinando funcionalidades del viejo y nuevo"""
    table = Table(title="👹 Comandos de Mobs Avanzados")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Summon mob simple en jugador")
    table.add_row("2", "Execute at jugador - Summon mob con offset")
    table.add_row("3", "🔥 Summon múltiples mobs (DINÁMICO)")
    table.add_row("4", "✨ Summon mob con efectos especiales")
    table.add_row("5", "🧹 Limpiar mobs específicos")
    table.add_row("6", "🌪️ Summon en área con patrones avanzados")

    console.print(table)
    sel = Prompt.ask("Selecciona una opción o escribe 'back' para volver")

    if sel == "1":
        summon_mob_simple()
    elif sel == "2":
        execute_summon_at_player()
    elif sel == "3":
        summon_multiple_mobs_dynamic()
    elif sel == "4":
        summon_mob_with_effects()
    elif sel == "5":
        clear_specific_mobs()
    elif sel == "6":
        summon_area_patterns()
    elif sel.lower() != "back":
        console.print("[red]Opción no válida.[/red]")
def player_commands_menu1():
    table = Table(title="👤 Comandos de Jugador")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Teletransportar a spawn")
    table.add_row("2", "Teletransportar a coordenadas")
    table.add_row("3", "Modo creativo")
    table.add_row("4", "Modo supervivencia")
    table.add_row("5", "Modo aventura")
    table.add_row("6", "Modo espectador")
    table.add_row("7", "Curar jugador")
    table.add_row("8", "Alimentar jugador")
    table.add_row("9", "Limpiar inventario")

    console.print(table)
    sel = Prompt.ask("Selecciona un comando o escribe 'back' para volver")

    players = get_connected_players(SCREEN_NAME)
    if not players and sel != "back":
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    if sel != "back" and sel.isdigit() and 1 <= int(sel) <= 9:
        players_table = Table(title="Jugadores Conectados")
        players_table.add_column("N°", style="cyan", no_wrap=True)
        players_table.add_column("Jugador", style="green")
        for i, p in enumerate(players, start=1):
            players_table.add_row(str(i), p)
        console.print(players_table)

        choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
        if choice == 0:
            return
        if 1 <= choice <= len(players):
            player = players[choice - 1]
            
            if sel == "1":
                send_command(SCREEN_NAME, f"tp {player} 0 100 0")
            elif sel == "2":
                x = IntPrompt.ask("Coordenada X")
                y = IntPrompt.ask("Coordenada Y")
                z = IntPrompt.ask("Coordenada Z")
                send_command(SCREEN_NAME, f"tp {player} {x} {y} {z}")
            elif sel == "3":
                send_command(SCREEN_NAME, f"gamemode creative {player}")
            elif sel == "4":
                send_command(SCREEN_NAME, f"gamemode survival {player}")
            elif sel == "5":
                send_command(SCREEN_NAME, f"gamemode adventure {player}")
            elif sel == "6":
                send_command(SCREEN_NAME, f"gamemode spectator {player}")
            elif sel == "7":
                send_command(SCREEN_NAME, f"effect give {player} minecraft:instant_health 1 10")
            elif sel == "8":
                send_command(SCREEN_NAME, f"effect give {player} minecraft:saturation 1 10")
            elif sel == "9":
                send_command(SCREEN_NAME, f"clear {player}")
        else:
            console.print("[red]Selección inválida[/red]")
    elif sel.lower() != "back":
        console.print("[red]Comando no válido.[/red]")
def player_commands_menu():
    table = Table(title="👤 Comandos de Jugador")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Teletransportar a spawn")
    table.add_row("2", "Teletransportar a coordenadas")
    table.add_row("3", "Teletransportar jugador a otro jugador")
    table.add_row("4", "Teletransportar TODOS a coordenadas")
    table.add_row("5", "Teletransportar TODOS a un jugador")
    table.add_row("6", "Modo creativo")
    table.add_row("7", "Modo supervivencia")
    table.add_row("8", "Modo aventura")
    table.add_row("9", "Modo espectador")
    table.add_row("10", "Cambiar modo de juego a TODOS")
    table.add_row("11", "Curar jugador")
    table.add_row("12", "Alimentar jugador")
    table.add_row("13", "Curar y alimentar TODOS")
    table.add_row("14", "Limpiar inventario")
    table.add_row("15", "Limpiar inventario de TODOS")
    table.add_row("16", "Dar item personalizado")
    table.add_row("17", "Aplicar efecto personalizado")

    console.print(table)
    sel = Prompt.ask("Selecciona un comando o escribe 'back' para volver")

    players = get_connected_players(SCREEN_NAME)
    if not players and sel != "back":
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    if sel != "back" and sel.isdigit() and 1 <= int(sel) <= 17:
        # Comandos que afectan a TODOS los jugadores
        if sel in ["4", "5", "10", "13", "15"]:
            handle_all_players_commands(sel, players)
        # Comandos que requieren selección de jugador
        elif sel in ["1", "2", "6", "7", "8", "9", "11", "12", "14", "16", "17"]:
            handle_single_player_commands(sel, players)
        # Comandos especiales (tp entre jugadores)
        elif sel == "3":
            handle_player_to_player_tp(players)
    elif sel.lower() != "back":
        console.print("[red]Comando no válido.[/red]")
def show_players_table(players, title="Jugadores Conectados"):
    """Muestra una tabla con los jugadores conectados"""
    players_table = Table(title=title)
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)
def handle_single_player_commands(sel, players):
    """Maneja comandos que afectan a un solo jugador"""
    show_players_table(players)
    
    choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if choice == 0:
        return
    if 1 <= choice <= len(players):
        player = players[choice - 1]
        
        if sel == "1":  # TP a spawn
            send_command(SCREEN_NAME, f"tp {player} 0 100 0")
            console.print(f"[green]✓ {player} teletransportado al spawn[/green]")
            
        elif sel == "2":  # TP a coordenadas
            x = IntPrompt.ask("Coordenada X")
            y = IntPrompt.ask("Coordenada Y") 
            z = IntPrompt.ask("Coordenada Z")
            send_command(SCREEN_NAME, f"tp {player} {x} {y} {z}")
            console.print(f"[green]✓ {player} teletransportado a ({x}, {y}, {z})[/green]")
            
        elif sel == "6":  # Modo creativo
            send_command(SCREEN_NAME, f"gamemode creative {player}")
            console.print(f"[green]✓ {player} cambiado a modo creativo[/green]")
            
        elif sel == "7":  # Modo supervivencia
            send_command(SCREEN_NAME, f"gamemode survival {player}")
            console.print(f"[green]✓ {player} cambiado a modo supervivencia[/green]")
            
        elif sel == "8":  # Modo aventura
            send_command(SCREEN_NAME, f"gamemode adventure {player}")
            console.print(f"[green]✓ {player} cambiado a modo aventura[/green]")
            
        elif sel == "9":  # Modo espectador
            send_command(SCREEN_NAME, f"gamemode spectator {player}")
            console.print(f"[green]✓ {player} cambiado a modo espectador[/green]")
            
        elif sel == "11":  # Curar
            send_command(SCREEN_NAME, f"effect give {player} minecraft:instant_health 1 10")
            console.print(f"[green]✓ {player} curado[/green]")
            
        elif sel == "12":  # Alimentar
            send_command(SCREEN_NAME, f"effect give {player} minecraft:saturation 1 10")
            console.print(f"[green]✓ {player} alimentado[/green]")
            
        elif sel == "14":  # Limpiar inventario
            send_command(SCREEN_NAME, f"clear {player}")
            console.print(f"[green]✓ Inventario de {player} limpiado[/green]")
            
        elif sel == "16":  # Dar item personalizado
            handle_give_item(player)
            
        elif sel == "17":  # Aplicar efecto personalizado
            handle_custom_effect(player)
    else:
        console.print("[red]Selección inválida[/red]")
def handle_all_players_commands(sel, players):
    """Maneja comandos que afectan a todos los jugadores"""
    console.print(f"[yellow]⚠️  Este comando afectará a TODOS los jugadores conectados ({len(players)} jugadores)[/yellow]")
    
    # Mostrar lista de jugadores que serán afectados
    show_players_table(players, "Jugadores que serán afectados")
    
    confirm = Prompt.ask("¿Estás seguro? (s/n)", default="n")
    if confirm.lower() != "s":
        console.print("[yellow]Operación cancelada[/yellow]")
        return
    
    if sel == "4":  # TP todos a coordenadas
        x = IntPrompt.ask("Coordenada X")
        y = IntPrompt.ask("Coordenada Y")
        z = IntPrompt.ask("Coordenada Z")
        for player in players:
            send_command(SCREEN_NAME, f"tp {player} {x} {y} {z}")
        console.print(f"[green]✓ Todos los jugadores teletransportados a ({x}, {y}, {z})[/green]")
        
    elif sel == "5":  # TP todos a un jugador
        show_players_table(players, "Selecciona el jugador destino")
        choice = IntPrompt.ask("Selecciona jugador destino (0 para cancelar)", default=0)
        if choice == 0:
            return
        if 1 <= choice <= len(players):
            target_player = players[choice - 1]
            for player in players:
                if player != target_player:  # No teletransportar al jugador a sí mismo
                    send_command(SCREEN_NAME, f"tp {player} {target_player}")
            console.print(f"[green]✓ Todos los jugadores teletransportados a {target_player}[/green]")
        
    elif sel == "10":  # Cambiar modo de juego a todos
        modes = {
            "1": ("creative", "creativo"),
            "2": ("survival", "supervivencia"), 
            "3": ("adventure", "aventura"),
            "4": ("spectator", "espectador")
        }
        
        console.print("\n[cyan]Modos de juego disponibles:[/cyan]")
        for key, (mode, name) in modes.items():
            console.print(f"  {key}. {name.capitalize()}")
            
        mode_choice = Prompt.ask("Selecciona el modo de juego")
        if mode_choice in modes:
            mode, mode_name = modes[mode_choice]
            for player in players:
                send_command(SCREEN_NAME, f"gamemode {mode} {player}")
            console.print(f"[green]✓ Todos los jugadores cambiados a modo {mode_name}[/green]")
        
    elif sel == "13":  # Curar y alimentar a todos
        for player in players:
            send_command(SCREEN_NAME, f"effect give {player} minecraft:instant_health 1 10")
            send_command(SCREEN_NAME, f"effect give {player} minecraft:saturation 1 10")
        console.print("[green]✓ Todos los jugadores curados y alimentados[/green]")
        
    elif sel == "15":  # Limpiar inventario de todos
        for player in players:
            send_command(SCREEN_NAME, f"clear {player}")
        console.print("[green]✓ Inventarios de todos los jugadores limpiados[/green]")
def handle_player_to_player_tp(players):
    """Maneja el teletransporte entre jugadores"""
    if len(players) < 2:
        console.print("[red]Se necesitan al menos 2 jugadores para esta función[/red]")
        return
    
    console.print("\n[cyan]Selecciona el jugador que será teletransportado:[/cyan]")
    show_players_table(players, "Jugador origen")
    
    origin_choice = IntPrompt.ask("Selecciona jugador origen (0 para cancelar)", default=0)
    if origin_choice == 0:
        return
    if not (1 <= origin_choice <= len(players)):
        console.print("[red]Selección inválida[/red]")
        return
    
    origin_player = players[origin_choice - 1]
    
    # Filtrar jugadores para mostrar solo los posibles destinos
    destination_players = [p for p in players if p != origin_player]
    
    console.print(f"\n[cyan]Selecciona el jugador destino para {origin_player}:[/cyan]")
    dest_table = Table(title="Jugadores destino disponibles")
    dest_table.add_column("N°", style="cyan", no_wrap=True)
    dest_table.add_column("Jugador", style="green")
    for i, p in enumerate(destination_players, start=1):
        dest_table.add_row(str(i), p)
    console.print(dest_table)
    
    dest_choice = IntPrompt.ask("Selecciona jugador destino (0 para cancelar)", default=0)
    if dest_choice == 0:
        return
    if 1 <= dest_choice <= len(destination_players):
        dest_player = destination_players[dest_choice - 1]
        send_command(SCREEN_NAME, f"tp {origin_player} {dest_player}")
        console.print(f"[green]✓ {origin_player} teletransportado a {dest_player}[/green]")
    else:
        console.print("[red]Selección inválida[/red]")
def handle_give_item(player):
    """Maneja dar items personalizados a un jugador"""
    console.print(f"\n[cyan]Dar item a {player}:[/cyan]")
    
    # Items comunes predefinidos
    common_items = {
        "1": ("minecraft:diamond_sword", "Espada de diamante"),
        "2": ("minecraft:diamond_pickaxe", "Pico de diamante"),
        "3": ("minecraft:diamond_armor", "Armadura de diamante completa"),
        "4": ("minecraft:golden_apple", "Manzana dorada"),
        "5": ("minecraft:elytra", "Élitros"),
        "6": ("minecraft:totem_of_undying", "Tótem de la inmortalidad"),
        "7": ("custom", "Item personalizado")
    }
    
    console.print("\n[cyan]Items disponibles:[/cyan]")
    for key, (item, name) in common_items.items():
        console.print(f"  {key}. {name}")
    
    item_choice = Prompt.ask("Selecciona un item")
    
    if item_choice in common_items:
        if item_choice == "7":  # Item personalizado
            item_id = Prompt.ask("ID del item (ej: minecraft:diamond)")
            quantity = IntPrompt.ask("Cantidad", default=1)
            send_command(SCREEN_NAME, f"give {player} {item_id} {quantity}")
            console.print(f"[green]✓ {quantity}x {item_id} dado a {player}[/green]")
        elif item_choice == "3":  # Armadura completa
            armor_pieces = ["diamond_helmet", "diamond_chestplate", "diamond_leggings", "diamond_boots"]
            for piece in armor_pieces:
                send_command(SCREEN_NAME, f"give {player} minecraft:{piece}")
            console.print(f"[green]✓ Armadura de diamante completa dada a {player}[/green]")
        else:
            item_id, item_name = common_items[item_choice]
            quantity = IntPrompt.ask("Cantidad", default=1)
            send_command(SCREEN_NAME, f"give {player} {item_id} {quantity}")
            console.print(f"[green]✓ {quantity}x {item_name} dado a {player}[/green]")
def handle_custom_effect(player):
    """Maneja aplicar efectos personalizados a un jugador"""
    console.print(f"\n[cyan]Aplicar efecto a {player}:[/cyan]")
    
    effects = {
        "1": ("minecraft:speed", "Velocidad"),
        "2": ("minecraft:strength", "Fuerza"),
        "3": ("minecraft:jump_boost", "Salto mejorado"),
        "4": ("minecraft:regeneration", "Regeneración"),
        "5": ("minecraft:fire_resistance", "Resistencia al fuego"),
        "6": ("minecraft:water_breathing", "Respiración acuática"),
        "7": ("minecraft:night_vision", "Visión nocturna"),
        "8": ("minecraft:invisibility", "Invisibilidad"),
        "9": ("custom", "Efecto personalizado")
    }
    
    console.print("\n[cyan]Efectos disponibles:[/cyan]")
    for key, (effect, name) in effects.items():
        console.print(f"  {key}. {name}")
    
    effect_choice = Prompt.ask("Selecciona un efecto")
    
    if effect_choice in effects:
        if effect_choice == "9":  # Efecto personalizado
            effect_id = Prompt.ask("ID del efecto (ej: minecraft:speed)")
            duration = IntPrompt.ask("Duración en segundos", default=60)
            amplifier = IntPrompt.ask("Amplificador (0-255)", default=0)
            send_command(SCREEN_NAME, f"effect give {player} {effect_id} {duration} {amplifier}")
            console.print(f"[green]✓ Efecto {effect_id} aplicado a {player}[/green]")
        else:
            effect_id, effect_name = effects[effect_choice]
            duration = IntPrompt.ask("Duración en segundos", default=60)
            amplifier = IntPrompt.ask("Amplificador (0-255)", default=0)
            send_command(SCREEN_NAME, f"effect give {player} {effect_id} {duration} {amplifier}")
            console.print(f"[green]✓ {effect_name} aplicado a {player} por {duration} segundos[/green]")
def special_commands_menu():
    table = Table(title="⚡ Comandos Especiales")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Lanzar rayo")
    table.add_row("2", "Crear explosión")
    table.add_row("3", "Efectos de partículas")
    table.add_row("4", "Dar efectos de poción")
    table.add_row("5", "Crear estructura")

    console.print(table)
    sel = Prompt.ask("Selecciona un comando o escribe 'back' para volver")

    if sel == "1":  # Rayo
        players = get_connected_players(SCREEN_NAME)
        if not players:
            console.print("[red]No hay jugadores conectados.[/red]")
            return
            
        players_table = Table(title="Jugadores Conectados")
        players_table.add_column("N°", style="cyan", no_wrap=True)
        players_table.add_column("Jugador", style="green")
        for i, p in enumerate(players, start=1):
            players_table.add_row(str(i), p)
        console.print(players_table)

        player_choice = IntPrompt.ask("Selecciona jugador para el rayo (0 para cancelar)", default=0)
        if player_choice == 0:
            return
        if 1 <= player_choice <= len(players):
            player = players[player_choice - 1]
            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt")
        else:
            console.print("[red]Selección inválida[/red]")
    
    elif sel == "2":  # Explosión
        power = IntPrompt.ask("Poder de explosión (1-10)", default=3)
        send_command(SCREEN_NAME, f"summon tnt ~ ~ ~ {{Fuse:0,ExplosionPower:{power}}}")
    
    elif sel == "3":  # Partículas
        particles = ["heart", "flame", "smoke", "portal", "enchant", "crit", "magic_crit"]
        console.print(f"[yellow]Partículas disponibles: {', '.join(particles)}[/yellow]")
        particle = Prompt.ask("Tipo de partícula", default="heart")
        send_command(SCREEN_NAME, f"particle {particle} ~ ~ ~ 1 1 1 0.1 100")
    
    elif sel == "4":  # Efectos de poción
        players = get_connected_players(SCREEN_NAME)
        if not players:
            console.print("[red]No hay jugadores conectados.[/red]")
            return
            
        players_table = Table(title="Jugadores Conectados")
        players_table.add_column("N°", style="cyan", no_wrap=True)
        players_table.add_column("Jugador", style="green")
        for i, p in enumerate(players, start=1):
            players_table.add_row(str(i), p)
        console.print(players_table)

        player_choice = IntPrompt.ask("Selecciona jugador (0 para cancelar)", default=0)
        if player_choice == 0:
            return
        if 1 <= player_choice <= len(players):
            player = players[player_choice - 1]
            
            effects = ["speed", "strength", "jump_boost", "regeneration", "fire_resistance", 
                      "water_breathing", "night_vision", "invisibility", "slowness", "weakness"]
            console.print(f"[yellow]Efectos disponibles: {', '.join(effects)}[/yellow]")
            effect = Prompt.ask("Tipo de efecto", default="speed")
            duration = IntPrompt.ask("Duración en segundos", default=60)
            amplifier = IntPrompt.ask("Amplificador (0-10)", default=1)
            
            send_command(SCREEN_NAME, f"effect give {player} minecraft:{effect} {duration} {amplifier}")
    
    elif sel == "5":  # Crear estructura
        structures = ["village", "mansion", "monument", "fortress", "stronghold", "temple"]
        console.print(f"[yellow]Estructuras disponibles: {', '.join(structures)}[/yellow]")
        structure = Prompt.ask("Tipo de estructura", default="village")
        send_command(SCREEN_NAME, f"locate structure minecraft:{structure}")
    
    elif sel.lower() != "back":
        console.print("[red]Comando no válido.[/red]")
def lightning_commands_menu():
    """Menú dedicado específicamente a comandos de rayos mejorado"""
    table = Table(title="⚡ Comandos de Rayos Avanzados")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Rayo simple en jugador")
    table.add_row("2", "Tormenta de rayos en jugador")
    table.add_row("3", "Rayo de castigo (con efectos)")
    table.add_row("4", "Rayo de bendición (sin daño)")
    table.add_row("5", "Lluvia de rayos en área")
    table.add_row("6", "Rayo siguiendo al jugador")
    table.add_row("7", "🌟 Lluvia de rayos de bendición")
    table.add_row("8", "🌩️ Lluvia de rayos sobre jugador")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    players = get_connected_players(SCREEN_NAME)
    
    if choice == 0:
        return
    elif choice in [1, 2, 3, 4, 6, 7, 8] and not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    if choice == 1:  # Rayo simple
        player = select_player_from_list(players)
        if player:
            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt")
            console.print(f"[yellow]⚡ Rayo lanzado sobre {player}[/yellow]")
    
    elif choice == 2:  # Tormenta de rayos
        player = select_player_from_list(players)
        if player:
            intensity = IntPrompt.ask("Intensidad de la tormenta (1-10)", default=5)
            duration = IntPrompt.ask("Duración en segundos", default=10)
        
            console.print(f"[yellow]🌩️ Iniciando tormenta sobre {player} por {duration} segundos...[/yellow]")
        
            def lightning_storm():
                end_time = time.time() + duration
                while time.time() < end_time:
                    for _ in range(intensity):
                        x_offset = random.randint(-3, 3)
                        z_offset = random.randint(-3, 3)
                        send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~{x_offset} ~ ~{z_offset}")
                    time.sleep(1)
        
            storm_thread = threading.Thread(target=lightning_storm)
            storm_thread.daemon = True
            storm_thread.start()
    
    elif choice == 3:  # Rayo de castigo
        player = select_player_from_list(players)
        if player:
            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt")
            send_command(SCREEN_NAME, f"effect give {player} minecraft:slowness 10 2")
            send_command(SCREEN_NAME, f"effect give {player} minecraft:weakness 10 1")
            console.print(f"[red]⚡ Rayo de castigo lanzado sobre {player}[/red]")
    
    elif choice == 4:  # Rayo de bendición
        player = select_player_from_list(players)
        if player:
            # Rayo sin daño usando area_effect_cloud
            send_command(SCREEN_NAME, f"execute at {player} run summon area_effect_cloud ~ ~ ~ {{Duration:1,Effects:[{{Id:10,Amplifier:2,Duration:200}}]}}")
            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~ ~ ~ {{Tags:[\"no_damage\"]}}")
            send_command(SCREEN_NAME, f"effect give {player} minecraft:regeneration 10 2")
            send_command(SCREEN_NAME, f"effect give {player} minecraft:absorption 30 1")
            console.print(f"[green]⚡ Rayo de bendición lanzado sobre {player}[/green]")
    
    elif choice == 5:  # Lluvia de rayos en área
        x = IntPrompt.ask("Centro X")
        z = IntPrompt.ask("Centro Z")
        y = IntPrompt.ask("Altura Y", default=100)
        radius = IntPrompt.ask("Radio del área", default=10)
        count = IntPrompt.ask("Cantidad de rayos", default=20)
    
        console.print(f"[yellow]🌩️ Lanzando {count} rayos en área...[/yellow]")
    
        for i in range(count):
            offset_x = random.randint(-radius, radius)
            offset_z = random.randint(-radius, radius)
            target_x = x + offset_x
            target_z = z + offset_z
        
            send_command(SCREEN_NAME, f"summon lightning_bolt {target_x} {y} {target_z}")
            time.sleep(0.5)
    
    elif choice == 6:  # Rayo siguiendo al jugador
        player = select_player_from_list(players)
        if player:
            duration = IntPrompt.ask("Duración en segundos", default=15)
            frequency = IntPrompt.ask("Frecuencia (rayos por segundo)", default=2)
        
            console.print(f"[yellow]⚡ Rayos siguiendo a {player} por {duration} segundos...[/yellow]")
        
            def follow_lightning():
                end_time = time.time() + duration
                while time.time() < end_time:
                    send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~ ~ ~")
                    time.sleep(1/frequency)
        
            follow_thread = threading.Thread(target=follow_lightning)
            follow_thread.daemon = True
            follow_thread.start()
    
    elif choice == 7:  # 🌟 NUEVA: Lluvia de rayos de bendición
        # Configuración de la lluvia de bendición
        blessing_table = Table(title="🌟 Configuración de Lluvia de Bendición")
        blessing_table.add_column("Opción", style="cyan")
        blessing_table.add_column("Descripción", style="green")
        
        blessing_table.add_row("1", "Lluvia de bendición en área específica")
        blessing_table.add_row("2", "Lluvia de bendición centrada en jugador")
        
        console.print(blessing_table)
        blessing_type = IntPrompt.ask("Tipo de lluvia de bendición", default=1)
        
        if blessing_type == 1:  # Área específica
            x = IntPrompt.ask("Centro X")
            z = IntPrompt.ask("Centro Z")
            y = IntPrompt.ask("Altura Y", default=100)
            radius = IntPrompt.ask("Radio del área", default=15)
            count = IntPrompt.ask("Cantidad de rayos de bendición", default=25)
            delay = IntPrompt.ask("Delay entre rayos (segundos)", default=1)
            
            console.print(f"[green]🌟 Lanzando {count} rayos de bendición en área...[/green]")
            
            def blessing_rain_area():
                for i in range(count):
                    offset_x = random.randint(-radius, radius)
                    offset_z = random.randint(-radius, radius)
                    target_x = x + offset_x
                    target_z = z + offset_z
                    
                    # Crear área de efecto con efectos positivos
                    send_command(SCREEN_NAME, f"summon area_effect_cloud {target_x} {y} {target_z} {{Duration:100,Effects:[{{Id:10,Amplifier:3,Duration:300}},{{Id:11,Amplifier:2,Duration:300}},{{Id:22,Amplifier:1,Duration:600}}]}}")
                    # Rayo visual sin daño
                    send_command(SCREEN_NAME, f"execute positioned {target_x} {y} {target_z} run summon lightning_bolt ~ ~ ~ {{Tags:[\"blessing\"]}}")
                    
                    console.print(f"[green]Rayo de bendición {i+1}/{count} lanzado[/green]")
                    time.sleep(delay)
                
                console.print(f"[bold green]🌟 Lluvia de bendición completada! {count} rayos lanzados[/bold green]")
            
            blessing_thread = threading.Thread(target=blessing_rain_area)
            blessing_thread.daemon = True
            blessing_thread.start()
            
        elif blessing_type == 2:  # Centrada en jugador
            player = select_player_from_list(players)
            if player:
                radius = IntPrompt.ask("Radio alrededor del jugador", default=10)
                count = IntPrompt.ask("Cantidad de rayos de bendición", default=20)
                delay = IntPrompt.ask("Delay entre rayos (segundos)", default=1)
                
                console.print(f"[green]🌟 Lanzando {count} rayos de bendición alrededor de {player}...[/green]")
                
                def blessing_rain_player():
                    for i in range(count):
                        offset_x = random.randint(-radius, radius)
                        offset_z = random.randint(-radius, radius)
                        
                        # Crear área de efecto con efectos positivos alrededor del jugador
                        send_command(SCREEN_NAME, f"execute at {player} run summon area_effect_cloud ~{offset_x} ~ ~{offset_z} {{Duration:100,Effects:[{{Id:10,Amplifier:3,Duration:300}},{{Id:11,Amplifier:2,Duration:300}},{{Id:22,Amplifier:1,Duration:600}}]}}")
                        # Rayo visual sin daño
                        send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~{offset_x} ~ ~{offset_z} {{Tags:[\"blessing\"]}}")
                        
                        console.print(f"[green]Rayo de bendición {i+1}/{count} lanzado alrededor de {player}[/green]")
                        time.sleep(delay)
                    
                    # Dar efectos especiales al jugador objetivo
                    send_command(SCREEN_NAME, f"effect give {player} minecraft:regeneration 30 3")
                    send_command(SCREEN_NAME, f"effect give {player} minecraft:absorption 60 2")
                    send_command(SCREEN_NAME, f"effect give {player} minecraft:resistance 30 1")
                    send_command(SCREEN_NAME, f"effect give {player} minecraft:speed 30 1")
                    
                    console.print(f"[bold green]🌟 Lluvia de bendición completada sobre {player}![/bold green]")
                
                blessing_thread = threading.Thread(target=blessing_rain_player)
                blessing_thread.daemon = True
                blessing_thread.start()
    
    elif choice == 8:  # 🌩️ NUEVA: Lluvia de rayos sobre jugador
        player = select_player_from_list(players)
        if player:
            # Configuración de la lluvia sobre jugador
            rain_table = Table(title="🌩️ Configuración de Lluvia sobre Jugador")
            rain_table.add_column("Parámetro", style="cyan")
            rain_table.add_column("Descripción", style="green")
            
            rain_table.add_row("Radio", "Área alrededor del jugador donde caerán los rayos")
            rain_table.add_row("Cantidad", "Número total de rayos a lanzar")
            rain_table.add_row("Intensidad", "Rayos por segundo")
            rain_table.add_row("Duración", "Tiempo total de la lluvia")
            rain_table.add_row("Tipo", "Normal (con daño) o Espectacular (solo visual)")
            
            console.print(rain_table)
            
            radius = IntPrompt.ask("Radio alrededor del jugador", default=8)
            count = IntPrompt.ask("Cantidad total de rayos", default=30)
            intensity = IntPrompt.ask("Rayos por segundo", default=3)
            duration = IntPrompt.ask("Duración total (segundos)", default=15)
            
            # Tipo de rayos
            type_table = Table(title="Tipo de Rayos")
            type_table.add_column("Opción", style="cyan")
            type_table.add_column("Tipo", style="green")
            type_table.add_column("Descripción", style="yellow")
            
            type_table.add_row("1", "Normal", "Rayos con daño estándar")
            type_table.add_row("2", "Espectacular", "Solo efectos visuales, sin daño")
            type_table.add_row("3", "Devastador", "Rayos con mayor poder destructivo")
            
            console.print(type_table)
            lightning_type = IntPrompt.ask("Tipo de rayos", default=1)
            
            console.print(f"[yellow]🌩️ Iniciando lluvia de rayos sobre {player}...[/yellow]")
            console.print(f"[yellow]Configuración: {count} rayos, {intensity}/seg, radio {radius}, duración {duration}s[/yellow]")
            
            def player_lightning_rain():
                start_time = time.time()
                rayos_lanzados = 0
                
                while time.time() - start_time < duration and rayos_lanzados < count:
                    # Lanzar múltiples rayos según la intensidad
                    for _ in range(intensity):
                        if rayos_lanzados >= count:
                            break
                            
                        offset_x = random.randint(-radius, radius)
                        offset_z = random.randint(-radius, radius)
                        
                        if lightning_type == 1:  # Normal
                            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~{offset_x} ~ ~{offset_z}")
                        elif lightning_type == 2:  # Espectacular (sin daño)
                            send_command(SCREEN_NAME, f"execute at {player} run summon area_effect_cloud ~{offset_x} ~ ~{offset_z} {{Duration:1,Effects:[{{Id:15,Amplifier:0,Duration:1}}]}}")
                            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~{offset_x} ~ ~{offset_z} {{Tags:[\"spectacle\"]}}")
                            # Efectos de partículas adicionales
                            send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:electric_spark ~{offset_x} ~ ~{offset_z} 2 2 2 0.1 50")
                        elif lightning_type == 3:  # Devastador
                            send_command(SCREEN_NAME, f"execute at {player} run summon lightning_bolt ~{offset_x} ~ ~{offset_z}")
                            send_command(SCREEN_NAME, f"execute at {player} run summon tnt ~{offset_x} ~ ~{offset_z} {{Fuse:10}}")
                        
                        rayos_lanzados += 1
                        
                        # Mostrar progreso cada 5 rayos
                        if rayos_lanzados % 5 == 0:
                            console.print(f"[cyan]Progreso: {rayos_lanzados}/{count} rayos lanzados[/cyan]")
                    
                    time.sleep(1)  # Esperar 1 segundo antes del siguiente grupo
                
                # Efectos finales según el tipo
                if lightning_type == 2:  # Espectacular
                    send_command(SCREEN_NAME, f"effect give {player} minecraft:glowing 10 0")
                    send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:firework ~ ~1 ~ 3 3 3 0.1 100")
                elif lightning_type == 3:  # Devastador
                    send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:explosion_emitter ~ ~ ~ 5 5 5 0.1 10")
                
                console.print(f"[bold yellow]🌩️ Lluvia de rayos completada! {rayos_lanzados} rayos lanzados sobre {player}[/bold yellow]")
            
            rain_thread = threading.Thread(target=player_lightning_rain)
            rain_thread.daemon = True
            rain_thread.start()
def create_lightning_effects(player, effect_type="normal"):
    """Crea efectos de partículas especiales para los rayos"""
    if effect_type == "blessing":
        # Partículas doradas y brillantes
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:end_rod ~ ~1 ~ 2 2 2 0.1 20")
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:enchant ~ ~1 ~ 3 3 3 0.1 30")
    elif effect_type == "spectacle":
        # Partículas eléctricas y coloridas
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:electric_spark ~ ~1 ~ 3 3 3 0.1 50")
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:firework ~ ~1 ~ 2 2 2 0.1 25")
    elif effect_type == "devastation":
        # Partículas de explosión y humo
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:explosion ~ ~1 ~ 2 2 2 0.1 15")
        send_command(SCREEN_NAME, f"execute at {player} run particle minecraft:large_smoke ~ ~1 ~ 3 3 3 0.1 40")
def summon_mob_simple():
    """Summon simple usando el sistema de búsqueda del nuevo panel"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob usando el sistema mejorado
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return
    
    # Seleccionar jugador
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    player = select_player_from_list(players)
    if player:
        send_command(SCREEN_NAME, f"execute at {player} run summon {mob_id}")
        console.print(f"[green]✅ {mobs_dict.get(mob_id, mob_id)} spawneado en {player}[/green]")
def execute_summon_at_player():
    """Execute at jugador con offset - FUNCIONALIDAD DEL VIEJO PANEL"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob usando el sistema mejorado
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return
    
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    player = select_player_from_list(players)
    if not player:
        return
    
    # Configurar offset
    console.print("[yellow]Configurar posición relativa (offset):[/yellow]")
    x_offset = IntPrompt.ask("Offset X", default=1)
    y_offset = IntPrompt.ask("Offset Y", default=0)
    z_offset = IntPrompt.ask("Offset Z", default=1)

    # Cantidad de veces a ejecutar
    repeat_count = IntPrompt.ask("¿Cuántas veces ejecutar el comando?", default=1)

    # Ejecutar comando múltiples veces
    for i in range(repeat_count):
        command = f"execute at {player} run summon {mob_id} ~{x_offset} ~{y_offset} ~{z_offset}"
        send_command(SCREEN_NAME, command)
        if repeat_count > 1:
            console.print(f"[green]Ejecutando comando {i+1}/{repeat_count}[/green]")
            time.sleep(0.5)
def summon_multiple_mobs_dynamic():
    """Summon múltiples mobs - FUNCIONALIDAD PRINCIPAL DEL VIEJO PANEL"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob usando el sistema mejorado
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return
    
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    player = select_player_from_list(players)
    if not player:
        return
    
    # Cantidad de mobs
    mob_count = IntPrompt.ask("¿Cuántos mobs summon?", default=5)
    
    # Patrón de spawn
    pattern_table = Table(title="Patrón de Spawn")
    pattern_table.add_column("Opción", style="cyan")
    pattern_table.add_column("Descripción", style="green")
    
    pattern_table.add_row("1", "Círculo alrededor del jugador")
    pattern_table.add_row("2", "Línea recta")
    pattern_table.add_row("3", "Aleatorio en área")
    pattern_table.add_row("4", "Cuadrado/Grid")
    pattern_table.add_row("5", "Espiral")
    
    console.print(pattern_table)
    pattern = IntPrompt.ask("Selecciona patrón", default=1)

    console.print(f"[yellow]Spawneando {mob_count} {mobs_dict.get(mob_id, mob_id)} cerca de {player}...[/yellow]")

    if pattern == 1:  # Círculo
        radius = IntPrompt.ask("Radio del círculo", default=3)
        for i in range(mob_count):
            angle = (2 * math.pi * i) / mob_count
            x_offset = int(radius * math.cos(angle))
            z_offset = int(radius * math.sin(angle))
            command = f"execute at {player} run summon {mob_id} ~{x_offset} ~ ~{z_offset}"
            send_command(SCREEN_NAME, command)
            time.sleep(0.3)
    
    elif pattern == 2:  # Línea
        direction = Prompt.ask("Dirección (x/z)", default="x")
        spacing = IntPrompt.ask("Espaciado entre mobs", default=2)
        for i in range(mob_count):
            if direction.lower() == "x":
                command = f"execute at {player} run summon {mob_id} ~{i*spacing} ~ ~"
            else:
                command = f"execute at {player} run summon {mob_id} ~ ~ ~{i*spacing}"
            send_command(SCREEN_NAME, command)
            time.sleep(0.3)
    
    elif pattern == 3:  # Aleatorio
        area_size = IntPrompt.ask("Tamaño del área", default=10)
        for i in range(mob_count):
            x_offset = random.randint(-area_size, area_size)
            z_offset = random.randint(-area_size, area_size)
            command = f"execute at {player} run summon {mob_id} ~{x_offset} ~ ~{z_offset}"
            send_command(SCREEN_NAME, command)
            time.sleep(0.3)
    
    elif pattern == 4:  # Cuadrado/Grid
        grid_size = int(math.sqrt(mob_count)) + 1
        spacing = IntPrompt.ask("Espaciado entre mobs", default=2)
        count = 0
        for x in range(grid_size):
            for z in range(grid_size):
                if count >= mob_count:
                    break
                x_offset = (x - grid_size//2) * spacing
                z_offset = (z - grid_size//2) * spacing
                command = f"execute at {player} run summon {mob_id} ~{x_offset} ~ ~{z_offset}"
                send_command(SCREEN_NAME, command)
                count += 1
                time.sleep(0.3)
            if count >= mob_count:
                break
    
    elif pattern == 5:  # Espiral
        radius_increment = IntPrompt.ask("Incremento de radio", default=1)
        for i in range(mob_count):
            angle = i * 0.5
            radius = i * radius_increment / 5
            x_offset = int(radius * math.cos(angle))
            z_offset = int(radius * math.sin(angle))
            command = f"execute at {player} run summon {mob_id} ~{x_offset} ~ ~{z_offset}"
            send_command(SCREEN_NAME, command)
            time.sleep(0.3)

    console.print(f"[green]✅ {mob_count} mobs spawneados exitosamente![/green]")
def summon_mob_with_effects():
    """Summon mob con efectos especiales - FUNCIONALIDAD DEL VIEJO PANEL"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob usando el sistema mejorado
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return
    
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    player = select_player_from_list(players)
    if not player:
        return
    
    # Efectos especiales
    effects_table = Table(title="Efectos Especiales")
    effects_table.add_column("Opción", style="cyan")
    effects_table.add_column("Descripción", style="green")
    
    effects_table.add_row("1", "Mob gigante (NoAI)")
    effects_table.add_row("2", "Mob invisible")
    effects_table.add_row("3", "Mob con nombre personalizado")
    effects_table.add_row("4", "Mob con efectos de poción")
    effects_table.add_row("5", "Mob bebé")
    effects_table.add_row("6", "Mob con equipo personalizado")
    effects_table.add_row("7", "Mob normal")
    
    console.print(effects_table)
    effect_choice = IntPrompt.ask("Selecciona efecto", default=7)

    if effect_choice == 1:  # Gigante
        command = f"execute at {player} run summon {mob_id} ~ ~ ~ {{NoAI:1b,Silent:1b}}"
    elif effect_choice == 2:  # Invisible
        command = f"execute at {player} run summon {mob_id} ~ ~ ~ {{ActiveEffects:[{{Id:14,Amplifier:0,Duration:999999}}]}}"
    elif effect_choice == 3:  # Nombre personalizado
        custom_name = Prompt.ask("Nombre personalizado")
        command = f'execute at {player} run summon {mob_id} ~ ~ ~ {{CustomName:"{custom_name}",CustomNameVisible:1b}}'
    elif effect_choice == 4:  # Efectos de poción
        console.print("[yellow]Efectos disponibles: speed, strength, resistance, fire_resistance[/yellow]")
        effect_name = Prompt.ask("Nombre del efecto", default="strength")
        amplifier = IntPrompt.ask("Amplificador (0-10)", default=1)
        command = f"execute at {player} run summon {mob_id} ~ ~ ~ {{ActiveEffects:[{{Id:{effect_name},Amplifier:{amplifier},Duration:999999}}]}}"
    elif effect_choice == 5:  # Bebé
        command = f"execute at {player} run summon {mob_id} ~ ~ ~ {{IsBaby:1b}}"
    elif effect_choice == 6:  # Con equipo
        console.print("[yellow]Spawneando mob con equipo de diamante...[/yellow]")
        command = f'execute at {player} run summon {mob_id} ~ ~ ~ {{HandItems:[{{id:"minecraft:diamond_sword",Count:1b}},{{}}],ArmorItems:[{{id:"minecraft:diamond_boots",Count:1b}},{{id:"minecraft:diamond_leggings",Count:1b}},{{id:"minecraft:diamond_chestplate",Count:1b}},{{id:"minecraft:diamond_helmet",Count:1b}}]}}'
    else:  # Normal
        command = f"execute at {player} run summon {mob_id}"

    send_command(SCREEN_NAME, command)
    console.print(f"[green]✅ {mobs_dict.get(mob_id, mob_id)} con efectos spawneado en {player}[/green]")
def clear_specific_mobs():
    """Limpiar mobs específicos - FUNCIONALIDAD DEL VIEJO PANEL"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob usando el sistema mejorado
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return

    radius = IntPrompt.ask("Radio de limpieza (0 para todo el mundo)", default=0)
    
    if radius == 0:
        command = f"kill @e[type={mob_id}]"
        send_command(SCREEN_NAME, command)
        console.print(f"[green]Eliminados todos los {mobs_dict.get(mob_id, mob_id)} del mundo[/green]")
    else:
        players = get_connected_players(SCREEN_NAME)
        if not players:
            console.print("[red]No hay jugadores conectados para usar como centro.[/red]")
            return
        
        player = select_player_from_list(players)
        if player:
            command = f"execute at {player} run kill @e[type={mob_id},distance=..{radius}]"
            send_command(SCREEN_NAME, command)
            console.print(f"[green]Eliminados {mobs_dict.get(mob_id, mob_id)} en radio {radius} de {player}[/green]")
def summon_area_patterns():
    """Nueva función: Summon en área con patrones avanzados"""
    mobs_dict = load_mobs()
    
    # Seleccionar mob
    mob_id = select_item_or_mob(mobs_dict, "mob")
    if not mob_id:
        return
    
    # Configuración del área
    console.print("[cyan]Configuración del área de spawn:[/cyan]")
    center_x = IntPrompt.ask("Centro X")
    center_y = IntPrompt.ask("Centro Y", default=100)
    center_z = IntPrompt.ask("Centro Z")
    
    # Patrones avanzados
    pattern_table = Table(title="Patrones Avanzados de Área")
    pattern_table.add_column("Opción", style="cyan")
    pattern_table.add_column("Descripción", style="green")
    
    pattern_table.add_row("1", "Círculos concéntricos")
    pattern_table.add_row("2", "Cruz gigante")
    pattern_table.add_row("3", "Estrella de 8 puntas")
    pattern_table.add_row("4", "Lluvia aleatoria en área")
    pattern_table.add_row("5", "Patrón de ondas")
    
    console.print(pattern_table)
    pattern = IntPrompt.ask("Selecciona patrón", default=1)
    
    mob_count = IntPrompt.ask("Cantidad total de mobs", default=20)
    delay = IntPrompt.ask("Delay entre spawns (segundos)", default=0.5)
    
    console.print(f"[yellow]Iniciando spawn de {mob_count} {mobs_dict.get(mob_id, mob_id)}...[/yellow]")
    
    if pattern == 1:  # Círculos concéntricos
        circles = IntPrompt.ask("Número de círculos", default=3)
        mobs_per_circle = mob_count // circles
        
        for circle in range(circles):
            radius = (circle + 1) * 5
            for i in range(mobs_per_circle):
                angle = (2 * math.pi * i) / mobs_per_circle
                x = center_x + int(radius * math.cos(angle))
                z = center_z + int(radius * math.sin(angle))
                send_command(SCREEN_NAME, f"summon {mob_id} {x} {center_y} {z}")
                time.sleep(delay)
    
    elif pattern == 2:  # Cruz gigante
        arm_length = IntPrompt.ask("Longitud de brazos", default=10)
        spacing = IntPrompt.ask("Espaciado", default=2)
        
        # Brazo horizontal
        for i in range(-arm_length, arm_length + 1, spacing):
            send_command(SCREEN_NAME, f"summon {mob_id} {center_x + i} {center_y} {center_z}")
            time.sleep(delay)
        
        # Brazo vertical
        for i in range(-arm_length, arm_length + 1, spacing):
            if i != 0:  # Evitar duplicar el centro
                send_command(SCREEN_NAME, f"summon {mob_id} {center_x} {center_y} {center_z + i}")
                time.sleep(delay)
    
    elif pattern == 3:  # Estrella de 8 puntas
        radius = IntPrompt.ask("Radio de la estrella", default=8)
        points = 8
        
        for point in range(points):
            angle = (2 * math.pi * point) / points
            # Puntas principales
            x = center_x + int(radius * math.cos(angle))
            z = center_z + int(radius * math.sin(angle))
            send_command(SCREEN_NAME, f"summon {mob_id} {x} {center_y} {z}")
            
            # Puntas intermedias (más cortas)
            angle_mid = angle + (math.pi / points)
            x_mid = center_x + int((radius * 0.6) * math.cos(angle_mid))
            z_mid = center_z + int((radius * 0.6) * math.sin(angle_mid))
            send_command(SCREEN_NAME, f"summon {mob_id} {x_mid} {center_y} {z_mid}")
            time.sleep(delay)
    
    elif pattern == 4:  # Lluvia aleatoria
        area_radius = IntPrompt.ask("Radio del área", default=15)
        
        for i in range(mob_count):
            x = center_x + random.randint(-area_radius, area_radius)
            z = center_z + random.randint(-area_radius, area_radius)
            send_command(SCREEN_NAME, f"summon {mob_id} {x} {center_y} {z}")
            time.sleep(delay)
    
    elif pattern == 5:  # Ondas
        wave_count = IntPrompt.ask("Número de ondas", default=4)
        wave_radius = IntPrompt.ask("Radio máximo", default=12)
        
        for wave in range(wave_count):
            radius = ((wave + 1) * wave_radius) // wave_count
            points_in_wave = max(8, radius * 2)
            
            for point in range(points_in_wave):
                angle = (2 * math.pi * point) / points_in_wave
                x = center_x + int(radius * math.cos(angle))
                z = center_z + int(radius * math.sin(angle))
                send_command(SCREEN_NAME, f"summon {mob_id} {x} {center_y} {z}")
                time.sleep(delay * 0.5)
    
    console.print(f"[green]✅ Patrón completado! {mob_count} mobs spawneados[/green]")
#region tntalert
def tnt_alert_menu():
    """Menú TNT Alert con manejo seguro de errores"""
    # Verificar si el servidor está en ejecución
    if not is_server_running(SCREEN_NAME):
        console.print("⚠️ El servidor no está en ejecución. Inicia el servidor primero.", style="red")
        return
        
    table = Table(title="🔔 TNT Alert - Notificaciones")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Enviar notificación única")
    table.add_row("2", "Modo consola de mensajes (título fijo)")
    table.add_row("3", "Verificar jugadores conectados")
    table.add_row("4", "Probar detección de jugadores")
    table.add_row("0", "Volver al menú principal")

    console.print(table)
    
    try:
        choice = IntPrompt.ask("Selecciona una opción", default=0)
    except KeyboardInterrupt:
        console.print("\nOperación cancelada", style="yellow")
        return

    if choice == 1:
        try:
            # Caso 1: Enviar una sola notificación
            titulo = Prompt.ask("📝 Título de la notificación")
            mensaje = Prompt.ask("📝 Mensaje de la notificación")
            
            # Verificar que no estén vacíos
            if not titulo.strip() or not mensaje.strip():
                console.print("El título y el mensaje no pueden estar vacíos.", style="red")
                return
                
            # Enviar comando TNT Alert
            send_command(SCREEN_NAME, f"tntalert {titulo} {mensaje}")
            console.print(f"✅ Notificación enviada con título: {titulo}", style="green")
            console.print(f"   Mensaje: {mensaje}", style="green")
        except Exception as e:
            safe_print(f"Error al enviar notificación: {e}", "red")
        
    elif choice == 2:
        try:
            # Caso 2: Modo consola con título fijo
            titulo = Prompt.ask("📝 Título fijo para todas las notificaciones")
            
            # Verificar que el título no esté vacío
            if not titulo.strip():
                console.print("El título no puede estar vacío.", style="red")
                return
                
            console.print(f"🔔 Modo consola de mensajes activado con título: {titulo}", style="cyan")
            console.print("Escribe los mensajes a enviar. Para salir escribe: 'exit', 'quit', o 'salir'", style="yellow")
            console.print("Comandos especiales:", style="yellow")
            console.print("  - 'help': Mostrar ayuda", style="yellow")
            console.print("  - 'status': Ver estado del servidor", style="yellow")
            
            mensaje_count = 0
            
            # Bucle para enviar mensajes continuamente
            while True:
                try:
                    mensaje = Prompt.ask(f"📝 Mensaje #{mensaje_count + 1}")
                    
                    # Verificar comandos especiales
                    if mensaje.lower() in ['exit', 'quit', 'salir']:
                        console.print(f"Saliendo del modo consola de mensajes... Total de mensajes enviados: {mensaje_count}", style="yellow")
                        break
                    elif mensaje.lower() == 'help':
                        console.print("Comandos disponibles:", style="cyan")
                        console.print("  - exit/quit/salir: Salir del modo consola", style="cyan")
                        console.print("  - help: Mostrar esta ayuda", style="cyan")
                        console.print("  - status: Ver jugadores conectados", style="cyan")
                        continue
                    elif mensaje.lower() == 'status':
                        players = get_connected_players(SCREEN_NAME)
                        if players:
                            player_list = ', '.join(players)
                            console.print(f"Jugadores conectados: {len(players)} - {player_list}", style="cyan")
                        else:
                            console.print("Jugadores conectados: ninguno", style="cyan")
                        continue
                        
                    # Verificar que el mensaje no esté vacío
                    if not mensaje.strip():
                        console.print("El mensaje no puede estar vacío.", style="red")
                        continue
                        
                    # Enviar comando TNT Alert
                    send_command(SCREEN_NAME, f"tntalert {titulo} {mensaje}")
                    mensaje_count += 1
                    console.print(f"✅ Mensaje #{mensaje_count} enviado: {mensaje}", style="green")
                    
                except KeyboardInterrupt:
                    console.print(f"\nModo consola interrumpido. Total de mensajes enviados: {mensaje_count}", style="yellow")
                    break
                except Exception as e:
                    safe_print(f"Error al enviar mensaje: {e}", "red")
        except Exception as e:
            safe_print(f"Error en modo consola: {e}", "red")
    
    elif choice == 3:
        try:
            # Verificar jugadores conectados
            console.print("Verificando jugadores conectados...", style="yellow")
            players = get_connected_players(SCREEN_NAME)
            
            if players:
                players_table = Table(title="Jugadores Conectados")
                players_table.add_column("N°", style="cyan")
                players_table.add_column("Jugador", style="green")
                
                for i, player in enumerate(players, 1):
                    players_table.add_row(str(i), player)
                    
                console.print(players_table)
                console.print(f"Total: {len(players)} jugadores conectados", style="green")
            else:
                console.print("No hay jugadores conectados actualmente.", style="yellow")
        except Exception as e:
            safe_print(f"Error al verificar jugadores: {e}", "red")
    
    elif choice == 4:
        try:
            # Probar detección de jugadores
            console.print("🧪 Probando detección de jugadores...", style="yellow")
            
            # Simular el contenido que tienes
            test_content = "[16:33:30 INFO]: There are 1 of a max of 20 players online: JaimeJ99"
            
            # Probar el patrón
            pattern = r"\[.*?\]: There are \d+ of a max of \d+ players online:(.*)"
            matches = re.findall(pattern, test_content)
            
            if matches:
                players_str = matches[0].strip()
                console.print(f"✅ Patrón funciona. Jugadores encontrados: '{players_str}'", style="green")
                
                if players_str:
                    players = [p.strip() for p in players_str.split(",") if p.strip()]
                    console.print(f"✅ Lista de jugadores: {players}", style="green")
                else:
                    console.print("⚠️ Cadena de jugadores vacía", style="yellow")
            else:
                console.print("❌ El patrón no coincide", style="red")
                
            console.print("Ahora probando con el servidor real...", style="yellow")
            players = get_connected_players(SCREEN_NAME)
            console.print(f"Resultado final: {len(players)} jugadores - {players}", style="cyan")
        except Exception as e:
            safe_print(f"Error en prueba de detección: {e}", "red")
def tnt_alert_menu():
    """Menú para el plugin TNT Alert que permite enviar notificaciones llamativas"""
    table = Table(title="🔔 TNT Alert - Notificaciones")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Enviar notificación única")
    table.add_row("2", "Modo consola de mensajes (título fijo)")
    table.add_row("0", "Volver al menú principal")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción", default=0)

    if choice == 1:
        # Caso 1: Enviar una sola notificación
        titulo = Prompt.ask("📝 Título de la notificación")
        mensaje = Prompt.ask("📝 Mensaje de la notificación")
        
        # Verificar que no estén vacíos
        if not titulo.strip() or not mensaje.strip():
            console.print("[red]El título y el mensaje no pueden estar vacíos.[/red]")
            return
            
        # Enviar comando TNT Alert
        send_command(SCREEN_NAME, f"tntalert {titulo} {mensaje}")
        console.print(f"[green]✅ Notificación enviada con título: [bold]{titulo}[/bold][/green]")
        
    elif choice == 2:
        # Caso 2: Modo consola con título fijo
        titulo = Prompt.ask("📝 Título fijo para todas las notificaciones")
        
        # Verificar que el título no esté vacío
        if not titulo.strip():
            console.print("[red]El título no puede estar vacío.[/red]")
            return
            
        console.print(f"[cyan]🔔 Modo consola de mensajes activado con título: [bold]{titulo}[/bold][/cyan]")
        console.print("[yellow]Escribe los mensajes a enviar. Para salir escribe: 'exit', 'quit', o 'salir'[/yellow]")
        
        # Bucle para enviar mensajes continuamente
        while True:
            mensaje = Prompt.ask("📝 Mensaje")
            
            # Verificar comandos de salida
            if mensaje.lower() in ['exit', 'quit', 'salir']:
                console.print("[yellow]Saliendo del modo consola de mensajes...[/yellow]")
                break
                
            # Verificar que el mensaje no esté vacío
            if not mensaje.strip():
                console.print("[red]El mensaje no puede estar vacío.[/red]")
                continue
                
            # Enviar comando TNT Alert
            send_command(SCREEN_NAME, f"tntalert {titulo} {mensaje}")
            console.print(f"[green]✅ Mensaje enviado: [bold]{mensaje}[/bold][/green]")

#endregion
#endregion

#region jugadores
# ==================== GESTIÓN DE JUGADORES ====================
def player_management_menu():
    while True:
        table = Table(title="Gestión de Jugadores")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        table.add_row("1", "Ver jugadores conectados")
        table.add_row("2", "Kickear jugador")
        table.add_row("3", "Banear jugador")
        table.add_row("4", "Desbanear jugador")
        table.add_row("5", "Agregar a whitelist")
        table.add_row("6", "Remover de whitelist")
        table.add_row("7", "Ver whitelist")
        table.add_row("8", "Dar OP a jugador")
        table.add_row("9", "Quitar OP a jugador")
        table.add_row("10", "Ver lista de OPs")
        table.add_row("11", "Activar whitelist")
        table.add_row("12", "Desactivar whitelist")
        table.add_row("13", "Ver estado de whitelist")
        table.add_row("0", "Volver al menú anterior")

        console.print(table)
        choice = IntPrompt.ask("Selecciona una opción", default=0)

        if choice == 0:
            break

        elif choice == 1:
            players = get_connected_players(SCREEN_NAME)
            if players:
                players_table = Table(title="Jugadores Conectados")
                players_table.add_column("Jugador", style="green")
                for player in players:
                    players_table.add_row(player)
                console.print(players_table)
            else:
                console.print("[yellow]No hay jugadores conectados.[/yellow]")

        elif choice == 2:
            player = Prompt.ask("Nombre del jugador a kickear")
            reason = Prompt.ask("Razón (opcional)", default="")
            cmd = f"kick {player} {reason}" if reason else f"kick {player}"
            send_command(SCREEN_NAME, cmd)

        elif choice == 3:
            player = Prompt.ask("Nombre del jugador a banear")
            reason = Prompt.ask("Razón (opcional)", default="")
            cmd = f"ban {player} {reason}" if reason else f"ban {player}"
            send_command(SCREEN_NAME, cmd)

        elif choice == 4:
            player = Prompt.ask("Nombre del jugador a desbanear")
            send_command(SCREEN_NAME, f"pardon {player}")

        elif choice == 5:
            player = Prompt.ask("Nombre del jugador para whitelist")
            send_command(SCREEN_NAME, f"whitelist add {player}")

        elif choice == 6:
            player = Prompt.ask("Nombre del jugador para remover de whitelist")
            send_command(SCREEN_NAME, f"whitelist remove {player}")

        elif choice == 7:
            send_command(SCREEN_NAME, "whitelist list")

        elif choice == 8:
            player = Prompt.ask("Nombre del jugador para dar OP")
            send_command(SCREEN_NAME, f"op {player}")

        elif choice == 9:
            player = Prompt.ask("Nombre del jugador para quitar OP")
            send_command(SCREEN_NAME, f"deop {player}")

        elif choice == 10:
            ops_file = os.path.join(SERVER_PATH, "ops.json")
            if os.path.exists(ops_file):
                try:
                    with open(ops_file, 'r') as f:
                        ops = json.load(f)
                    ops_table = Table(title="Lista de OPs")
                    ops_table.add_column("Jugador", style="green")
                    ops_table.add_column("Nivel", style="cyan")
                    for op in ops:
                        ops_table.add_row(op.get('name', 'N/A'), str(op.get('level', 'N/A')))
                    console.print(ops_table)
                except:
                    console.print("[red]Error al leer ops.json[/red]")
            else:
                console.print("[yellow]Archivo ops.json no encontrado[/yellow]")

        elif choice == 11:
            send_command(SCREEN_NAME, "whitelist on")
            console.print("[green]Whitelist activada[/green]")

        elif choice == 12:
            send_command(SCREEN_NAME, "whitelist off")
            console.print("[green]Whitelist desactivada[/green]")

        elif choice == 13:
            # Ejecutar el comando whitelist para ver su estado
            output = send_command(SCREEN_NAME, "whitelist")  # Este comando devuelve algo como "Whitelist: on"
            if output:
                console.print(f"[cyan]Estado actual de la whitelist:[/] {output.strip()}")
            else:
                console.print("[red]No se pudo obtener el estado de la whitelist[/red]")

        else:
            console.print("[red]Opción no válida[/red]")
def get_connected_players(screen_name):
    """Obtiene la lista de jugadores conectados en una sesión de screen de Minecraft."""
    import os
    import re
    import subprocess
    import time

    temp_file = "/tmp/screen_output.txt"
    send_command(screen_name, "list")
    time.sleep(3)

    hardcopy_cmd = f'screen -S {screen_name} -X hardcopy {temp_file}'
    subprocess.run(hardcopy_cmd, shell=True)

    if not os.path.isfile(temp_file):
        return []

    players = []
    with open(temp_file, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    pattern = r"\[.*?\]: There are \d+ of a max of \d+ players online:(.*)"
    matches = re.findall(pattern, content)

    if matches:
        players_str = matches[-1].strip()
        if players_str:
            players = [p.strip() for p in players_str.split(",") if p.strip()]
            players = [p for p in players if p and not p.isspace()]

    no_players_pattern = r"\[.*?\]: There are 0 of a max of \d+ players online:"
    if re.search(no_players_pattern, content):
        players = []

    if os.path.exists(temp_file):
        os.remove(temp_file)

    return players
def select_player_from_list(players):
    """Función auxiliar para seleccionar un jugador de la lista"""
    if not players:
        return None
        
    players_table = Table(title="Seleccionar Jugador")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    
    console.print(players_table)
    
    choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if choice == 0:
        return None
    if 1 <= choice <= len(players):
        return players[choice - 1]
    else:
        console.print("[red]Selección inválida[/red]")
        return None
#endregion

#region plugins/mods
# ==================== GESTIÓN DE PLUGINS/MODS ====================
def plugin_management_menu():
  while True:
        plugins_dir = os.path.join(SERVER_PATH, "plugins")
        mods_dir = os.path.join(SERVER_PATH, "mods")
        
        table = Table(title="Gestión de Plugins/Mods")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        table.add_row("1", "Listar plugins")
        table.add_row("2", "Listar mods")
        table.add_row("3", "Recargar plugins")
        table.add_row("4", "Información de plugin específico")
        table.add_row("5", "Habilitar/Deshabilitar plugin")
        table.add_row("0", "Rgresar")

        console.print(table)
        choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

        if choice == 0:
            break
        elif choice == 1:
            if os.path.exists(plugins_dir):
                plugins = [f for f in os.listdir(plugins_dir) if f.endswith('.jar')]
                if plugins:
                    plugins_table = Table(title="Plugins Instalados")
                    plugins_table.add_column("Plugin", style="green")
                    plugins_table.add_column("Tamaño", style="cyan")
                    for plugin in plugins:
                        size = os.path.getsize(os.path.join(plugins_dir, plugin))
                        size_mb = round(size / 1024 / 1024, 2)
                        plugins_table.add_row(plugin, f"{size_mb} MB")
                    console.print(plugins_table)
                else:
                    console.print("[yellow]No se encontraron plugins.[/yellow]")
            else:
                console.print("[red]Directorio de plugins no encontrado.[/red]")
        
        elif choice == 2:
            if os.path.exists(mods_dir):
                mods = [f for f in os.listdir(mods_dir) if f.endswith('.jar')]
                if mods:
                    mods_table = Table(title="Mods Instalados")
                    mods_table.add_column("Mod", style="green")
                    mods_table.add_column("Tamaño", style="cyan")
                    for mod in mods:
                        size = os.path.getsize(os.path.join(mods_dir, mod))
                        size_mb = round(size / 1024 / 1024, 2)
                        mods_table.add_row(mod, f"{size_mb} MB")
                    console.print(mods_table)
                else:
                    console.print("[yellow]No se encontraron mods.[/yellow]")
            else:
                console.print("[red]Directorio de mods no encontrado.[/red]")
        
        elif choice == 3:
            send_command(SCREEN_NAME, "reload")
            console.print("[green]Comando de recarga enviado.[/green]")
        
        elif choice == 4:
            plugin_name = Prompt.ask("Nombre del plugin")
            send_command(SCREEN_NAME, f"pl {plugin_name}")
        
        elif choice == 5:
            plugin_name = Prompt.ask("Nombre del plugin")
            action = Prompt.ask("¿Habilitar o deshabilitar? (enable/disable)")
            if action.lower() in ['enable', 'disable']:
                send_command(SCREEN_NAME, f"plugman {action} {plugin_name}")
            else:
                console.print("[red]Acción inválida. Usa 'enable' o 'disable'.[/red]")
#endregion

#region backups
# ==================== GESTIÓN DE BACKUPS ====================
def backup_management_menu():
    global BACKUP_PATH
    while True:
        table = Table(title="Gestión de Backups")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        table.add_row("1", "Crear backup completo")
        table.add_row("2", "Crear backup solo del mundo")
        table.add_row("3", "Listar backups")
        table.add_row("4", "Eliminar backup")
        table.add_row("5", "Configurar ruta de backups")
        table.add_row("0", "Regresar")

        console.print(table)
        choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

        if choice == 0:
            break
        elif choice == 1:
            create_full_backup()
        elif choice == 2:
            create_world_backup()
        elif choice == 3:
            list_backups()
        elif choice == 4:
            delete_backup()
        elif choice == 5:
            BACKUP_PATH = Prompt.ask("Nueva ruta para backups", default=BACKUP_PATH)
            if not os.path.exists(BACKUP_PATH):
                os.makedirs(BACKUP_PATH)
            console.print(f"[green]Ruta de backups configurada: {BACKUP_PATH}[/green]")
def create_full_backup():
    if not BACKUP_PATH:
        console.print("[red]Configura primero la ruta de backups.[/red]")
        return
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_name = f"full_backup_{timestamp}.zip"
    backup_file = os.path.join(BACKUP_PATH, backup_name)
    
    console.print("[yellow]Creando backup completo...[/yellow]")
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console,
    ) as progress:
        task = progress.add_task("Comprimiendo archivos...", total=None)
        
        with zipfile.ZipFile(backup_file, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for root, dirs, files in os.walk(SERVER_PATH):
                dirs[:] = [d for d in dirs if d not in ['logs', 'crash-reports', 'debug']]
                for file in files:
                    if not file.endswith(('.log', '.tmp')):
                        file_path = os.path.join(root, file)
                        arcname = os.path.relpath(file_path, SERVER_PATH)
                        zipf.write(file_path, arcname)
    
    console.print(f"[green]Backup completo creado: {backup_name}[/green]")
def create_world_backup():
    if not BACKUP_PATH:
        console.print("[red]Configura primero la ruta de backups.[/red]")
        return
    
    world_dirs = ['world', 'world_nether', 'world_the_end']
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_name = f"world_backup_{timestamp}.zip"
    backup_file = os.path.join(BACKUP_PATH, backup_name)
    
    console.print("[yellow]Creando backup del mundo...[/yellow]")
    
    with zipfile.ZipFile(backup_file, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for world_dir in world_dirs:
            world_path = os.path.join(SERVER_PATH, world_dir)
            if os.path.exists(world_path):
                for root, dirs, files in os.walk(world_path):
                    for file in files:
                        file_path = os.path.join(root, file)
                        arcname = os.path.relpath(file_path, SERVER_PATH)
                        zipf.write(file_path, arcname)
    
    console.print(f"[green]Backup del mundo creado: {backup_name}[/green]")
def list_backups():
    if not BACKUP_PATH or not os.path.exists(BACKUP_PATH):
        console.print("[red]No hay ruta de backups configurada o no existe.[/red]")
        return
    
    backups = [f for f in os.listdir(BACKUP_PATH) if f.endswith('.zip')]
    if not backups:
        console.print("[yellow]No se encontraron backups.[/yellow]")
        return
    
    backups_table = Table(title="Backups Disponibles")
    backups_table.add_column("Archivo", style="green")
    backups_table.add_column("Tamaño", style="cyan")
    backups_table.add_column("Fecha", style="yellow")
    
    for backup in sorted(backups, reverse=True):
        backup_path = os.path.join(BACKUP_PATH, backup)
        size = os.path.getsize(backup_path)
        size_mb = round(size / 1024 / 1024, 2)
        mtime = os.path.getmtime(backup_path)
        date = datetime.fromtimestamp(mtime).strftime("%Y-%m-%d %H:%M:%S")
        backups_table.add_row(backup, f"{size_mb} MB", date)
    
    console.print(backups_table)
def delete_backup():
    if not BACKUP_PATH or not os.path.exists(BACKUP_PATH):
        console.print("[red]No hay ruta de backups configurada.[/red]")
        return
    
    backups = [f for f in os.listdir(BACKUP_PATH) if f.endswith('.zip')]
    if not backups:
        console.print("[yellow]No se encontraron backups.[/yellow]")
        return
    
    backups_table = Table(title="Backups para Eliminar")
    backups_table.add_column("N°", style="cyan")
    backups_table.add_column("Archivo", style="green")
    
    for i, backup in enumerate(sorted(backups, reverse=True), 1):
        backups_table.add_row(str(i), backup)
    
    console.print(backups_table)
    
    choice = IntPrompt.ask("Selecciona backup a eliminar (0 para cancelar)", default=0)
    if choice == 0:
        return
    
    if 1 <= choice <= len(backups):
        backup_to_delete = sorted(backups, reverse=True)[choice - 1]
        if Confirm.ask(f"¿Estás seguro de eliminar {backup_to_delete}?"):
            backup_path = os.path.join(BACKUP_PATH, backup_to_delete)
            os.remove(backup_path)
            console.print(f"[green]Backup {backup_to_delete} eliminado.[/green]")
    else:
        console.print("[red]Selección inválida.[/red]")
#endregion

#region mundos
# ==================== GESTIÓN DE MUNDOS ====================
def world_management_menu():
    """Menú mejorado de gestión de mundos con captura de salida"""
    while True:
        table = Table(title="🌍 Gestión de Mundos Mejorada")
        table.add_column("Opción", style="cyan")
        table.add_column("Descripción", style="green")

        table.add_row("1", "📊 Información completa del mundo")
        table.add_row("2", "🏠 Cambiar spawn del mundo")
        table.add_row("3", "🧹 Limpiar entidades")
        table.add_row("4", "💾 Guardar mundo")
        table.add_row("5", "⚙️ Desactivar/Activar guardado automático")
        table.add_row("6", "🌱 Solo mostrar semilla")
        table.add_row("7", "🗺️ Solo mostrar borde del mundo")
        table.add_row("8", "⏰ Gestión de tiempo")
        table.add_row("9", "🌤️ Gestión de clima")
        table.add_row("0", "⬅️ Regresar")

        console.print(table)
        choice = IntPrompt.ask("Selecciona una opción", default=0)

        if choice == 0:
            break
        elif choice == 1:
            show_world_info()
        elif choice == 2:
            x = IntPrompt.ask("Coordenada X")
            y = IntPrompt.ask("Coordenada Y")
            z = IntPrompt.ask("Coordenada Z")
            send_command(SCREEN_NAME, f"setworldspawn {x} {y} {z}")
            console.print(f"[green]✅ Spawn del mundo cambiado a: {x}, {y}, {z}[/green]")
        elif choice == 3:
            clean_entities_menu()
        elif choice == 4:
            send_command(SCREEN_NAME, "save-all")
            console.print("[green]✅ Comando de guardado enviado[/green]")
            
            # Mostrar resultado del guardado
            if Prompt.ask("¿Mostrar resultado del guardado? (s/n)", default="s").lower() == "s":
                save_output = get_command_output(SCREEN_NAME, "save-all", 3)
                console.print(Panel.fit(save_output, title="💾 Resultado del Guardado", border_style="green"))
        elif choice == 5:
            toggle_autosave()
        elif choice == 6:
            # Solo mostrar semilla
            console.print("[cyan]🌱 Obteniendo semilla...[/cyan]")
            seed_output = get_command_output(SCREEN_NAME, "seed", 2)
            seed = parse_seed_output(seed_output)
            console.print(f"[green]🌱 Semilla del mundo: [bold]{seed}[/bold][/green]")
        elif choice == 7:
            # Solo mostrar borde del mundo
            console.print("[cyan]🗺️ Obteniendo información del borde...[/cyan]")
            border_output = get_command_output(SCREEN_NAME, "worldborder get", 2)
            border_size = parse_worldborder_output(border_output)
            console.print(f"[green]🗺️ Tamaño del borde: [bold]{border_size}[/bold][/green]")
        elif choice == 8:
            time_management_menu()
        elif choice == 9:
            weather_management_menu()
def clean_entities_menu():
    """Menú mejorado para limpiar entidades"""
    table = Table(title="🧹 Limpiar Entidades")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")
    
    table.add_row("1", "🔥 Todas las entidades (excepto jugadores)")
    table.add_row("2", "🐄 Solo animales")
    table.add_row("3", "👹 Solo monstruos")
    table.add_row("4", "📦 Solo items en el suelo")
    table.add_row("5", "🎯 Tipo específico de entidad")
    table.add_row("0", "⬅️ Volver")
    
    console.print(table)
    choice = IntPrompt.ask("Selecciona qué limpiar", default=0)
    
    if choice == 0:
        return
    elif choice == 1:
        send_command(SCREEN_NAME, "kill @e[type=!player]")
        console.print("[green]✅ Comando para eliminar todas las entidades enviado[/green]")
    elif choice == 2:
        # Lista de animales comunes
        animals = ["cow", "pig", "sheep", "chicken", "horse", "donkey", "mule", "llama"]
        for animal in animals:
            send_command(SCREEN_NAME, f"kill @e[type=minecraft:{animal}]")
        console.print("[green]✅ Comandos para eliminar animales enviados[/green]")
    elif choice == 3:
        # Lista de monstruos comunes
        monsters = ["zombie", "skeleton", "creeper", "spider", "enderman", "witch"]
        for monster in monsters:
            send_command(SCREEN_NAME, f"kill @e[type=minecraft:{monster}]")
        console.print("[green]✅ Comandos para eliminar monstruos enviados[/green]")
    elif choice == 4:
        send_command(SCREEN_NAME, "kill @e[type=item]")
        console.print("[green]✅ Comando para eliminar items en el suelo enviado[/green]")
    elif choice == 5:
        entity_type = Prompt.ask("Tipo de entidad (ej: minecraft:zombie)")
        send_command(SCREEN_NAME, f"kill @e[type={entity_type}]")
        console.print(f"[green]✅ Comando para eliminar {entity_type} enviado[/green]")
def toggle_autosave():
    """Menú para activar/desactivar guardado automático"""
    table = Table(title="⚙️ Guardado Automático")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")
    
    table.add_row("1", "❌ Desactivar guardado automático")
    table.add_row("2", "✅ Activar guardado automático")
    table.add_row("3", "📊 Verificar estado actual")
    
    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción", default=3)
    
    if choice == 1:
        send_command(SCREEN_NAME, "save-off")
        console.print("[yellow]⚠️ Guardado automático desactivado[/yellow]")
    elif choice == 2:
        send_command(SCREEN_NAME, "save-on")
        console.print("[green]✅ Guardado automático activado[/green]")
    elif choice == 3:
        # Verificar estado (esto requeriría capturar la salida)
        console.print("[cyan]📊 Verificando estado del guardado automático...[/cyan]")
        output = get_command_output(SCREEN_NAME, "save-query", 2)
        console.print(Panel.fit(output, title="📊 Estado del Guardado", border_style="cyan"))
def time_management_menu():
    """Menú para gestión de tiempo del mundo"""
    table = Table(title="⏰ Gestión de Tiempo")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")
    
    table.add_row("1", "☀️ Establecer día")
    table.add_row("2", "🌙 Establecer noche")
    table.add_row("3", "🌅 Establecer amanecer")
    table.add_row("4", "🌇 Establecer atardecer")
    table.add_row("5", "⏰ Tiempo personalizado")
    table.add_row("6", "📊 Consultar tiempo actual")
    table.add_row("7", "⏸️ Detener ciclo día/noche")
    table.add_row("8", "▶️ Activar ciclo día/noche")
    
    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción", default=0)
    
    if choice == 1:
        send_command(SCREEN_NAME, "time set day")
        console.print("[green]☀️ Tiempo establecido a día[/green]")
    elif choice == 2:
        send_command(SCREEN_NAME, "time set night")
        console.print("[green]🌙 Tiempo establecido a noche[/green]")
    elif choice == 3:
        send_command(SCREEN_NAME, "time set 0")
        console.print("[green]🌅 Tiempo establecido a amanecer[/green]")
    elif choice == 4:
        send_command(SCREEN_NAME, "time set 12000")
        console.print("[green]🌇 Tiempo establecido a atardecer[/green]")
    elif choice == 5:
        time_value = IntPrompt.ask("Valor de tiempo (0-24000)")
        send_command(SCREEN_NAME, f"time set {time_value}")
        console.print(f"[green]⏰ Tiempo establecido a {time_value}[/green]")
    elif choice == 6:
        console.print("[cyan]📊 Consultando tiempo actual...[/cyan]")
        time_output = get_command_output(SCREEN_NAME, "time query daytime", 2)
        console.print(Panel.fit(time_output, title="⏰ Tiempo Actual", border_style="cyan"))
    elif choice == 7:
        send_command(SCREEN_NAME, "gamerule doDaylightCycle false")
        console.print("[yellow]⏸️ Ciclo día/noche detenido[/yellow]")
    elif choice == 8:
        send_command(SCREEN_NAME, "gamerule doDaylightCycle true")
        console.print("[green]▶️ Ciclo día/noche activado[/green]")
def weather_management_menu():
    """Menú para gestión del clima"""
    table = Table(title="🌤️ Gestión de Clima")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")
    
    table.add_row("1", "☀️ Clima despejado")
    table.add_row("2", "🌧️ Activar lluvia")
    table.add_row("3", "⛈️ Activar tormenta")
    table.add_row("4", "📊 Consultar clima actual")
    table.add_row("5", "⏰ Clima con duración personalizada")
    
    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción", default=0)
    
    if choice == 1:
        send_command(SCREEN_NAME, "weather clear")
        console.print("[green]☀️ Clima establecido a despejado[/green]")
    elif choice == 2:
        send_command(SCREEN_NAME, "weather rain")
        console.print("[green]🌧️ Lluvia activada[/green]")
    elif choice == 3:
        send_command(SCREEN_NAME, "weather thunder")
        console.print("[green]⛈️ Tormenta activada[/green]")
    elif choice == 4:
        console.print("[cyan]📊 Consultando clima actual...[/cyan]")
        weather_output = get_command_output(SCREEN_NAME, "weather query", 2)
        console.print(Panel.fit(weather_output, title="🌤️ Clima Actual", border_style="cyan"))
    elif choice == 5:
        weather_type = Prompt.ask("Tipo de clima (clear/rain/thunder)", default="clear")
        duration = IntPrompt.ask("Duración en segundos", default=600)
        send_command(SCREEN_NAME, f"weather {weather_type} {duration}")
        console.print(f"[green]🌤️ Clima {weather_type} establecido por {duration} segundos[/green]")
def show_world_info():
    """Muestra información detallada del mundo"""
    console.print("[yellow]📊 Obteniendo información del mundo...[/yellow]")
    
    # Obtener información de la semilla
    console.print("[cyan]🌱 Obteniendo semilla del mundo...[/cyan]")
    seed_output = get_command_output(SCREEN_NAME, "seed", 2)
    seed = parse_seed_output(seed_output)
    
    # Obtener información del borde del mundo
    console.print("[cyan]🗺️ Obteniendo información del borde del mundo...[/cyan]")
    border_output = get_command_output(SCREEN_NAME, "worldborder get", 2)
    border_size = parse_worldborder_output(border_output)
    
    # Obtener información adicional del mundo
    console.print("[cyan]⏰ Obteniendo tiempo del mundo...[/cyan]")
    time_output = get_command_output(SCREEN_NAME, "time query daytime", 2)
    
    console.print("[cyan]🌤️ Obteniendo clima del mundo...[/cyan]")
    weather_output = get_command_output(SCREEN_NAME, "weather query", 2)
    
    console.print("[cyan]⚙️ Obteniendo dificultad del mundo...[/cyan]")
    difficulty_output = get_command_output(SCREEN_NAME, "difficulty", 2)
    
    # Crear tabla con la información
    info_table = Table(title="🌍 Información del Mundo")
    info_table.add_column("Propiedad", style="cyan", no_wrap=True)
    info_table.add_column("Valor", style="green")
    info_table.add_column("Detalles", style="yellow")
    
    info_table.add_row("🌱 Semilla", seed, "Semilla única del mundo")
    info_table.add_row("🗺️ Borde del Mundo", border_size, "Tamaño actual del borde")
    
    # Parsear tiempo del mundo
    time_match = re.search(r"The time is (\d+)", time_output)
    if time_match:
        game_time = int(time_match.group(1))
        time_of_day = "Día" if 0 <= (game_time % 24000) < 12000 else "Noche"
        info_table.add_row("⏰ Tiempo", str(game_time), f"Hora del día: {time_of_day}")
    else:
        info_table.add_row("⏰ Tiempo", "No disponible", "No se pudo obtener")
    
    # Parsear clima
    if "clear" in weather_output.lower():
        weather_status = "☀️ Despejado"
    elif "rain" in weather_output.lower():
        weather_status = "🌧️ Lluvia"
    elif "thunder" in weather_output.lower():
        weather_status = "⛈️ Tormenta"
    else:
        weather_status = "❓ Desconocido"
    
    info_table.add_row("🌤️ Clima", weather_status, "Estado actual del clima")
    
    # Parsear dificultad
    difficulty_match = re.search(r"The difficulty is (\w+)", difficulty_output)
    if difficulty_match:
        difficulty = difficulty_match.group(1).title()
        difficulty_icons = {
            "Peaceful": "😇",
            "Easy": "😊",
            "Normal": "😐",
            "Hard": "😈"
        }
        icon = difficulty_icons.get(difficulty, "❓")
        info_table.add_row("⚔️ Dificultad", f"{icon} {difficulty}", "Nivel de dificultad actual")
    else:
        info_table.add_row("⚔️ Dificultad", "No disponible", "No se pudo obtener")
    
    console.print(info_table)
    
    # Mostrar salida completa en un panel colapsable
    if Prompt.ask("¿Mostrar salida completa de comandos? (s/n)", default="n").lower() == "s":
        output_panel = Panel.fit(
            f"[bold]Salida de 'seed':[/bold]\n{seed_output}\n\n"
            f"[bold]Salida de 'worldborder get':[/bold]\n{border_output}\n\n"
            f"[bold]Salida de 'time query':[/bold]\n{time_output}\n\n"
            f"[bold]Salida de 'weather query':[/bold]\n{weather_output}\n\n"
            f"[bold]Salida de 'difficulty':[/bold]\n{difficulty_output}",
            title="📋 Salida Completa de Comandos",
            border_style="blue"
        )
        console.print(output_panel)
def get_command_output(screen_name, command, wait_time=3):
    """
    Ejecuta un comando y captura la salida del servidor
    """
    temp_file = "/tmp/minecraft_output.txt"
    
    # Enviar el comando
    send_command(screen_name, command)
    
    # Esperar a que se ejecute
    time.sleep(wait_time)
    
    # Capturar la salida de la screen
    hardcopy_cmd = f'screen -S {screen_name} -X hardcopy {temp_file}'
    subprocess.run(hardcopy_cmd, shell=True)
    
    if not os.path.isfile(temp_file):
        return "No se pudo capturar la salida"
    
    try:
        with open(temp_file, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        
        # Limpiar el archivo temporal
        os.remove(temp_file)
        
        return content
    except Exception as e:
        return f"Error al leer la salida: {e}"
def parse_seed_output(output):
    """Extrae la información de la semilla del output"""
    seed_pattern = r"Seed: \[(-?\d+)\]"
    match = re.search(seed_pattern, output)
    if match:
        return match.group(1)
    return "No encontrada"
def parse_worldborder_output(output):
    """Extrae la información del borde del mundo del output"""
    border_patterns = [
        r"The world border is currently (\d+(?:\.\d+)?) block\(s\) wide",  # Para tu salida actual
        r"The world border is currently (\d+(?:\.\d+)? blocks? wide",      # Para otras posibles variantes
        r"World border is currently (\d+(?:\.\d+)? blocks? wide"          # Otra posible variante
    ]
    
    for pattern in border_patterns:
        match = re.search(pattern, output)
        if match:
            return f"{match.group(1)} bloques"
    
    return "No encontrado"
#endregion

#region monitor de rendimiento
# ==================== MONITOR DE RENDIMIENTO ====================
def performance_monitor():
    layout = Layout()
    
    def make_status():
        running = is_server_running(SCREEN_NAME)
        ram = get_ram_usage(SCREEN_NAME)
        cpu = get_cpu_usage(SCREEN_NAME)
        
        status_table = Table(title="Monitor de Rendimiento")
        status_table.add_column("Métrica", style="cyan")
        status_table.add_column("Valor", style="green")
        
        status_table.add_row("Estado", "🟢 Encendido" if running else "🔴 Apagado")
        status_table.add_row("RAM", f"{ram} MB" if ram else "N/A")
        status_table.add_row("CPU", f"{cpu}%" if cpu else "N/A")
        
        system_ram = psutil.virtual_memory()
        system_cpu = psutil.cpu_percent(interval=1)
        
        status_table.add_row("RAM Sistema", f"{round(system_ram.used/1024/1024/1024, 2)} GB / {round(system_ram.total/1024/1024/1024, 2)} GB")
        status_table.add_row("CPU Sistema", f"{system_cpu}%")
        
        return status_table
    
    with Live(make_status(), refresh_per_second=2, console=console) as live:
        console.print("Presiona Ctrl+C para salir del monitor")
        try:
            while True:
                time.sleep(1)
                live.update(make_status())
        except KeyboardInterrupt:
            console.print("\n[yellow]Saliendo del monitor...[/yellow]")
#endregion

#region logs
# ==================== VISUALIZACIÓN DE LOGS ====================
def view_logs():
    logs_dir = os.path.join(SERVER_PATH, "logs")
    if not os.path.exists(logs_dir):
        console.print("[red]Directorio de logs no encontrado.[/red]")
        return
    
    log_files = [f for f in os.listdir(logs_dir) if f.endswith('.log') or f.endswith('.gz')]
    if not log_files:
        console.print("[yellow]No se encontraron archivos de log.[/yellow]")
        return
    
    table = Table(title="Archivos de Log")
    table.add_column("N°", style="cyan")
    table.add_column("Archivo", style="green")
    table.add_column("Tamaño", style="yellow")
    
    for i, log_file in enumerate(sorted(log_files, reverse=True), 1):
        log_path = os.path.join(logs_dir, log_file)
        size = os.path.getsize(log_path)
        size_kb = round(size / 1024, 2)
        table.add_row(str(i), log_file, f"{size_kb} KB")
    
    console.print(table)
    
    choice = IntPrompt.ask("Selecciona un log para ver (0 para cancelar)", default=0)
    if choice == 0:
        return
    
    if 1 <= choice <= len(log_files):
        log_file = sorted(log_files, reverse=True)[choice - 1]
        log_path = os.path.join(logs_dir, log_file)
        
        lines = IntPrompt.ask("¿Cuántas líneas mostrar?", default=50)
        console.print(f"[yellow]Mostrando últimas {lines} líneas de {log_file}:[/yellow]")
        
        try:
            if log_file.endswith('.gz'):
                import gzip
                with gzip.open(log_path, 'rt', encoding='utf-8', errors='ignore') as f:
                    content = f.readlines()
            else:
                with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.readlines()
            
            for line in content[-lines:]:
                console.print(line.strip())
        except Exception as e:
            console.print(f"[red]Error al leer el archivo: {e}[/red]")
#endregion

#region configuraciones
# ==================== GESTIÓN DE CONFIGURACIONES ====================
def list_config_files():
    base_dirs = ["plugins", "config", "mods", "."]
    exts = (".yml", ".yaml", ".json", ".toml", ".properties", ".cfg", ".conf")

    found_files = []
    for base in base_dirs:
        path = os.path.join(SERVER_PATH, base)
        if not os.path.isdir(path):
            continue
        for root, _, files in os.walk(path):
            for file in files:
                if file.endswith(exts):
                    full_path = os.path.join(root, file)
                    found_files.append(full_path)
    return found_files
def paginated_config_menu():
    """Menú de configuración mejorado con paginación"""
    files = list_config_files()
    
    if not files:
        console.print("[red]No se encontraron archivos de configuración.[/red]")
        return
    
    page_size = 20
    total_pages = math.ceil(len(files) / page_size)
    current_page = 1
    
    while True:
        start_idx = (current_page - 1) * page_size
        end_idx = start_idx + page_size
        page_files = files[start_idx:end_idx]
        
        table = Table(title=f"Archivos de Configuración - Página {current_page}/{total_pages}")
        table.add_column("N°", style="cyan", no_wrap=True)
        table.add_column("Archivo", style="green")
        table.add_column("Tipo", style="yellow")
        table.add_column("Tamaño", style="magenta")
        
        for i, file_path in enumerate(page_files, start_idx + 1):
            ext = os.path.splitext(file_path)[1]
            try:
                size = os.path.getsize(file_path)
                size_kb = round(size / 1024, 2)
                size_str = f"{size_kb} KB"
            except:
                size_str = "N/A"
            
            relative_path = os.path.relpath(file_path, SERVER_PATH)
            table.add_row(str(i), relative_path, ext, size_str)
        
        console.print(table)
        
        console.print(f"[cyan]Página {current_page} de {total_pages} | Total: {len(files)} archivos[/cyan]")
        console.print("[yellow]Comandos disponibles:[/yellow]")
        console.print("[yellow]- [n]ext: Siguiente página[/yellow]")
        console.print("[yellow]- [p]rev: Página anterior[/yellow]")
        console.print("[yellow]- [g]oto: Ir a página específica[/yellow]")
        console.print("[yellow]- [e]dit <número>: Editar archivo[/yellow]")
        console.print("[yellow]- [s]earch: Buscar archivo[/yellow]")
        console.print("[yellow]- [q]uit: Salir[/yellow]")
        
        cmd = Prompt.ask("Comando").lower().split()
        
        if not cmd:
            continue
            
        action = cmd[0]
        
        if action == 'n' and current_page < total_pages:
            current_page += 1
        elif action == 'p' and current_page > 1:
            current_page -= 1
        elif action == 'g':
            page = IntPrompt.ask(f"Ir a página (1-{total_pages})", default=current_page)
            if 1 <= page <= total_pages:
                current_page = page
        elif action == 'e':
            if len(cmd) > 1 and cmd[1].isdigit():
                file_num = int(cmd[1])
            else:
                file_num = IntPrompt.ask("Número de archivo a editar", default=0)
            
            if 1 <= file_num <= len(files):
                file_to_edit = files[file_num - 1]
                console.print(f"[yellow]Editando: {os.path.relpath(file_to_edit, SERVER_PATH)}[/yellow]")
                if Confirm.ask("¿Continuar con la edición?"):
                    os.system(f"nano '{file_to_edit}'")
            else:
                console.print("[red]Número de archivo inválido[/red]")
        elif action == 's':
            search_term = Prompt.ask("Buscar archivo")
            search_results = [f for f in files if search_term.lower() in f.lower()]
            
            if search_results:
                search_table = Table(title=f"Resultados de búsqueda para '{search_term}'")
                search_table.add_column("N°", style="cyan")
                search_table.add_column("Archivo", style="green")
                
                for i, file_path in enumerate(search_results, 1):
                    relative_path = os.path.relpath(file_path, SERVER_PATH)
                    search_table.add_row(str(i), relative_path)
                
                console.print(search_table)
                
                if Confirm.ask("¿Editar alguno de estos archivos?"):
                    file_num = IntPrompt.ask("Número de archivo", default=0)
                    if 1 <= file_num <= len(search_results):
                        file_to_edit = search_results[file_num - 1]
                        os.system(f"nano '{file_to_edit}'")
            else:
                console.print(f"[red]No se encontraron archivos que contengan '{search_term}'[/red]")
        elif action == 'q':
            break
        else:
            console.print("[red]Comando no reconocido[/red]")
def load_or_create_config():
    """
    Maneja la carga o creación del archivo de configuración con soporte para múltiples instancias
    """
    global SERVER_PATH, SCREEN_NAME, BACKUP_PATH
    
    console.print(Panel.fit(
        "🔧 Verificando configuración del servidor...",
        title="Configuración",
        border_style="cyan"
    ))
    
    # Caso 1: El archivo no existe
    if not os.path.exists(CONFIG_FILE):
        console.print(f"[yellow]📄 Archivo {CONFIG_FILE} no encontrado.[/yellow]")
        console.print("[cyan]🔧 Configuración inicial requerida.[/cyan]")
        
        # Solicitar path base donde están las instancias
        base_path = Prompt.ask("📁 Ingresa la ruta base donde están las instancias de servidores")
        
        # Validar que la ruta existe
        if not os.path.isdir(base_path):
            console.print("[red]❌ La ruta especificada no existe o no es un directorio válido.[/red]")
            return False
        
        # Seleccionar instancia específica
        selected_instance = select_server_instance(base_path)
        if not selected_instance:
            console.print("[red]❌ No se seleccionó ninguna instancia válida.[/red]")
            return False
        
        # Crear configuración
        config = create_config(base_path, selected_instance)
        
        # Guardar configuración
        try:
            with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                json.dump(config, f, indent=4, ensure_ascii=False)
            console.print(f"[green]✅ Archivo {CONFIG_FILE} creado exitosamente.[/green]")
        except Exception as e:
            console.print(f"[red]❌ Error al crear {CONFIG_FILE}: {e}[/red]")
            return False
    
    # Caso 2: El archivo existe
    else:
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                config_content = f.read().strip()
                
            # Verificar si está vacío
            if not config_content:
                console.print(f"[yellow]📄 Archivo {CONFIG_FILE} está vacío.[/yellow]")
                return handle_empty_config()
            
            else:
                # El archivo existe y tiene contenido
                try:
                    config = json.loads(config_content)
                    console.print(f"[green]✅ Configuración cargada desde {CONFIG_FILE}.[/green]")
                    
                    # Validar configuración
                    if not validate_config(config):
                        console.print("[yellow]⚠️ Configuración incompleta o inválida.[/yellow]")
                        if Confirm.ask("¿Deseas reconfigurar?"):
                            return handle_reconfiguration()
                        else:
                            return False
                    
                    # Si la configuración es del formato antiguo (solo server_path), migrar
                    if "server_path" in config and "base_path" not in config:
                        console.print("[yellow]⚠️ Detectada configuración del formato antiguo. Migrando...[/yellow]")
                        return migrate_old_config(config)
                    
                except json.JSONDecodeError:
                    console.print(f"[red]❌ Error: {CONFIG_FILE} contiene JSON inválido.[/red]")
                    if Confirm.ask("¿Deseas recrear la configuración?"):
                        return handle_reconfiguration()
                    else:
                        return False
                        
        except Exception as e:
            console.print(f"[red]❌ Error al leer {CONFIG_FILE}: {e}[/red]")
            return False
    
    # Aplicar configuración a las variables globales
    apply_config(config)
    
    # Mostrar configuración cargada
    show_loaded_config(config)
    
    return True
def handle_empty_config():
    """Maneja el caso de configuración vacía"""
    console.print("[cyan]🔧 Configuración requerida.[/cyan]")
    
    base_path = Prompt.ask("📁 Ingresa la ruta base donde están las instancias de servidores")
    
    if not os.path.isdir(base_path):
        console.print("[red]❌ La ruta especificada no existe o no es un directorio válido.[/red]")
        return False
    
    selected_instance = select_server_instance(base_path)
    if not selected_instance:
        return False
    
    config = create_config(base_path, selected_instance)
    
    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, indent=4, ensure_ascii=False)
    console.print(f"[green]✅ Configuración guardada en {CONFIG_FILE}.[/green]")
    
    apply_config(config)
    show_loaded_config(config)
    return True
def create_config(base_path, selected_instance):
    """Crea un diccionario de configuración basado en la instancia seleccionada"""
    
    config = {
        "base_path": base_path,
        "selected_instance": {
            "name": selected_instance['name'],
            "path": selected_instance['path']
        },
        "server_path": selected_instance['path'],  # Mantener compatibilidad
        "version": "2.0"  # Versión de configuración
    }
    
    return config
def handle_reconfiguration():
    """Maneja la reconfiguración completa"""
    base_path = Prompt.ask("📁 Ingresa la ruta base donde están las instancias de servidores")
    if not os.path.isdir(base_path):
        console.print("[red]❌ La ruta especificada no existe.[/red]")
        return False
    
    selected_instance = select_server_instance(base_path)
    if not selected_instance:
        return False
    
    config = create_config(base_path, selected_instance)
    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, indent=4, ensure_ascii=False)
    
    apply_config(config)
    show_loaded_config(config)
    return True
def migrate_old_config(old_config):
    """Migra configuración del formato antiguo al nuevo"""
    console.print("[cyan]🔄 Migrando configuración al nuevo formato...[/cyan]")
    
    old_server_path = old_config["server_path"]
    
    # Determinar si el path antiguo es una instancia específica o un directorio base
    if os.path.isfile(os.path.join(old_server_path, "run.sh")) and os.path.isfile(os.path.join(old_server_path, "server.jar")):
        # Es una instancia específica, el directorio padre será la base
        base_path = os.path.dirname(old_server_path)
        instance_name = os.path.basename(old_server_path)
        
        console.print(f"[yellow]Detectada instancia específica: {instance_name}[/yellow]")
        console.print(f"[yellow]Directorio base inferido: {base_path}[/yellow]")
        
        if Confirm.ask("¿Es correcto este directorio base?"):
            selected_instance = {
                'name': instance_name,
                'path': old_server_path,
                'run_sh': os.path.join(old_server_path, "run.sh"),
                'server_jar': os.path.join(old_server_path, "server.jar")
            }
        else:
            base_path = Prompt.ask("📁 Ingresa la ruta base correcta")
            selected_instance = select_server_instance(base_path)
            if not selected_instance:
                return False
    else:
        # Tratar como directorio base y buscar instancias
        base_path = old_server_path
        selected_instance = select_server_instance(base_path)
        if not selected_instance:
            return False
    
    # Crear nueva configuración
    config = create_config(base_path, selected_instance)
    
    # Guardar configuración migrada
    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, indent=4, ensure_ascii=False)
    
    console.print("[green]✅ Configuración migrada exitosamente.[/green]")
    
    apply_config(config)
    show_loaded_config(config)
    return True
def validate_config(config):
    """Valida que la configuración tenga todos los campos necesarios"""
    
    # Configuración nueva (v2.0)
    if config.get("version") == "2.0":
        required_fields = ["base_path", "selected_instance", "server_path"]
        
        for field in required_fields:
            if field not in config:
                console.print(f"[red]❌ Campo requerido '{field}' no encontrado en la configuración.[/red]")
                return False
        
        # Validar que el path base existe
        if not os.path.isdir(config["base_path"]):
            console.print(f"[red]❌ La ruta base '{config['base_path']}' no existe.[/red]")
            return False
        
        # Validar que la instancia seleccionada existe
        instance_path = config["selected_instance"]["path"]
        if not os.path.isdir(instance_path):
            console.print(f"[red]❌ La instancia seleccionada '{instance_path}' no existe.[/red]")
            return False
        
        # Validar archivos requeridos
        run_sh = os.path.join(instance_path, "run.sh")
        server_jar = os.path.join(instance_path, "server.jar")
        
        if not os.path.isfile(run_sh):
            console.print(f"[red]❌ Archivo run.sh no encontrado en '{instance_path}'.[/red]")
            return False
        
        if not os.path.isfile(server_jar):
            console.print(f"[red]❌ Archivo server.jar no encontrado en '{instance_path}'.[/red]")
            return False
    
    # Configuración antigua (solo server_path)
    elif "server_path" in config and "base_path" not in config:
        if not os.path.isdir(config["server_path"]):
            console.print(f"[red]❌ La ruta del servidor '{config['server_path']}' no existe.[/red]")
            return False
    
    else:
        console.print("[red]❌ Formato de configuración no reconocido.[/red]")
        return False
    
    return True
def apply_config(config):
    """Aplica la configuración a las variables globales"""
    global SERVER_PATH, SCREEN_NAME, BACKUP_PATH
    
    if config.get("version") == "2.0":
        SERVER_PATH = config["selected_instance"]["path"]
        SCREEN_NAME = config["selected_instance"]["name"]
        BACKUP_PATH = os.path.join(SERVER_PATH, "backups")
    else:
        # Configuración antigua
        SERVER_PATH = config["server_path"]
        SCREEN_NAME = os.path.basename(os.path.abspath(SERVER_PATH))
        BACKUP_PATH = os.path.join(SERVER_PATH, "backups")

    # Crear directorio de backups si no existe
    if not os.path.exists(BACKUP_PATH):
        try:
            os.makedirs(BACKUP_PATH)
            console.print(f"[green]📁 Directorio de backups creado: {BACKUP_PATH}[/green]")
        except Exception as e:
            console.print(f"[yellow]⚠️ No se pudo crear el directorio de backups: {e}[/yellow]")
def show_loaded_config(config):
    """Muestra la configuración cargada en un panel"""
    
    if config.get("version") == "2.0":
        config_info = f"""[bold]Versión de Configuración:[/bold] {config['version']}
[bold]Ruta Base:[/bold] {config['base_path']}
[bold]Instancia Seleccionada:[/bold] {config['selected_instance']['name']}
[bold]Ruta de la Instancia:[/bold] {config['selected_instance']['path']}
[bold]Nombre de Screen:[/bold] {config['selected_instance']['name']}
[bold]Ruta de Backups:[/bold] {BACKUP_PATH}"""
    else:
        config_info = f"""[bold]Ruta del Servidor:[/bold] {config['server_path']}
[bold]Nombre de Screen:[/bold] {os.path.basename(os.path.abspath(config['server_path']))}
[bold]Ruta de Backups:[/bold] {BACKUP_PATH}"""
    
    panel = Panel.fit(
        config_info,
        title="⚙️ Configuración Cargada",
        border_style="green"
    )
    console.print(panel)
def update_config(key, value):
    """Actualiza un valor específico en la configuración"""
    try:
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            config = json.load(f)
        
        config[key] = value
        
        with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=4, ensure_ascii=False)
        
        console.print(f"[green]✅ Configuración actualizada: {key} = {value}[/green]")
        return True
    except Exception as e:
        console.print(f"[red]❌ Error al actualizar configuración: {e}[/red]")
        return False
def show_config_menu():
    """Menú para gestionar la configuración con soporte para múltiples instancias"""
    table = Table(title="⚙️ Gestión de Configuración")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Ver configuración actual")
    table.add_row("2", "Cambiar instancia de servidor")
    table.add_row("3", "Cambiar ruta base")
    table.add_row("4", "Listar todas las instancias disponibles")
    table.add_row("5", "Recrear configuración completa")
    table.add_row("0", "Volver al menú principal")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción", default=0)

    if choice == 1:
        # Ver configuración actual
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                config = json.load(f)
            show_loaded_config(config)
            
            # Mostrar detalles de la instancia actual si es configuración v2.0
            if config.get("version") == "2.0":
                current_instance = {
                    'name': config['selected_instance']['name'],
                    'path': config['selected_instance']['path'],
                    'run_sh': os.path.join(config['selected_instance']['path'], 'run.sh'),
                    'server_jar': os.path.join(config['selected_instance']['path'], 'server.jar')
                }
                show_instance_details(current_instance)
        except Exception as e:
            console.print(f"[red]❌ Error al leer configuración: {e}[/red]")
    
    elif choice == 2:
        # Cambiar instancia de servidor
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                config = json.load(f)
            
            if config.get("version") == "2.0":
                base_path = config["base_path"]
                console.print(f"[cyan]Buscando instancias en: {base_path}[/cyan]")
                
                selected_instance = select_server_instance(base_path)
                if selected_instance:
                    config["selected_instance"] = {
                        "name": selected_instance['name'],
                        "path": selected_instance['path']
                    }
                    config["server_path"] = selected_instance['path']
                    
                    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                        json.dump(config, f, indent=4, ensure_ascii=False)
                    
                    apply_config(config)
                    console.print("[green]✅ Instancia cambiada exitosamente.[/green]")
            else:
                console.print("[yellow]⚠️ Esta opción requiere configuración v2.0. Usa 'Recrear configuración'.[/yellow]")
        except Exception as e:
            console.print(f"[red]❌ Error al cambiar instancia: {e}[/red]")
    
    elif choice == 3:
        # Cambiar ruta base
        new_base_path = Prompt.ask("Nueva ruta base")
        if os.path.isdir(new_base_path):
            try:
                with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                
                config["base_path"] = new_base_path
                config["version"] = "2.0"
                
                # Seleccionar nueva instancia en la nueva ruta base
                selected_instance = select_server_instance(new_base_path)
                if selected_instance:
                    config["selected_instance"] = {
                        "name": selected_instance['name'],
                        "path": selected_instance['path']
                    }
                    config["server_path"] = selected_instance['path']
                    
                    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                        json.dump(config, f, indent=4, ensure_ascii=False)
                    
                    apply_config(config)
                    console.print("[green]✅ Ruta base cambiada exitosamente.[/green]")
            except Exception as e:
                console.print(f"[red]❌ Error al cambiar ruta base: {e}[/red]")
        else:
            console.print("[red]❌ La ruta especificada no existe.[/red]")
    
    elif choice == 4:
        # Listar todas las instancias disponibles
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                config = json.load(f)
            
            if config.get("version") == "2.0":
                base_path = config["base_path"]
                console.print(f"[cyan]Listando todas las instancias en: {base_path}[/cyan]")
                find_valid_server_instances(base_path)
            else:
                console.print("[yellow]⚠️ Esta opción requiere configuración v2.0.[/yellow]")
        except Exception as e:
            console.print(f"[red]❌ Error al listar instancias: {e}[/red]")
    
    elif choice == 5:
        # Recrear configuración completa
        if Confirm.ask("¿Estás seguro de recrear la configuración completa?"):
            base_path = Prompt.ask("📁 Ingresa la ruta base donde están las instancias de servidores")
            if os.path.isdir(base_path):
                selected_instance = select_server_instance(base_path)
                if selected_instance:
                    config = create_config(base_path, selected_instance)
                    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                        json.dump(config, f, indent=4, ensure_ascii=False)
                    apply_config(config)
                    console.print("[green]✅ Configuración recreada exitosamente.[/green]")
            else:
                console.print("[red]❌ La ruta especificada no existe.[/red]")
#endregion

#region interfaz principal
# ==================== INTERFAZ PRINCIPAL ====================
def show_status_panel():
    running = is_server_running(SCREEN_NAME)
    status = "[green]🟢 Encendido[/green]" if running else "[red]🔴 Apagado[/red]"
    ram = get_ram_usage(SCREEN_NAME)
    ram_text = f"{ram} MB" if ram else "N/A"
    
    players = get_connected_players(SCREEN_NAME) if running else []
    player_count = len(players)
    
    panel = Panel.fit(
        f"[bold]Servidor:[/bold] {SCREEN_NAME}\n"
        f"[bold]Ruta:[/bold] {SERVER_PATH}\n"
        f"[bold]Estado:[/bold] {status}\n"
        f"[bold]RAM Usada:[/bold] {ram_text}\n"
        f"[bold]Jugadores:[/bold] {player_count} conectados\n"
        f"[bold]Backups:[/bold] {BACKUP_PATH if BACKUP_PATH else 'No configurado'}",
        title="🎮 Estado del Servidor Minecraft",
        border_style="cyan"
    )
    console.print(panel)
def show_main_menu():
    table = Table(title="🎮 Panel de Administración de Minecraft Unificado")
    table.add_column("Opción", justify="right", style="cyan")
    table.add_column("Descripción", style="magenta")

    table.add_row("1", "🚀 Iniciar Servidor")
    table.add_row("2", "🛑 Detener Servidor")
    table.add_row("3", "🔄 Reiniciar Servidor")
    table.add_row("4", "⚡ Comandos rápidos (mejorados)")
    table.add_row("5", "💬 Comando personalizado")
    table.add_row("6", "👥 Gestión de jugadores")
    table.add_row("7", "🔌 Gestión de plugins/mods")
    table.add_row("8", "🌍 Gestión de mundos")
    table.add_row("9", "💾 Gestión de backups")
    table.add_row("10", "⚙️ Editar configuraciones (paginado)")
    table.add_row("11", "📊 Monitor de rendimiento")
    table.add_row("12", "📋 Ver logs")
    table.add_row("13", "🔔 TNT Alert - Notificaciones") # Nueva opción
    table.add_row("14", "⚙️ Configuración de scrip") # Nueva opción
    table.add_row("0", "❌ Salir")

    console.print(table)
#endregion

#region instancias
# ==================== FUNCIONES DE VALIDACIÓN DE INSTANCIAS ====================
def find_valid_server_instances(base_path):
    """
    Busca instancias válidas de servidores en el directorio base.
    Una instancia es válida si contiene run.sh y server.jar
    """
    valid_instances = []
    
    if not os.path.exists(base_path):
        console.print(f"[red]❌ El directorio base {base_path} no existe.[/red]")
        return valid_instances
    
    try:
        # Listar todos los subdirectorios
        for item in os.listdir(base_path):
            item_path = os.path.join(base_path, item)
            
            # Solo procesar directorios
            if os.path.isdir(item_path):
                run_sh_path = os.path.join(item_path, "run.sh")
                start_sh_path = os.path.join(item_path, "start.sh")
                server_jar_path = os.path.join(item_path, "server.jar")
                
                # Verificar si ambos archivos existen
                if os.path.isfile(run_sh_path) and os.path.isfile(server_jar_path):
                    valid_instances.append({
                        'name': item,
                        'path': item_path,
                        'run_sh': run_sh_path,
                        'server_jar': server_jar_path
                    })
                    console.print(f"[green]✅ Instancia válida encontrada: {item}[/green]")
                elif os.path.isfile(run_sh_path):
                    valid_instances.append({
                        'name': item,
                        'path': item_path,
                        'run_sh': run_sh_path
                    })
                    console.print(f"[yellow]⚠️ Instancia posiblemente válida encontrada: {item} puede ser solo forge[/yellow]")
                elif os.path.isfile(start_sh_path):
                    valid_instances.append({
                        'name': item,
                        'path': item_path,
                        'start_sh': start_sh_path
                    })
                    console.print(f"[yellow]⚠️ Instancia posiblemente válida encontrada: {item} puede ser solo forge[/yellow]")
                else:
                    missing_files = []
                    if not os.path.isfile(run_sh_path):
                        missing_files.append("run.sh")
                    if not os.path.isfile(server_jar_path):
                        missing_files.append("server.jar")
                    console.print(f"[red]❌ Instancia inválida '{item}': faltan archivos {', '.join(missing_files)}[/red]")
    
    except Exception as e:
        console.print(f"[red]❌ Error al escanear el directorio: {e}[/red]")
    
    return valid_instances
def select_server_instance(base_path):
    """
    Permite al usuario seleccionar una instancia de servidor válida
    """
    console.print(f"[cyan]🔍 Buscando instancias de servidores en: {base_path}[/cyan]")
    
    valid_instances = find_valid_server_instances(base_path)
    
    if not valid_instances:
        console.print("[red]❌ No se encontraron instancias válidas de servidores.[/red]")
        console.print("[yellow]💡 Una instancia válida debe contener los archivos 'run.sh' y 'server.jar'[/yellow]")
        return None
    
    # Mostrar tabla de instancias válidas
    instances_table = Table(title="🎮 Instancias de Servidores Disponibles")
    instances_table.add_column("N°", style="cyan", no_wrap=True)
    instances_table.add_column("Nombre", style="green")
    instances_table.add_column("Ruta", style="yellow")
    instances_table.add_column("Estado", style="magenta")
    
    for i, instance in enumerate(valid_instances, 1):
        # Verificar si el servidor está corriendo
        screen_name = instance['name']
        is_running = is_server_running(screen_name)
        status = "🟢 Activo" if is_running else "🔴 Inactivo"
        
        instances_table.add_row(
            str(i), 
            instance['name'], 
            instance['path'], 
            status
        )
    
    console.print(instances_table)
    console.print(f"[cyan]Total de instancias válidas encontradas: {len(valid_instances)}[/cyan]")
    
    # Permitir selección
    while True:
        try:
            choice = IntPrompt.ask(
                f"Selecciona una instancia (1-{len(valid_instances)}) o 0 para cancelar", 
                default=0
            )
            
            if choice == 0:
                console.print("[yellow]Selección cancelada.[/yellow]")
                return None
            
            if 1 <= choice <= len(valid_instances):
                selected_instance = valid_instances[choice - 1]
                console.print(f"[green]✅ Instancia seleccionada: {selected_instance['name']}[/green]")
                console.print(f"[green]📁 Ruta: {selected_instance['path']}[/green]")
                
                # Confirmar selección
                if Confirm.ask(f"¿Confirmas la selección de '{selected_instance['name']}'?"):
                    return selected_instance
                else:
                    console.print("[yellow]Selección cancelada. Elige otra instancia.[/yellow]")
                    continue
            else:
                console.print(f"[red]❌ Opción inválida. Debe estar entre 1 y {len(valid_instances)}[/red]")
                
        except KeyboardInterrupt:
            console.print("\n[yellow]Selección cancelada por el usuario.[/yellow]")
            return None
        except Exception as e:
            console.print(f"[red]❌ Error en la selección: {e}[/red]")
def show_instance_details(instance):
    """
    Muestra detalles detallados de la instancia seleccionada
    """
    details_table = Table(title=f"📋 Detalles de la Instancia: {instance['name']}")
    details_table.add_column("Propiedad", style="cyan")
    details_table.add_column("Valor", style="green")
    
    details_table.add_row("Nombre", instance['name'])
    details_table.add_row("Ruta completa", instance['path'])
    details_table.add_row("Archivo run.sh", instance['run_sh'])
    details_table.add_row("Archivo server.jar", instance['server_jar'])
    
    # Información adicional de archivos
    try:
        run_sh_size = os.path.getsize(instance['run_sh'])
        server_jar_size = os.path.getsize(instance['server_jar'])
        
        details_table.add_row("Tamaño run.sh", f"{run_sh_size} bytes")
        details_table.add_row("Tamaño server.jar", f"{round(server_jar_size / 1024 / 1024, 2)} MB")
        
        # Verificar permisos de ejecución
        run_sh_executable = os.access(instance['run_sh'], os.X_OK)
        details_table.add_row("run.sh ejecutable", "✅ Sí" if run_sh_executable else "❌ No")
        
    except Exception as e:
        details_table.add_row("Error", f"No se pudo obtener información: {e}")
    
    console.print(details_table)
#endregion

# ==================== FUNCIÓN PRINCIPAL ====================
def main():
    global SERVER_PATH, SCREEN_NAME, BACKUP_PATH

    console.print(Panel.fit(
        "🎮 Panel de Administración de Minecraft Unificado\n"
        "Versión con soporte para múltiples instancias de servidores\n"
        "Compatible con Minecraft 1.20.2",
        title="Panel de Administración",
        border_style="green"
    ))

    # Cargar o crear configuración con soporte para múltiples instancias
    if not load_or_create_config():
        console.print("[red]❌ Error en la configuración. Saliendo...[/red]")
        return

    # Mostrar información de la instancia seleccionada
    console.print(f"[green]🎮 Instancia activa: {SCREEN_NAME}[/green]")
    console.print(f"[green]📁 Ruta: {SERVER_PATH}[/green]")

    # Crear archivos por defecto
    create_default_files()

    try:
        while True:
            try:
                show_status_panel()
                show_main_menu()
                choice = IntPrompt.ask("Selecciona una opción", default=0)

                # [Resto del código del menú principal permanece igual]
                if choice == 1:
                    start_server(SCREEN_NAME, SERVER_PATH)
                elif choice == 2:
                    stop_server()
                elif choice == 3:
                    restart_server()
                elif choice == 4:
                    quick_commands_menu()
                elif choice == 5:
                    cmd = Prompt.ask("Escribe tu comando personalizado")
                    send_command(SCREEN_NAME, cmd)
                elif choice == 6:
                    player_management_menu()
                elif choice == 7:
                    plugin_management_menu()
                elif choice == 8:
                    world_management_menu()
                elif choice == 9:
                    backup_management_menu()
                elif choice == 10:
                    paginated_config_menu()
                elif choice == 11:
                    performance_monitor()
                elif choice == 12:
                    view_logs()
                elif choice == 13:
                    tnt_alert_menu()
                elif choice == 14:
                    show_config_menu()  # Menú de configuración mejorado
                elif choice == 0:
                    console.print("👋 Saliendo del panel...", style="bold red")
                    break
                else:
                    console.print("❌ Opción inválida.", style="red")
                    
            except KeyboardInterrupt:
                console.print("\nOperación cancelada por el usuario", style="yellow")
            except Exception as e:
                console.print(f"Error: {e}", style="red")
                
    except KeyboardInterrupt:
        console.print("\nSaliendo del panel...", style="yellow")
    except Exception as e:
        console.print(f"Error crítico: {e}", style="red")

if __name__ == "__main__":
    main()