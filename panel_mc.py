import subprocess
import os
import time
import psutil
import re
import json
import shutil
import zipfile
from datetime import datetime
from rich.console import Console
from rich.prompt import Prompt, IntPrompt, Confirm
from rich.table import Table
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn
from rich.layout import Layout
from rich.live import Live
import math
import random
import threading

console = Console()

# Variables globales
SERVER_PATH = ""
SCREEN_NAME = ""
BACKUP_PATH = ""

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

def restart_server():
    stop_server()
    time.sleep(10)
    start_server(SCREEN_NAME, SERVER_PATH)

def get_connected_players(screen_name):
    temp_file = "/tmp/screen_output.txt"
    send_command(screen_name, "list")
    time.sleep(2)
    
    hardcopy_cmd = f'screen -S {screen_name} -X hardcopy {temp_file}'
    subprocess.run(hardcopy_cmd, shell=True)

    if not os.path.isfile(temp_file):
        return []

    players = []
    with open(temp_file, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    match = re.search(r"players online: (.*)", content)
    if match:
        players_str = match.group(1).strip()
        if players_str:
            players = [p.strip() for p in players_str.split(",") if p.strip() != '']

    if os.path.exists(temp_file):
        os.remove(temp_file)
    return players

def quick_commands_menu():
    table = Table(title="Comandos Rápidos")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    console.print(table)
    
    # Crear submenús
    submenu_table = Table(title="Categorías de Comandos")
    submenu_table.add_column("Opción", style="cyan")
    submenu_table.add_column("Categoría", style="green")
    
    submenu_table.add_row("1", "🌍 Comandos de Mundo (tiempo, clima, dificultad)")
    submenu_table.add_row("2", "🎁 Comandos de Items (give, enchant)")
    submenu_table.add_row("3", "👤 Comandos de Jugador (tp, gamemode)")
    submenu_table.add_row("4", "👹 Comandos de Mobs (summon, execute)")
    submenu_table.add_row("5", "⚡ Comandos Especiales (efectos, partículas)")
    
    console.print(submenu_table)
    category = Prompt.ask("Selecciona una categoría o escribe 'back' para volver")
    
    if category == "1":
        world_commands_menu()
    elif category == "2":
        item_commands_menu()
    elif category == "3":
        player_commands_menu()
    elif category == "4":
        mob_commands_menu()
    elif category == "5":
        special_commands_menu()
    elif category.lower() != "back":
        console.print("[red]Categoría no válida.[/red]")

def world_commands_menu():
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

def item_commands_menu():
    table = Table(title="🎁 Comandos de Items")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    items = {
        "1": ('minecraft:diamond_sword', "Espada de diamante"),
        "2": ('minecraft:diamond_pickaxe', "Pico de diamante"),
        "3": ('minecraft:diamond_armor', "Armadura de diamante completa"),
        "4": ('minecraft:elytra', "Élitros"),
        "5": ('minecraft:totem_of_undying', "Tótem de la inmortalidad"),
        "6": ('minecraft:enchanted_golden_apple', "Manzana dorada encantada"),
        "7": ('minecraft:netherite_sword', "Espada de netherita"),
        "8": ('minecraft:bow', "Arco"),
        "9": ('minecraft:arrow', "Flechas"),
        "10": ('minecraft:ender_pearl', "Perlas de ender"),
    }

    for k, (item, desc) in items.items():
        table.add_row(k, desc)

    console.print(table)
    sel = Prompt.ask("Selecciona un item o escribe 'back' para volver")

    if sel in items:
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
            item = items[sel][0]
            
            if item == 'minecraft:diamond_armor':
                # Dar armadura completa
                armor_pieces = ['minecraft:diamond_helmet', 'minecraft:diamond_chestplate', 
                              'minecraft:diamond_leggings', 'minecraft:diamond_boots']
                for piece in armor_pieces:
                    send_command(SCREEN_NAME, f"give {player} {piece}")
            else:
                quantity = IntPrompt.ask("Cantidad", default=1)
                send_command(SCREEN_NAME, f"give {player} {item} {quantity}")
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

def mob_commands_menu():
    table = Table(title="👹 Comandos de Mobs")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Summon mob en jugador")
    table.add_row("2", "Execute at jugador - Summon mob")
    table.add_row("3", "Summon múltiples mobs")
    table.add_row("4", "Summon mob con efectos")
    table.add_row("5", "Limpiar mobs específicos")

    console.print(table)
    sel = Prompt.ask("Selecciona una opción o escribe 'back' para volver")

    if sel == "1":
        summon_mob_at_player()
    elif sel == "2":
        execute_summon_at_player()
    elif sel == "3":
        summon_multiple_mobs()
    elif sel == "4":
        summon_mob_with_effects()
    elif sel == "5":
        clear_specific_mobs()
    elif sel.lower() != "back":
        console.print("[red]Opción no válida.[/red]")

def get_mob_selection():
    """Muestra lista de mobs y retorna el seleccionado"""
    mobs = {
        "1": ("minecraft:zombie", "Zombie"),
        "2": ("minecraft:skeleton", "Esqueleto"),
        "3": ("minecraft:creeper", "Creeper"),
        "4": ("minecraft:enderman", "Enderman"),
        "5": ("minecraft:spider", "Araña"),
        "6": ("minecraft:witch", "Bruja"),
        "7": ("minecraft:villager", "Aldeano"),
        "8": ("minecraft:iron_golem", "Golem de hierro"),
        "9": ("minecraft:wither_skeleton", "Esqueleto wither"),
        "10": ("minecraft:blaze", "Blaze"),
        "11": ("minecraft:ghast", "Ghast"),
        "12": ("minecraft:ender_dragon", "Dragón del End"),
        "13": ("minecraft:wither", "Wither"),
        "14": ("minecraft:pig", "Cerdo"),
        "15": ("minecraft:cow", "Vaca"),
        "16": ("minecraft:sheep", "Oveja"),
        "17": ("minecraft:chicken", "Pollo"),
        "18": ("minecraft:horse", "Caballo"),
        "19": ("minecraft:wolf", "Lobo"),
        "20": ("minecraft:cat", "Gato"),
    }

    mobs_table = Table(title="Seleccionar Mob")
    mobs_table.add_column("N°", style="cyan")
    mobs_table.add_column("Mob", style="green")
    mobs_table.add_column("ID", style="yellow")

    for k, (mob_id, name) in mobs.items():
        mobs_table.add_row(k, name, mob_id)

    console.print(mobs_table)
    choice = Prompt.ask("Selecciona un mob (0 para cancelar)")
    
    if choice == "0":
        return None
    if choice in mobs:
        return mobs[choice][0]
    else:
        console.print("[red]Selección inválida[/red]")
        return None

def summon_mob_at_player():
    """Summon mob directamente en la posición del jugador"""
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    # Seleccionar jugador
    players_table = Table(title="Jugadores Conectados")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)

    player_choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if player_choice == 0:
        return
    if not (1 <= player_choice <= len(players)):
        console.print("[red]Selección inválida[/red]")
        return

    player = players[player_choice - 1]
    
    # Seleccionar mob
    mob_id = get_mob_selection()
    if not mob_id:
        return

    # Ejecutar comando
    send_command(SCREEN_NAME, f"execute at {player} run summon {mob_id}")

