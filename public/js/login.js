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
    window.location.href = "/panel";
  } else {
    status.textContent = "Error: Usuario o contraseña incorrectos.";
  }
});

window.addEventListener('DOMContentLoaded', () => {
    document.getElementById('user').focus();
  });