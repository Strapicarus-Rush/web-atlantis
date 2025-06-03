document.addEventListener('DOMContentLoaded', () => {
  const tabs = document.querySelectorAll('.tab');
  const sections = document.querySelectorAll('.content');

  function switchTab(newTab) {
    const target = newTab.dataset.tab;
    const newSection = document.querySelector(`[data-content="${target}"]`);
    const currentSection = document.querySelector('.content.active');

    if (newSection === currentSection) return;

    const direction = Array.from(sections).indexOf(newSection) > Array.from(sections).indexOf(currentSection)
      ? 'left' : 'rigth';
    console.log(direction);
    currentSection.classList.remove('active');
    currentSection.classList.add(`fade-out-${direction}`);

    newSection.classList.add('active');

    setTimeout(() => {
      currentSection.classList.remove(`fade-out-${direction}`);
    }, 400);
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
      console.log('Datos recibidos:', json);
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

      if (type === 'bar') drawBarChart(ctx, data, labels, progress);
      else drawLineChart(ctx, data, labels, progress);

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
});