def execute_summon_at_player():
    """Execute at jugador con offset para summon"""
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    # Seleccionar jugador
    players_table = Table(title="Jugadores Conectados")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)

    player_choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if player_choice == 0:
        return
    if not (1 <= player_choice <= len(players)):
        console.print("[red]Selección inválida[/red]")
        return

    player = players[player_choice - 1]
    
    # Seleccionar mob
    mob_id = get_mob_selection()
    if not mob_id:
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
            time.sleep(0.5)  # Pequeña pausa entre comandos

def summon_multiple_mobs():
    """Summon múltiples mobs del mismo tipo"""
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    # Seleccionar jugador
    players_table = Table(title="Jugadores Conectados")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)

    player_choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if player_choice == 0:
        return
    if not (1 <= player_choice <= len(players)):
        console.print("[red]Selección inválida[/red]")
        return

    player = players[player_choice - 1]
    
    # Seleccionar mob
    mob_id = get_mob_selection()
    if not mob_id:
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
    
    console.print(pattern_table)
    pattern = IntPrompt.ask("Selecciona patrón", default=1)

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

def summon_mob_with_effects():
    """Summon mob con efectos especiales"""
    players = get_connected_players(SCREEN_NAME)
    if not players:
        console.print("[red]No hay jugadores conectados.[/red]")
        return

    # Seleccionar jugador
    players_table = Table(title="Jugadores Conectados")
    players_table.add_column("N°", style="cyan", no_wrap=True)
    players_table.add_column("Jugador", style="green")
    for i, p in enumerate(players, start=1):
        players_table.add_row(str(i), p)
    console.print(players_table)

    player_choice = IntPrompt.ask("Selecciona un jugador (0 para cancelar)", default=0)
    if player_choice == 0:
        return
    if not (1 <= player_choice <= len(players)):
        console.print("[red]Selección inválida[/red]")
        return

    player = players[player_choice - 1]
    
    # Seleccionar mob
    mob_id = get_mob_selection()
    if not mob_id:
        return

    # Efectos especiales
    effects_table = Table(title="Efectos Especiales")
    effects_table.add_column("Opción", style="cyan")
    effects_table.add_column("Descripción", style="green")
    
    effects_table.add_row("1", "Mob gigante (NoAI)")
    effects_table.add_row("2", "Mob invisible")
    effects_table.add_row("3", "Mob con nombre personalizado")
    effects_table.add_row("4", "Mob con efectos de poción")
    effects_table.add_row("5", "Mob normal")
    
    console.print(effects_table)
    effect_choice = IntPrompt.ask("Selecciona efecto", default=5)

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
    else:  # Normal
        command = f"execute at {player} run summon {mob_id}"

    send_command(SCREEN_NAME, command)

