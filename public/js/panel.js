document.addEventListener('DOMContentLoaded', () => {
  const tabs = document.querySelectorAll('.tab');
  const sections = document.querySelectorAll('.content');

  function switchTab(newTab) {
    const target = newTab.dataset.tab;
    const newSection = document.querySelector(`[data-content="${target}"]`);
    const currentSection = document.querySelector('.content.active');

    if (newSection === currentSection) return;

    // const direction = Array.from(sections).indexOf(newSection) > Array.from(sections).indexOf(currentSection)
    //   ? 'left' : 'right';
    currentSection.classList.remove('active');
    currentSection.classList.add(`fade-out`);
    // newSection.classList.add(`fade-in-${direction}`);

    setTimeout(() => {
      currentSection.classList.remove(`fade-out`);
      // newSection.classList.remove(`fade-in-${direction}`);
      newSection.classList.add('active');
    }, 300);
  }

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      switchTab(tab);
    });
  });

  // document.getElementById('loadData').addEventListener('click', async (e) => {
  //   const endpoint = e.target.dataset.endpoint;
  //   try {
  //     const response = await fetch(endpoint);
  //     const json = await response.json();
  //   } catch (error) {
  //     console.error('Error al obtener datos:', error);
  //   }
  // });

  const canvas = document.getElementById('chartCanvas');
  const ctx = canvas.getContext('2d');
  let chartType = canvas.dataset.type;

  function drawAxes(ctx, width, height, padding, maxVal, steps) {
    const axisColor = '#888';
    ctx.strokeStyle = axisColor;
    ctx.fillStyle = axisColor;
    ctx.font = '12px sans-serif';

    ctx.beginPath();
    ctx.moveTo(padding, padding);
    ctx.lineTo(padding, height - padding);
    ctx.lineTo(width - padding, height - padding);
    ctx.stroke();

    for (let i = 0; i <= steps; i++) {
      const val = Math.round((maxVal / steps) * i);
      const y = height - padding - ((height - 2 * padding) / steps) * i;
      ctx.fillText(val, 5, y + 3);
      ctx.beginPath();
      ctx.moveTo(padding, y);
      ctx.lineTo(width - padding, y);
      ctx.strokeStyle = '#ccc';
      ctx.stroke();
    }
  }

  function getAccentColors() {
    const styles = getComputedStyle(document.documentElement);
    return {
      fill: styles.getPropertyValue('--clr-accent1').trim(),
      stroke: styles.getPropertyValue('--clr-accent2').trim(),
      text: styles.getPropertyValue('--clr-text').trim()
    };
  }

  function drawBarChart(ctx, data, labels, progress = 1) {
    const { width, height } = canvas;
    const padding = 40;
    const maxVal = Math.max(...data);
    const barWidth = (width - 2 * padding) / data.length;
    const colors = getAccentColors();

    ctx.clearRect(0, 0, width, height);
    drawAxes(ctx, width, height, padding, maxVal, 5);

    data.forEach((val, i) => {
      const animatedVal = val * progress;
      const x = padding + i * barWidth + 10;
      const y = height - padding - (animatedVal / maxVal) * (height - 2 * padding);
      const h = (animatedVal / maxVal) * (height - 2 * padding);

      // Draw bar
      ctx.fillStyle = colors.fill;
      ctx.fillRect(x, y, barWidth - 20, h);

      // Labels below
      ctx.fillStyle = colors.text;
      ctx.fillText(labels[i], x + (barWidth - 20) / 2 - 5, height - padding + 15);

      // Value above bar
      if (progress > 0.95) {
        ctx.fillText(val, x + (barWidth - 20) / 2 - 5, y - 8);
      }
    });
  }

  function drawLineChart(ctx, data, labels, progress = 1) {
    const { width, height } = canvas;
    const padding = 40;
    const maxVal = Math.max(...data);
    const stepX = (width - 2 * padding) / (data.length - 1);
    const colors = getAccentColors();

    ctx.clearRect(0, 0, width, height);
    drawAxes(ctx, width, height, padding, maxVal, 5);

    ctx.strokeStyle = colors.stroke;
    ctx.lineWidth = 2;
    ctx.beginPath();

    const points = data.map((val, i) => {
      const x = padding + i * stepX;
      const y = height - padding - (val / maxVal) * (height - 2 * padding);
      return { x, y, val };
    });

    for (let i = 0; i < points.length; i++) {
      const p = i / (points.length - 1);
      if (p > progress) break;

      if (i === 0) ctx.moveTo(points[i].x, points[i].y);
      else ctx.lineTo(points[i].x, points[i].y);
    }
    ctx.stroke();

    points.forEach((point, i) => {
      if ((i / (points.length - 1)) <= progress) {
        ctx.fillStyle = colors.fill;
        ctx.beginPath();
        ctx.arc(point.x, point.y, 4, 0, Math.PI * 2);
        ctx.fill();

        // Label and value
        ctx.fillStyle = colors.text;
        ctx.fillText(labels[i], point.x - 5, height - padding + 15);
        if (progress > 0.95) {
          ctx.fillText(point.val, point.x - 5, point.y - 10);
        }
      }
    });
  }

  function animateChart(type, data, labels, duration = 800) {
    const start = performance.now();

    function animate(now) {
      const elapsed = now - start;
      const progress = Math.min(elapsed / duration, 1);

      if (type === 'bar') {
        drawBarChart(ctx, data, labels, progress);
      }else{
        drawLineChart(ctx, data, labels, progress);
      }

      if (progress < 1) requestAnimationFrame(animate);
    }

    requestAnimationFrame(animate);
  }

  function renderChart(type, data, labels) {
    chartType = type;
    canvas.dataset.type = type;
    animateChart(type, data, labels);
  }

  // Datos iniciales
  let chartData = [12, 25, 9, 16, 30, 21];
  let chartLabels = ['A', 'B', 'C', 'D', 'E', 'F'];

  renderChart(chartType, chartData, chartLabels);


  const output = document.getElementById('console-output');
  let autoscroll = true;
  let autoscrollTimeout;

  // Detectar si el usuario ha hecho scroll manual
  output.addEventListener('scroll', () => {
    const nearBottom = output.scrollHeight - output.scrollTop <= output.clientHeight + 20;

    if (!nearBottom) {
      autoscroll = false;
      // Reiniciar el temporizador cada vez que el usuario hace scroll
      if (autoscrollTimeout) clearTimeout(autoscrollTimeout);

      autoscrollTimeout = setTimeout(() => {
        autoscroll = true;
      }, 60000); // 60 segundos sin tocar el scroll -> reactivar autoscroll
    } else {
      autoscroll = true;
      if (autoscrollTimeout) clearTimeout(autoscrollTimeout);
    }
  });

  async function fetchConsole() {
    const consoleTab = document.querySelector('button[data-tab="console-view"]');
    
    // Solo ejecutar si la pestaña está activa
    if (!consoleTab || !consoleTab.classList.contains('active')) return;

    try {
      const res = await fetch('/api/console', { method: 'REPORT' });
      if (res.status === 401) {
        window.location.href = '/'; // Redirige al login o página principal
      }
      if (!res.ok) throw new Error(`Error ${res.status}`);
      const data = await res.text();
      output.textContent = data;
    } catch (err) {
      output.textContent += `\n[Error] ${err.message}`;
    }
    if (autoscroll) {
      output.scrollTop = output.scrollHeight; // Auto-scroll al final
    }
  }

  // Actualiza cada 2 segundos solo si la pestaña "Consola" está activa
  setInterval(fetchConsole, 2000);
  fetchConsole();

  // Botón: Cambiar tipo de gráfico
  document.getElementById('toggleChartType').addEventListener('click', () => {
    chartType = chartType === 'bar' ? 'line' : 'bar';
    document.getElementById('toggleChartType').textContent = chartType === 'bar'
      ? 'Cambiar a Línea'
      : 'Cambiar a Barras';

    renderChart(chartType, chartData, chartLabels);
  });

  // Botón: Actualizar gráfico desde endpoint
  document.getElementById('refreshChart').addEventListener('click', async (e) => {
    const endpoint = e.target.dataset.endpoint;
    try {
      const res = await fetch(endpoint);
      if (res.status === 401) {
        return window.location.href = '/'; // Redirige al login o página principal
      }
      const json = await res.json();

      // Se espera que json tenga: { values: [..], labels: [..] }
      chartData = json.values;
      chartLabels = json.labels;

      renderChart(chartType, chartData, chartLabels);
    } catch (error) {
      console.error('Error al actualizar gráfico:', error);
    }
  });

  // =============================================================
  //  LÓGICA PARA LOS BOTONES DEL MENÚ PRINCIPAL (div.menu-buttons)
  // =============================================================
  const menuContainer = document.querySelector('.menu-buttons');
  if (menuContainer) {
    menuContainer.addEventListener('click', async (e) => {
      // Si no clicamos un <button>, salimos
      if (e.target.tagName !== 'BUTTON') return;

      const btn = e.target;
      const action = btn.dataset.action; // por ejemplo: "start-server", "edit-config", etc.
      try {
        switch (action) {
          case 'start-server':
            const res = await fetch('/api/start-server', { method: 'POST' });
            if (res.status === 401) {
              window.location.href = '/'; // Redirige al login o página principal
            }
            alert('Solicitud de inicio enviada.');
            break;

          case 'stop-server':
            res = await fetch('/api/stop-server', { method: 'POST' });
            if (res.status === 401) {
              window.location.href = '/'; // Redirige al login o página principal
            }
            alert('Solicitud de detención enviada.');
            break;

          case 'restart-server':
            res = await fetch('/api/restart-server', { method: 'POST' });
            if (res.status === 401) {
              window.location.href = '/'; // Redirige al login o página principal
            }
            alert('Solicitud de reinicio enviada.');
            break;

          case 'edit-config':
            // redirigir a tu vista de edición
            window.location.href = '/panel/edit-config';
            break;

          case 'performance-monitor':
            window.location.href = '/panel/performance';
            break;

          case 'view-logs':
            window.location.href = '/panel/logs';
            break;

          // Los botones sin data-action (p.ej. id="openPlayerMgmt", etc.) 
          // no tienen acción aquí; su listener abre un diálogo directamente.
          default:
            break;
        }
      } catch (err) {
        console.error('Error en menú principal:', err);
        alert('Error al ejecutar la acción: ' + (btn.textContent || action));
      }
    });
  }

  // const viewportInfo = document.getElementById('viewport-info');

  // function updateViewportSize() {
  //   const width = window.innerWidth;
  //   const height = window.innerHeight;
  //   viewportInfo.textContent = `Viewport: ${width} x ${height}`;
  // }

  // // Inicializar
  // updateViewportSize();

  // // Actualizar al cambiar tamaño
  // window.addEventListener('resize', updateViewportSize);


  /* ===========================================================
     2) GESTIÓN DE DIÁLOGOS (Modales)
     =========================================================== */
  const dialogs = {
    quick: document.getElementById('quickCommandsDialog'),
    playerMgmt: document.getElementById('playerMgmtDialog'),
    pluginMgmt: document.getElementById('pluginMgmtDialog'),
    worldMgmt: document.getElementById('worldMgmtDialog'),
    backupMgmt: document.getElementById('backupMgmtDialog'),
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
  document.getElementById('openPlayerMgmt').addEventListener('click', () => {
    dialogs.playerMgmt.showModal();
  });
  document.getElementById('openPluginMgmt').addEventListener('click', () => {
    dialogs.pluginMgmt.showModal();
  });
  document.getElementById('openWorldMgmt').addEventListener('click', () => {
    dialogs.worldMgmt.showModal();
  });
  document.getElementById('openBackupMgmt').addEventListener('click', () => {
    dialogs.backupMgmt.showModal();
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
      alert('Comando enviado: ' + cmd);
    } catch (err) {
      console.error(err);
      alert('Error al enviar el comando.');
    }
  });

  // ===========================================================
  //  ENVÍO DE PETICIONES DESDE CADA BOTÓN .action-btn
  // ===========================================================
  document.querySelectorAll('.action-btn').forEach(btn => {
    btn.addEventListener('click', async () => {
      const endpoint = btn.dataset.endpoint;
      try {
        // Si la acción requiere parámetro extra (por ejemplo,nombre usuario a kickear), preguntar aquí
        if (endpoint.endsWith('/player/kick')) {
          const nombre = prompt('Ingresa nombre de jugador:');
          if (!nombre) return;
          const res = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ player: nombre })
          });
          if (res.status === 401) {
            window.location.href = '/'; // Redirige al login o página principal
          }
          alert(`Kick de ${nombre} enviado.`);
        }
        else {
          // Para la mayoría, alcanza con POST sin cuerpo adicional
          await fetch(endpoint, { method: 'POST' });
          alert('Comando enviado: ' + btn.textContent.trim());
        }
      }
      catch (err) {
        console.error('Error en comando', endpoint, err);
        alert('Error al ejecutar: ' + btn.textContent.trim());
      }
      finally {
        // Cierra el modal que contenga este botón
        btn.closest('dialog')?.close();
      }
    });
  });


  // ===========================================================
  //  Status Server Instances
  // ===========================================================
  const container = document.getElementById("instancesContainer");
  const infoContainer = document.querySelector('[data-bind="server-info"]');
  const nameDisplay = document.querySelector('[data-instance-name]');
  let instances = {};
  let selectedId = null;

  async function loadInstances(endpoint) {
    try {
      const res = await fetch(endpoint);
      const data = await res.json();
      instances = {};
      container.innerHTML = "";

      data.forEach(item => {
        instances[item.id] = item;

        const card = document.createElement("div");
        card.classList.add("instance-card");
        card.dataset.id = item.id;
        card.dataset.screenName = item.screen_name;
        card.dataset.serverPath = item.server_path;
        card.dataset.status = item.status;
        card.dataset.ramUsed = item.ram_used;
        card.dataset.playerCount = item.player_count;
        card.dataset.backupPath = item.backup_path;

        card.innerHTML = `
          <p><strong>${item.screen_name}</strong></p>
          <p>Estado: ${item.status}</p>
          <p>Jugadores: ${item.player_count}</p>
        `;

        card.addEventListener("click", () => {
          selectInstance(item.id);
        });

        container.appendChild(card);
      });

      if (data.length > 0) {
        selectInstance(data[0].id);
      }
    } catch (err) {
      console.error("Error cargando instancias:", err);
    }
  }

  function selectInstance(id) {
    if (selectedId === id) return; // No cambia

    selectedId = id;

    // Actualizar selección visual
    container.querySelectorAll(".instance-card").forEach(card => {
      card.classList.toggle("selected", card.dataset.id === id);
    });

    const data = instances[id];
    if (!data) return;

    // Actualizar info detallada
    infoContainer.querySelectorAll("[data-key]").forEach(el => {
      const key = el.dataset.key;
      let val = data[toCamelCase(key)] ?? data[key] ?? "-";

      if (key === "player_count") val = `${val} conectados`;
      if (key === "backup_path") val = val || "No configurado";

      el.textContent = val;
    });

    // Actualizar header
    if (nameDisplay) {
      nameDisplay.textContent = data.screen_name;
    }
  }

  // Convierte snake_case o lowercase a camelCase para asegurar acceso a propiedades JS
  function toCamelCase(str) {
    return str.replace(/_([a-z])/g, g => g[1].toUpperCase());
  }

  // Botones de acción
  document.querySelectorAll("button[data-action]").forEach(btn => {
    btn.addEventListener("click", async () => {
      if (!selectedId) {
        alert("Selecciona una instancia primero.");
        return;
      }
      const action = btn.dataset.action;
      let endpoint = btn.dataset.endpoint.replace("{id}", selectedId);

      try {
        const res = await fetch(endpoint, {
          method: action === "refresh" ? "GET" : "POST",
        });
        if (!res.ok) throw new Error();

        if (action === "refresh") {
          await loadInstances(container.dataset.endpoint);
        } else {
          alert(`Acción '${action}' ejecutada correctamente`);
        }
      } catch (err) {
        alert(`Error ejecutando '${action}'`);
        console.error(err);
      }
    });
  });

  // Auto cargar
  if (container.dataset.autoLoad === "true") {
    loadInstances(container.dataset.endpoint);
  }
  // const selector = document.getElementById("instanceSelector");
  // const infoContainer = document.querySelector('[data-bind="server-info"]');
  // const nameDisplay = document.querySelector('[data-instance-name]');
  // let instances = {};

  // async function loadInstances(endpoint) {
  //   try {
  //     const res = await fetch(endpoint);
  //     const data = await res.json();
  //     const idField = selector.dataset.fieldId || "id";
  //     const labelField = selector.dataset.fieldLabel || "screen_name";
  //     selector.innerHTML = "";

  //     data.forEach(item => {
  //       instances[item[idField]] = item;

  //       const option = document.createElement("option");
  //       option.value = item[idField];
  //       option.textContent = item[labelField];
  //       selector.appendChild(option);
  //     });

  //     if (data.length > 0) updateInfo(data[0][idField]);
  //   } catch (err) {
  //     console.error("Error al cargar instancias:", err);
  //   }
  // }

  // function updateInfo(id) {
  //   const data = instances[id];
  //   if (!data) return;

  //   infoContainer.querySelectorAll("[data-key]").forEach(el => {
  //     const key = el.dataset.key;
  //     let val = data[key];
  //     if (key === "player_count") val = `${val} conectados`;
  //     if (key === "backup_path") val = val || "No configurado";
  //     el.textContent = val;
  //   });

  //   selector.dataset.activeId = id;
  //   if (nameDisplay) nameDisplay.textContent = data.screen_name;
  // }

  // selector.addEventListener("change", () => {
  //   updateInfo(selector.value);
  // });

  // document.querySelectorAll("button[data-action]").forEach(btn => {
  //   btn.addEventListener("click", async () => {
  //     const id = selector.dataset.activeId;
  //     const endpoint = btn.dataset.endpoint.replace("{id}", id);
  //     const action = btn.dataset.action;

  //     try {
  //       const res = await fetch(endpoint, {
  //         method: action === "refresh" ? "GET" : "POST"
  //       });
  //       if (!res.ok) throw new Error();

  //       if (action === "refresh") {
  //         await loadInstances(selector.dataset.endpoint);
  //       } else {
  //         alert(`Acción '${action}' ejecutada correctamente`);
  //       }
  //     } catch (err) {
  //       alert(`Error ejecutando '${action}'`);
  //       console.error(err);
  //     }
  //   });
  // });

  // if (selector.dataset.autoLoad === "true") {
  //   loadInstances(selector.dataset.endpoint);
  // }

  // ===========================================================
  //  CERRAR DIÁLOGOS
  // ===========================================================
  document.querySelectorAll('.close-dialog').forEach(btn => {
    btn.addEventListener('click', () => {
      btn.closest('dialog').close();
    });
  });

});