document.addEventListener('DOMContentLoaded', () => {
  const tabs = document.querySelectorAll('.tab');
  const sections = document.querySelectorAll('.content');

  function switchTab(newTab) {
    const target = newTab.dataset.tab;
    const newSection = document.querySelector(`[data-content="${target}"]`);
    const currentSection = document.querySelector('.content.active');

    if (newSection === currentSection) return;

    const direction = Array.from(sections).indexOf(newSection) > Array.from(sections).indexOf(currentSection)
      ? 'left' : 'right';
    currentSection.classList.remove('active');
    currentSection.classList.add(`fade-out-${direction}`);
    newSection.classList.add(`fade-in-${direction}`);

    setTimeout(() => {
      currentSection.classList.remove(`fade-out-${direction}`);
      newSection.classList.remove(`fade-in-${direction}`);
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

  document.getElementById('loadData').addEventListener('click', async (e) => {
    const endpoint = e.target.dataset.endpoint;
    try {
      const response = await fetch(endpoint);
      const json = await response.json();
    } catch (error) {
      console.error('Error al obtener datos:', error);
    }
  });

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
      const response = await fetch(endpoint);
      const json = await response.json();

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
            await fetch('/api/start-server', { method: 'POST' });
            alert('Solicitud de inicio enviada.');
            break;

          case 'stop-server':
            await fetch('/api/stop-server', { method: 'POST' });
            alert('Solicitud de detención enviada.');
            break;

          case 'restart-server':
            await fetch('/api/restart-server', { method: 'POST' });
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
      await fetch('/api/send-command', {
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
        // Si la acción requiere parámetro extra (por ejemplo, kickear), preguntar aquí
        if (endpoint.endsWith('/player/kick')) {
          const nombre = prompt('Ingresa nombre de jugador a kickear:');
          if (!nombre) return;
          await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ player: nombre })
          });
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
  //  CERRAR DIÁLOGOS con botón .close-dialog (ya existía pero verifícalo)
  // ===========================================================
  document.querySelectorAll('.close-dialog').forEach(btn => {
    btn.addEventListener('click', () => {
      btn.closest('dialog').close();
    });
  });




});