def clear_specific_mobs():
    """Limpiar mobs específicos"""
    mob_id = get_mob_selection()
    if not mob_id:
        return

    radius = IntPrompt.ask("Radio de limpieza (0 para todo el mundo)", default=0)
    
    if radius == 0:
        command = f"kill @e[type={mob_id}]"
    else:
        players = get_connected_players(SCREEN_NAME)
        if not players:
            console.print("[red]No hay jugadores conectados para usar como centro.[/red]")
            return
        
        # Seleccionar jugador como centro
        players_table = Table(title="Jugadores Conectados")
        players_table.add_column("N°", style="cyan", no_wrap=True)
        players_table.add_column("Jugador", style="green")
        for i, p in enumerate(players, start=1):
            players_table.add_row(str(i), p)
        console.print(players_table)

        player_choice = IntPrompt.ask("Selecciona jugador como centro (0 para cancelar)", default=0)
        if player_choice == 0:
            return
        if not (1 <= player_choice <= len(players)):
            console.print("[red]Selección inválida[/red]")
            return

        player = players[player_choice - 1]
        command = f"execute at {player} run kill @e[type={mob_id},distance=..{radius}]"

    send_command(SCREEN_NAME, command)

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
        
        rayo_table = Table(title="⚡ Opciones de Rayo")
        rayo_table.add_column("Opción", style="cyan")
        rayo_table.add_column("Descripción", style="green")
        
        rayo_table.add_row("1", "Rayo en posición actual del servidor")
        rayo_table.add_row("2", "Rayo en jugador específico")
        rayo_table.add_row("3", "Rayo múltiple en jugador")
        rayo_table.add_row("4", "Rayo en coordenadas específicas")
        
        console.print(rayo_table)
        rayo_choice = IntPrompt.ask("Selecciona tipo de rayo (0 para cancelar)", default=0)
        
        if rayo_choice == 0:
            return
        elif rayo_choice == 1:
            send_command(SCREEN_NAME, "summon lightning_bolt ~ ~ ~")
        
        elif rayo_choice == 2:  # Rayo en jugador específico
            if not players:
                console.print("[red]No hay jugadores conectados.[/red]")
                return
                
            players_table = Table(title="⚡ Seleccionar Jugador para Rayo")
            players_table.add_column("N°", style="cyan", no_wrap=True)
            players_table.add_column("Jugador", style="green")
            players_table.add_column("Estado", style="yellow")
            
            for i, p in enumerate(players, start=1):
                players_table.add_row(str(i), p, "🎯 Objetivo")
            
            console.print(players_table)
            
            player_choice = IntPrompt.ask("Selecciona jugador para el rayo (0 para cancelar)", default=0)
            if player_choice == 0:
                return
            if 1 <= player_choice <= len(players):
                player = players[player_choice - 1]
                
                # Opciones adicionales para el rayo
                offset_choice = Confirm.ask("¿Quieres configurar offset del rayo?", default=False)
                
                if offset_choice:
                    x_offset = IntPrompt.ask("Offset X", default=0)
                    y_offset = IntPrompt.ask("Offset Y", default=0)
                    z_offset = IntPrompt.ask("Offset Z", default=0)
                    command = f"execute at {player} run summon lightning_bolt ~{x_offset} ~{y_offset} ~{z_offset}"
                else:
                    command = f"execute at {player} run summon lightning_bolt"
                
                send_command(SCREEN_NAME, command)
                console.print(f"[yellow]⚡ Rayo lanzado sobre {player}![/yellow]")
            else:
                console.print("[red]Selección inválida[/red]")
        
        elif rayo_choice == 3:  # Rayo múltiple
            if not players:
                console.print("[red]No hay jugadores conectados.[/red]")
                return
                
            players_table = Table(title="⚡ Seleccionar Jugador para Rayos Múltiples")
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
                
                # Configuración de rayos múltiples
                rayo_count = IntPrompt.ask("¿Cuántos rayos?", default=3)
                rayo_delay = IntPrompt.ask("Delay entre rayos (segundos)", default=1)
                
                # Patrón de rayos
                pattern_table = Table(title="Patrón de Rayos")
                pattern_table.add_column("Opción", style="cyan")
                pattern_table.add_column("Descripción", style="green")
                
                pattern_table.add_row("1", "Todos en el jugador")
                pattern_table.add_row("2", "Círculo alrededor del jugador")
                pattern_table.add_row("3", "Aleatorio cerca del jugador")
                
                console.print(pattern_table)
                pattern = IntPrompt.ask("Selecciona patrón", default=1)
                
                console.print(f"[yellow]Lanzando {rayo_count} rayos sobre {player}...[/yellow]")
                
                for i in range(rayo_count):
                    if pattern == 1:  # Todos en el jugador
                        command = f"execute at {player} run summon lightning_bolt"
                    elif pattern == 2:  # Círculo
                        radius = 3
                        angle = (2 * math.pi * i) / rayo_count
                        x_offset = int(radius * math.cos(angle))
                        z_offset = int(radius * math.sin(angle))
                        command = f"execute at {player} run summon lightning_bolt ~{x_offset} ~ ~{z_offset}"
                    elif pattern == 3:  # Aleatorio
                        x_offset = random.randint(-5, 5)
                        z_offset = random.randint(-5, 5)
                        command = f"execute at {player} run summon lightning_bolt ~{x_offset} ~ ~{z_offset}"
                    
                    send_command(SCREEN_NAME, command)
                    console.print(f"[green]Rayo {i+1}/{rayo_count} lanzado[/green]")
                    
                    if i < rayo_count - 1:  # No esperar después del último rayo
                        time.sleep(rayo_delay)
            else:
                console.print("[red]Selección inválida[/red]")
        
        elif rayo_choice == 4:  # Coordenadas específicas
            x = IntPrompt.ask("Coordenada X")
            y = IntPrompt.ask("Coordenada Y")
            z = IntPrompt.ask("Coordenada Z")
            send_command(SCREEN_NAME, f"summon lightning_bolt {x} {y} {z}")
            console.print(f"[yellow]⚡ Rayo lanzado en coordenadas ({x}, {y}, {z})[/yellow]")
        
        else:
            console.print("[red]Opción inválida[/red]")
    
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
    """Menú dedicado específicamente a comandos de rayos"""
    table = Table(title="⚡ Comandos de Rayos Avanzados")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Rayo simple en jugador")
    table.add_row("2", "Tormenta de rayos en jugador")
    table.add_row("3", "Rayo de castigo (con efectos)")
    table.add_row("4", "Rayo de bendición (sin daño)")
    table.add_row("5", "Lluvia de rayos en área")
    table.add_row("6", "Rayo siguiendo al jugador")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    players = get_connected_players(SCREEN_NAME)
    
    if choice == 0:
        return
    elif choice in [1, 2, 3, 4, 6] and not players:
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

