function getCookie(name) {
  const match = document.cookie.match(new RegExp('(^| )' + name + '=([^;]+)'));
  return match ? match[2] : null;
}

function applyInitialTheme() {
  const cookieTheme = getCookie("theme");
  const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  document.body.className = cookieTheme || systemTheme;
  const html = document.documentElement;
  html.setAttribute('data-theme', document.body.className);
}

function setTheme() {
  const html = document.documentElement;
  html.setAttribute(
      'data-theme',
      html.getAttribute('data-theme') === 'light' ? 'dark' : 'light'
    );
  const theme = html.getAttribute('data-theme');
  document.body.className = theme;
  document.cookie = `theme=${theme}; SameSite=strict; path=/; max-age=31536000`;
}

document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('toggleTheme').addEventListener('click', () => {
    setTheme();
  });
});

applyInitialTheme();