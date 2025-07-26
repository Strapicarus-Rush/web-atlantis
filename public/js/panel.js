document.addEventListener('DOMContentLoaded', () => {

  let selectedInstance = undefined;
  const selectorSelectText = "Selecciona una instancia";
  const selectorStatusText = "Ver Status general";
  let instancesList = []
  const tabs = document.querySelectorAll('.tab');
  const sections = document.querySelectorAll('.content');
  const instanceSelector = document.getElementById('instanceSelector');
  const generalStatus = document.getElementById('generalStatus');
  const instanceStatus = document.getElementById('instanceStatus');
  const instanceNameSpan = document.querySelector('[data-instance-name]');

  function formatMemory(kb) {
    if (kb == null || isNaN(kb)) return "-";

    const mb = kb / 1024;
    if (mb < 1024) {
      return mb.toFixed(2) + " MB";
    } else {
      const gb = mb / 1024;
      return gb.toFixed(2) + " GB";
    }
  }
  
  // =============================================================
  //  Lógica para mostrar alertas
  // =============================================================

  function showAlert(message, type = 'success') {
    const container = document.getElementById('alert-container');
    const alert = document.createElement('div');
    alert.classList.add('alert', type);
    alert.innerHTML = `
      <span>${message}</span>
      <button class="close-btn" aria-label="Cerrar">&times;</button>
    `;

    const closeBtn = alert.querySelector('.close-btn');

    closeBtn.addEventListener('click', () => closeAlert(alert));

    container.appendChild(alert);

    const timeout = setTimeout(() => {
      closeAlert(alert);
    }, 10000);

    function closeAlert(el) {
      clearTimeout(timeout);
      el.style.animation = 'fadeOutAlert 0.4s ease-out forwards';
      el.addEventListener('animationend', () => {
        if (container.contains(el)) {
          container.removeChild(el);
        }
      });
    }
  }

  // =============================================================
  //  Lógica para selección de pestañas
  // =============================================================

  function switchTab(newTab) {
    const target = newTab.dataset.tab;
    const newSection = document.querySelector(`[data-content="${target}"]`);
    const currentSection = document.querySelector('.content.active');

    if (newSection === currentSection) return;

    stopContinuousUpdate(currentSection);

    currentSection.classList.remove('active');
    currentSection.classList.add('fade-out');

    setTimeout(() => {
      currentSection.classList.remove('fade-out');
      newSection.classList.add('active');

      const updateType = newSection.dataset.update;
      if (updateType === 'on-activate') {
        updateContent(newSection);
      } else if (updateType === 'continuous') {
        startContinuousUpdate(newSection);
      }
    }, 300);
  }

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      switchTab(tab);
    });
  });

  // =============================================================
  //  Lógica para manejo de funciones de secciones
  // =============================================================

  const sectionFunctions = {
    fetchStatus,
    fetchInstanceData,
    fetchConsole,
    updateStatus
  };

  function updateContent(section) {
    const fnName = section.dataset.function;
    const fn = sectionFunctions[fnName];

    if (typeof fn === 'function') {
      fn(section); // Ejecuta la función asignada a esta sección
    } else {
      showAlert(`Función "${fnName}" no está registrada.`, "Error");
    }
  }

  let updateIntervals = new Map();

  function startContinuousUpdate(section) {
    const key = section.dataset.content;
    if (updateIntervals.has(key)) return; 

    const interval = setInterval(() => {
      if (section.classList.contains('active')) {
        updateContent(section);
      }
    }, 330);

    updateIntervals.set(key, interval);
  }

  function stopContinuousUpdate(section) {
    const key = section?.dataset.content;
    if (updateIntervals.has(key)) {
      clearInterval(updateIntervals.get(key));
      updateIntervals.delete(key);
    }
  }

  // =============================================================
  //  Lógica para vista de consola
  // =============================================================

  const console_output = document.getElementById('console-output');
  let autoscroll = true;
  let autoscrollTimeout;

  // Detectar si el usuario ha hecho scroll manual
  console_output.addEventListener('scroll', () => {
    const nearBottom = console_output.scrollHeight - console_output.scrollTop <= console_output.clientHeight + 20;

    if (!nearBottom) {
      autoscroll = false;
      // Reiniciar el temporizador cada vez que el usuario hace scroll
      if (autoscrollTimeout) clearTimeout(autoscrollTimeout);

      autoscrollTimeout = setTimeout(() => {
        autoscroll = true;
      }, 60000); // 60 segundos sin tocar el scroll, reactivar autoscroll
    } else {
      autoscroll = true;
      if (autoscrollTimeout) clearTimeout(autoscrollTimeout);
    }
  });

  // =============================================================
  //  Lógica para selector de instancia y status del servidor
  // =============================================================

  instanceSelector.addEventListener('change', () => {
    selectedInstance = instanceSelector.value;
    const instanceNameSpan = document.querySelector('[data-instance-name]');
    let isSelected = false;
    if(selectedInstance != null && selectedInstance != undefined && typeof(selectedInstance) == "string" && selectedInstance != "") {
      isSelected = true;
    }
    if (!isSelected) {
      generalStatus.classList.remove('hidden');
      instanceStatus.classList.add('hidden');
      instanceSelector.innerHTML = '<option value="">'+ selectorSelectText +'</option>';
      instanceNameSpan.textContent = "-";
      instancesList.forEach(server => {
        const option = document.createElement('option');
        option.value = server;
        option.textContent = server;
        instanceSelector.appendChild(option);
      });
      return;
    }

    generalStatus.classList.add('hidden');
    instanceStatus.classList.remove('hidden');
    instanceSelector.innerHTML = '<option value="">'+ selectorStatusText +'</option>';
    instancesList.forEach(server => {
      const option = document.createElement('option');
      option.value = server;
      option.textContent = server;
      instanceSelector.appendChild(option);
    });

    // Restaurar selección
    instanceSelector.value = selectedInstance;
    instanceNameSpan.textContent = selectedInstance;

    fetchInstanceData(selectedInstance);
  });

  function updateInstanceStatus(data) {
    const statusText = document.querySelector('[data-key="status"]');
    const statusDot = document.querySelector('[data-key="instance_status_dot"]');

    const statusMap = {
      running: { color: "green", label: "Activo" },
      starting: { color: "orange", label: "Iniciando" },
      stopped: { color: "red", label: "Detenido" }
    };

    const current = statusMap[data.status] || { color: "gray", label: "Desconocido" };

    // Actualizar texto
    if (statusText) statusText.textContent = current.label;

    // Limpiar clases y aplicar color
    if (statusDot) {
      statusDot.className = `status-dot ${current.color}`;
    }
    const mapping = {
      backup: data.backup ?? "-",
      complete: data.complete ?? "-",
      cpu: data.cpu ?? "-",
      has_jar: data.has_jar ?? "-",
      has_run: data.has_run ?? "-",
      name: data.name ?? "-",
      player_count: data.player_count ?? "-",
      ram_used: formatMemory(data.ram_used) ?? "-",
      status: data.status ?? "-",      
    };

    for (const key in mapping) {
      const txt = document.querySelector(`[data-key="${key}"]`);
      if (txt) txt.textContent = mapping[key];
    }
  }

  function updateGeneralStatus(data) {
    const mapping = {
      instance_count: data.status?.total ?? "-",
      instances_actives: data.status?.running ?? "-",
      general_ram_used: formatMemory(data.status.ram_used) ?? "-",
      general_cpu: data.status?.cpu ?? "-",
      general_player_count: data.status?.player_count ?? "-"
    };

    for (const key in mapping) {
      const txt = document.querySelector(`[data-key="${key}"]`);
      if (txt) txt.textContent = mapping[key];
    }
  }

  function populateInstanceSelector(data) {
    let isSelected = false;
    if(selectedInstance != null && selectedInstance != undefined && typeof(selectedInstance) == "string" && selectedInstance !="") {
      isSelected = true;
    }
    if(!isSelected){
      instanceSelector.innerHTML = '<option value="">'+ selectorSelectText +'</option>';
    } else {
      instanceSelector.innerHTML = '<option value="">'+ selectorStatusText +'</option>';
    }
    instancesList = data.servers ? data.servers : instancesList;
    instancesList.forEach(server => {
      const option = document.createElement('option');
      option.value = server;
      option.textContent = server;
      instanceSelector.appendChild(option);
    });
    
      if(isSelected) instanceSelector.value = selectedInstance;
  }

  async function fetchJSON(endpoint, method, payload="") {
    const res = await fetch(endpoint, {
      method: method,
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(payload),
    });
    if (res.status === 401 || res.status === 403) {
      window.location.href = '/';
    }

    if (res.status === 500) {
      throw new Error(`Error HTTP ${res.status}`);
    }

    return await res.json();
  }

  // =============================================================
  // --- Funciones específicas de cada fetch
  // =============================================================
  async function fetchStatus(section) {

    const endpoint = section.dataset.endpoint;
    const method = section.dataset.method;

    try {
      const data = await fetchJSON(endpoint, method);
      populateInstanceSelector(data);
      updateGeneralStatus(data);
    } catch (err) {
      console.error(err);
    }
  }

  async function updateStatus(endpoint, method) {
    try {
      const data = await fetchJSON(endpoint, method);
      populateInstanceSelector(data);
      updateGeneralStatus(data);
    } catch (err) {
      console.error(err);
    }
  }

  async function fetchInstanceData(instance) {

    const endpoint = instanceSelector.dataset.endpoint;
    const method = instanceSelector.dataset.method;
    const payload = { "name":instance }

    try {
      const data = await fetchJSON(endpoint, method, payload);
      updateInstanceStatus(data);
    } catch (err) {
      console.error(err);
    }
  }

  async function actionInstanceData(instance) {

    const endpoint = instanceSelector.dataset.endpoint;
    const method = instanceSelector.dataset.method;
    const payload = { "name":instance }

    try {
      const data = await fetchJSON(endpoint, method, payload);
      updateInstanceStatus(data);
    } catch (err) {
      console.error(err);
    }
  }

  async function fetchConsole(section) {
    const consoleTab = document.querySelector('button[data-tab="console-view"]');
    // const output = document.querySelector('button[data-tab="console-view"]');
    if (!consoleTab || !consoleTab.classList.contains('active')) return;
    const endpoint = section.dataset.endpoint;
    const method = section.dataset.method;
    const payload = {"name": selectedInstance};
    try {
      const res = await fetchJSON(endpoint, method, payload);
      if (!res.success) {
        showAlert(res.message, "error");
      }else{
        console_output.textContent = res.console;
      }
    } catch (err) {
      console_output.textContent += `\n[Error] ${err.message}`;
    }
    if (autoscroll) {
      console_output.scrollTop = console_output.scrollHeight;
    }
  }

  // =============================================================
  // Forzar actualización inicial en primera carga
  // =============================================================
  const initialSection = document.querySelector('.content.active');
  if (initialSection) {
    const updateType = initialSection.dataset.update;
    if (updateType === 'on-activate') {
      updateContent(initialSection);
    } else if (updateType === 'continuous') {
      startContinuousUpdate(initialSection);
    }
  }


  // =============================================================
  //  LÓGICA PARA LOS BOTONES DEL MENÚ PRINCIPAL (div.menu-buttons)
  // =============================================================
  // const menuContainer = document.querySelector('.menu-buttons');
  // if (menuContainer) {
  //   menuContainer.addEventListener('click', async (e) => {
  //     // Si no clicamos un <button>, salimos
  //     if (e.target.tagName !== 'BUTTON') return;

  //     const btn = e.target;
  //     const action = btn.dataset.action; // por ejemplo: "start-server", "edit-config", etc.
  //     try {
  //       switch (action) {
  //         case 'start-server':
  //           const res = await fetch('/api/start-server', { method: 'POST' });
  //           if (res.status === 401) {
  //             window.location.href = '/'; // Redirige al login o página principal
  //           }
  //           alert('Solicitud de inicio enviada.');
  //           break;

  //         case 'stop-server':
  //           res = await fetch('/api/stop-server', { method: 'POST' });
  //           if (res.status === 401) {
  //             window.location.href = '/'; // Redirige al login o página principal
  //           }
  //           alert('Solicitud de detención enviada.');
  //           break;

  //         case 'restart-server':
  //           res = await fetch('/api/restart-server', { method: 'POST' });
  //           if (res.status === 401) {
  //             window.location.href = '/'; // Redirige al login o página principal
  //           }
  //           alert('Solicitud de reinicio enviada.');
  //           break;

  //         case 'edit-config':
  //           // redirigir a tu vista de edición
  //           window.location.href = '/panel/edit-config';
  //           break;

  //         case 'performance-monitor':
  //           window.location.href = '/panel/performance';
  //           break;

  //         case 'view-logs':
  //           window.location.href = '/panel/logs';
  //           break;

  //         default:
  //           break;
  //       }
  //     } catch (err) {
  //       console.error('Error en menú principal:', err);
  //       alert('Error al ejecutar la acción: ' + (btn.textContent || action));
  //     }
  //   });
  // }


  /* ===========================================================
     2) GESTIÓN DE DIÁLOGOS
     =========================================================== */
  const dialogs = {
    quick: document.getElementById('quickCommandsDialog'),
    player: document.getElementById('playerDialog'),
    plugin: document.getElementById('pluginDialog'),
    world: document.getElementById('worldDialog'),
    backup: document.getElementById('backupDialog'),
    lightning: document.getElementById('lightningCommandsDialog'),
    itemCmds: document.getElementById('itemCommandsDialog'),
    mobCmds: document.getElementById('mobCommandsDialog'),
    playerCmds: document.getElementById('playerCommandsDialog'),
    worldCmds: document.getElementById('worldCommandsDialog')
  };

  // Botones "Cerrar" de cada diálogo
  document.querySelectorAll('.close-dialog').forEach(btn => {
    btn.addEventListener('click', () => {
      btn.closest('dialog').close();
    });
  });

  // Abrir un diálogo concreto al hacer clic
  document.getElementById('openQuickCommands').addEventListener('click', () => {
    dialogs.quick.showModal();
  });
  document.getElementById('openPlayer').addEventListener('click', () => {
    dialogs.player.showModal();
  });
  document.getElementById('openPlugin').addEventListener('click', () => {
    dialogs.plugin.showModal();
  });
  document.getElementById('openWorld').addEventListener('click', () => {
    dialogs.world.showModal();
  });
  document.getElementById('openBackup').addEventListener('click', () => {
    dialogs.backup.showModal();
  });
  document.getElementById('openLightningCommands').addEventListener('click', () => {
    dialogs.lightning.showModal();
  });


  // Opción 5: Comando personalizado
  document.getElementById('customCommandBtn').addEventListener('click', async () => {
    const cmd = prompt('Escribe tu comando personalizado:');
    if (!cmd) return;
    try {
      const res = await fetch('/api/send-command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: cmd })
      });
      if (res.status === 401 || res.status === 403) {
            window.location.href = '/';
          }
      alert('Comando enviado: ' + cmd);
    } catch (err) {
      console.error(err);
      alert('Error al enviar el comando.');
    }
  });

  // ===========================================================
  //  Server Instances actions
  // ===========================================================

  document.querySelectorAll("button[data-instance-action]").forEach(btn => {
    btn.addEventListener("click", async () => {
      if (!selectedInstance) {
        alert("Selecciona una instancia primero.");
        return;
      }
      const action = btn.dataset.action
      const method = btn.dataset.method;
      const endpoint = btn.dataset.endpoint;
      const payload = { name: selectedInstance };

      try {
        if (action=="refresh") {
          fetchInstanceData(selectedInstance);
        } else {
          const res = await fetchJSON(endpoint, method, payload);
          if (res.status === 401 || res.status === 403) {
            window.location.href = '/';
          }
          if (res.success) {
            fetchInstanceData(selectedInstance);
            showAlert(res.message, "success");
          } else {
            showAlert(res.message, "error");
            // showAlert(`Acción '${action}' no ejecutada`, "error");
          }
        }
        
      } catch (err) {
        alert(`Error ejecutando '${action}'`);
        console.error(err);
      }
    });
  });

  // ===========================================================
  //  PLayer Actiones
  // ===========================================================
  function incluye(texto, palabras) {
    return palabras.some(palabra => texto.includes(palabra));
  }
  document.querySelectorAll('button[data-action^="data-player-"]').forEach(btn => {
    btn.addEventListener('click', async () => {
      const endpoint = btn.dataset.endpoint;
      const action = btn.dataset.action;
      let player = "";
      let reason = "";
      try {
        if (incluye(action, ["kick","ban"])) {
          player = prompt('Ingresa nombre de jugador:');
          reason = prompt('Ingresa la razón:');
        }
        const payload = {name: selectedInstance, player: player, reason: reason};
        const res = await fetchJSON(endpoint, method, payload);
        if (res.status === 401 || res.status === 403) {
          window.location.href = '/';
        }
        if (res.success) {
          showAlert(res.message, "success");
        } else {
          showAlert(res.message, "error");
        }
      }
      catch (err) {
        console.error('Error en comando', endpoint, err);
        showAlert(err, "error");
      }
      finally {
        btn.closest('dialog')?.close();
      }
    });
  });

  document.querySelectorAll("button[data-unique]").forEach(btn => {
    btn.addEventListener("click", async () => {

      const fnName = btn.dataset.function;
      const fn = sectionFunctions[fnName];
      if (typeof fn === 'function') {
       
      } else {
        return showAlert(`Función "${fnName}" no está registrada.`, "Error");
      }
      const method = btn.dataset.method;
      const endpoint = btn.dataset.endpoint;

      try {
        fn(endpoint, method);
      } catch (err) {
        console.error(err);
      }
    });
  });

  // ===========================================================
  //  CERRAR DIÁLOGOS
  // ===========================================================
  document.querySelectorAll('.close-dialog').forEach(btn => {
    btn.addEventListener('click', () => {
      btn.closest('dialog').close();
    });
  });

});