def player_management_menu():
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

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    if choice == 1:
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
        # Leer archivo ops.json si existe
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

def plugin_management_menu():
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
    table.add_row("6", "Ver logs de plugins")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    if choice == 1:
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

def backup_management_menu():
    global BACKUP_PATH
    
    table = Table(title="Gestión de Backups")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Crear backup completo")
    table.add_row("2", "Crear backup solo del mundo")
    table.add_row("3", "Listar backups")
    table.add_row("4", "Restaurar backup")
    table.add_row("5", "Eliminar backup")
    table.add_row("6", "Configurar ruta de backups")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    if choice == 1:
        create_full_backup()
    elif choice == 2:
        create_world_backup()
    elif choice == 3:
        list_backups()
    elif choice == 4:
        restore_backup()
    elif choice == 5:
        delete_backup()
    elif choice == 6:
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
                # Excluir logs y archivos temporales
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

def restore_backup():
    console.print("[yellow]Función de restauración no implementada por seguridad.[/yellow]")
    console.print("[yellow]Restaura manualmente desde: {BACKUP_PATH}[/yellow]")

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

def world_management_menu():
    table = Table(title="Gestión de Mundos")
    table.add_column("Opción", style="cyan")
    table.add_column("Descripción", style="green")

    table.add_row("1", "Información del mundo")
    table.add_row("2", "Cambiar spawn del mundo")
    table.add_row("3", "Regenerar chunks")
    table.add_row("4", "Limpiar entidades")
    table.add_row("5", "Guardar mundo")
    table.add_row("6", "Desactivar/Activar guardado automático")
    table.add_row("6", "Desactivar/Activar guardado automático")

    console.print(table)
    choice = IntPrompt.ask("Selecciona una opción (0 para volver)", default=0)

    if choice == 1:
        send_command(SCREEN_NAME, "seed")
        send_command(SCREEN_NAME, "worldborder get")
    elif choice == 2:
        x = IntPrompt.ask("Coordenada X")
        y = IntPrompt.ask("Coordenada Y")
        z = IntPrompt.ask("Coordenada Z")
        send_command(SCREEN_NAME, f"setworldspawn {x} {y} {z}")
    elif choice == 3:
        x1 = IntPrompt.ask("X1")
        z1 = IntPrompt.ask("Z1")
        x2 = IntPrompt.ask("X2")
        z2 = IntPrompt.ask("Z2")
        console.print("[yellow]Nota: Este comando puede requerir plugins específicos[/yellow]")
        send_command(SCREEN_NAME, f"regen {x1} {z1} {x2} {z2}")
    elif choice == 4:
        entity_type = Prompt.ask("Tipo de entidad (o 'all' para todas)", default="all")
        if entity_type == "all":
            send_command(SCREEN_NAME, "kill @e[type=!player]")
        else:
            send_command(SCREEN_NAME, f"kill @e[type={entity_type}]")
    elif choice == 5:
        send_command(SCREEN_NAME, "save-all")
    elif choice == 6:
        action = Prompt.ask("¿Activar o desactivar? (on/off)")
        if action.lower() == "off":
            send_command(SCREEN_NAME, "save-off")
        elif action.lower() == "on":
            send_command(SCREEN_NAME, "save-on")

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
        
        # Información del sistema
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

