/*******************************************************************************
 * api.js
 * Thin fetch() wrapper around the C++ HTTP API exposed by drone_server.
 * Every business decision (booking, assignment, routing, reports, AI calls)
 * happens on the C++ backend -- this file only sends/receives JSON.
 ******************************************************************************/
const API_BASE = ""; // same-origin, served by drone_server on :8080

async function apiGet(path) {
  const res = await fetch(API_BASE + path);
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.message || "Request failed");
  return data;
}

async function apiPost(path, body) {
  const res = await fetch(API_BASE + path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body || {}),
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.message || "Request failed");
  return data;
}

function showToast(message, type = "success") {
  let toast = document.getElementById("global-toast");
  if (!toast) {
    toast = document.createElement("div");
    toast.id = "global-toast";
    toast.className = "toast";
    document.body.appendChild(toast);
  }
  toast.textContent = message;
  toast.className = "toast show " + type;
  clearTimeout(window.__toastTimer);
  window.__toastTimer = setTimeout(() => (toast.className = "toast"), 3500);
}

function fmtDate(unixSeconds) {
  const d = new Date(unixSeconds * 1000);
  return d.toLocaleString();
}

function getSession() {
  try { return JSON.parse(localStorage.getItem("dds_customer") || "null"); }
  catch { return null; }
}
function setSession(customer) {
  localStorage.setItem("dds_customer", JSON.stringify(customer));
}
function clearSession() {
  localStorage.removeItem("dds_customer");
}
function requireCustomerLogin() {
  const session = getSession();
  if (!session) { window.location.href = "index.html"; }
  return session;
}