def edit_config_file():
    files = list_config_files()

    if not files:
        console.print("[red]No se encontraron archivos de configuración.[/red]")
        return

    table = Table(title="Archivos de Configuración")
    table.add_column("N°", style="cyan", no_wrap=True)
    table.add_column("Ruta", style="green")
    table.add_column("Tipo", style="yellow")

    for i, path in enumerate(files):
        ext = os.path.splitext(path)[1]
        table.add_row(str(i + 1), path, ext)

    console.print(table)

    choice = IntPrompt.ask("Elige un archivo para editar (0 para cancelar)", default=0)
    if choice == 0:
        return
    if 1 <= choice <= len(files):
        file_to_edit = files[choice - 1]
        console.print(f"[yellow]Abriendo {file_to_edit} con nano...[/yellow]")
        os.system(f"nano '{file_to_edit}'")
    else:
        console.print("[red]Selección inválida[/red]")

def show_status_panel():
    running = is_server_running(SCREEN_NAME)
    status = "[green]🟢 Encendido[/green]" if running else "[red]🔴 Apagado[/red]"
    ram = get_ram_usage(SCREEN_NAME)
    ram_text = f"{ram} MB" if ram else "N/A"
    
    # Información adicional
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
    table = Table(title="🎮 Panel de Administración de Minecraft Avanzado")
    table.add_column("Opción", justify="right", style="cyan")
    table.add_column("Descripción", style="magenta")

    table.add_row("1", "🚀 Iniciar Servidor")
    table.add_row("2", "🛑 Detener Servidor")
    table.add_row("3", "🔄 Reiniciar Servidor")
    table.add_row("4", "⚡ Comandos rápidos")
    table.add_row("5", "💬 Comando personalizado")
    table.add_row("6", "👥 Gestión de jugadores")
    table.add_row("7", "🔌 Gestión de plugins/mods")
    table.add_row("8", "🌍 Gestión de mundos")
    table.add_row("9", "💾 Gestión de backups")
    table.add_row("10", "⚙️ Editar configuraciones")
    table.add_row("11", "📊 Monitor de rendimiento")
    table.add_row("12", "📋 Ver logs")
    table.add_row("13", "⚡ Comandos de rayos avanzados")
    table.add_row("0", "❌ Salir")

    console.print(table)

def main():
    global SERVER_PATH, SCREEN_NAME, BACKUP_PATH

    console.print(Panel.fit(
        "[bold green]🎮 Panel de Administración de Minecraft Avanzado[/bold green]\n"
        "[yellow]Versión mejorada con soporte para plugins y mods[/yellow]",
        border_style="green"
    ))

    SERVER_PATH = Prompt.ask("📁 Ingresa la ruta del servidor")
    if not os.path.isdir(SERVER_PATH):
        console.print("[red]❌ Ruta no válida[/red]")
        return

    SCREEN_NAME = os.path.basename(os.path.abspath(SERVER_PATH))
    BACKUP_PATH = os.path.join(os.path.dirname(SERVER_PATH), "backups")
    
    if not os.path.exists(BACKUP_PATH):
        if Confirm.ask(f"¿Crear directorio de backups en {BACKUP_PATH}?"):
            os.makedirs(BACKUP_PATH)

    while True:
        try:
            show_status_panel()
            show_main_menu()
            choice = IntPrompt.ask("Selecciona una opción", default=0)

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
                edit_config_file()
            elif choice == 11:
                performance_monitor()
            elif choice == 12:
                view_logs()
            elif choice == 13:
                lightning_commands_menu()
            elif choice == 0:
                console.print("[bold red]👋 Saliendo del panel...[/bold red]")
                break
            else:
                console.print("[red]❌ Opción inválida.[/red]")
                
        except KeyboardInterrupt:
            console.print("\n[yellow]Operación cancelada por el usuario[/yellow]")
        except Exception as e:
            console.print(f"[red]Error: {e}[/red]")

if __name__ == "__main__":
    main()