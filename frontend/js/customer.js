const session = requireCustomerLogin();
if (session) document.getElementById("whoami").textContent = session.name + " (#" + session.id + ")";

function logout() { clearSession(); window.location.href = "index.html"; }

// ---------------- Tabs ----------------
document.querySelectorAll(".tab").forEach(tab => {
  tab.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach(c => (c.style.display = "none"));
    tab.classList.add("active");
    document.getElementById("tab-" + tab.dataset.tab).style.display = "block";
    if (tab.dataset.tab === "history") loadHistory();
  });
});

// ---------------- Book form setup ----------------
async function setupBookForm() {
  const cities = await apiGet("/api/cities");
  const pickupSel = document.getElementById("pickupCity");
  const destSel = document.getElementById("destCity");
  cities.forEach(c => {
    pickupSel.innerHTML += `<option value="${c}">${c}</option>`;
    destSel.innerHTML += `<option value="${c}">${c}</option>`;
  });
  destSel.selectedIndex = 1;

  const pkgSelect = document.getElementById("pkgType");
  ["Documents", "Electronics", "Medicines", "Groceries", "Clothing", "Spare Parts"].forEach(p => {
    pkgSelect.innerHTML += `<option value="${p}">${p}</option>`;
  });

  pickupSel.addEventListener("change", previewRoute);
  destSel.addEventListener("change", previewRoute);
  previewRoute();
}

async function previewRoute() {
  const pickup = document.getElementById("pickupCity").value;
  const dest = document.getElementById("destCity").value;
  const preview = document.getElementById("routePreview");
  if (!pickup || !dest || pickup === dest) { preview.textContent = "Pickup and destination must differ."; return; }
  try {
    const r = await apiGet(`/api/route?pickup=${encodeURIComponent(pickup)}&destination=${encodeURIComponent(dest)}`);
    preview.innerHTML = `<b>${r.distanceKm.toFixed(1)} km</b> via ${r.path.join(" → ")}`;
    DDX.renderRouteViz("routeVizBox", r.path, r.distanceKm);

    const tipBox = document.getElementById("aiRouteTip");
    tipBox.style.display = "block";
    tipBox.textContent = "🤖 Fetching AI route tip…";
    apiPost("/api/ai/route-tip", { pickup, destination: dest, distance: r.distanceKm })
      .then(res => (tipBox.textContent = "🤖 " + res.result));
  } catch { preview.textContent = "No route currently connects these cities."; document.getElementById("routeVizBox").innerHTML = ""; }
}

document.getElementById("bookForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const pickup = document.getElementById("pickupCity").value;
  const destination = document.getElementById("destCity").value;
  const weight = parseFloat(document.getElementById("pkgWeight").value);
  const priority = parseInt(document.getElementById("priority").value, 10);
  const pkg = document.getElementById("pkgType").value;
  try {
    const res = await apiPost("/api/book", { customerId: session.id, package: pkg, weight, pickup, destination, priority });
    showToast(`Delivery #${res.delivery.id} booked! Status: ${res.delivery.status}`);
    const predictBox = document.getElementById("aiTimePredict");
    predictBox.style.display = "block";
    predictBox.textContent = "🤖 Predicting delivery time…";
    apiPost("/api/ai/predict-time", { distance: res.delivery.distance, speed: 45, weather: "clear" })
      .then(r => (predictBox.textContent = "🤖 " + r.result));
  } catch (err) { showToast(err.message, "error"); }
});

// ---------------- History ----------------
async function loadHistory() {
  const rows = await apiGet(`/api/deliveries/history?customerId=${session.id}`);
  const tbody = document.getElementById("historyBody");
  tbody.innerHTML = rows.map(d => `
    <tr>
      <td>#${d.id}</td><td>${d.package}</td><td>${d.pickup} → ${d.destination}</td>
      <td>${d.weight} kg</td><td><span class="badge ${d.status}">${d.status}</span></td>
      <td>${d.eta ? Math.round(d.eta) + " min" : "-"}</td>
      <td>${fmtDate(d.createdAt)}</td>
      <td>${(d.status === "PENDING" || d.status === "ASSIGNED") ?
            `<button class="btn small danger" onclick="cancelDelivery(${d.id})">Cancel</button>` : ""}</td>
    </tr>`).join("") || `<tr><td colspan="8" class="muted">No deliveries yet.</td></tr>`;
}

async function cancelDelivery(id) {
  try {
    await apiPost("/api/cancel", { deliveryId: id, customerId: session.id, role: "customer" });
    showToast("Delivery cancelled.");
    loadHistory();
  } catch (err) { showToast(err.message, "error"); }
}

// ---------------- Track ----------------
async function trackPackage() {
  const id = document.getElementById("trackId").value;
  const box = document.getElementById("trackResult");
  if (!id) return;
  try {
    const all = await apiGet("/api/deliveries");
    const d = all.find(x => x.id === parseInt(id, 10));
    if (!d) { box.innerHTML = `<div class="sub">No delivery found with that ID.</div>`; return; }
    box.innerHTML = `
      <div class="glass-card" style="background:#0d1526">
        <div><b>Delivery #${d.id}</b> — <span class="badge ${d.status}">${d.status}</span></div>
        <div class="sub" style="margin-top:8px">Route: ${d.pickup} → ${d.destination} (${d.distance.toFixed(1)} km)</div>
        <div class="sub">Drone assigned: ${d.droneId === -1 ? "Not yet assigned" : "#" + d.droneId}</div>
        <div class="sub">Estimated time: ${d.eta ? Math.round(d.eta) + " minutes" : "Pending assignment"}</div>
        <div class="sub">Booked: ${fmtDate(d.createdAt)}</div>
        <div id="trackTimeline"></div>
      </div>`;
    DDX.renderTrackingTimeline("trackTimeline", d);
  } catch (err) { box.innerHTML = `<div class="sub">${err.message}</div>`; }
}

// ---------------- Chat ----------------
async function sendChat() {
  const input = document.getElementById("chatInput");
  const msg = input.value.trim();
  if (!msg) return;
  const win = document.getElementById("chatWindow");
  win.innerHTML += `<div class="chat-msg user">You: ${msg}</div>`;
  input.value = "";
  win.scrollTop = win.scrollHeight;
  try {
    const r = await apiPost("/api/ai/chat", { message: msg });
    win.innerHTML += `<div class="chat-msg bot">🤖 ${r.result}</div>`;
  } catch { win.innerHTML += `<div class="chat-msg bot">🤖 Sorry, support is unavailable right now.</div>`; }
  win.scrollTop = win.scrollHeight;
}
document.getElementById("chatInput").addEventListener("keydown", e => { if (e.key === "Enter") sendChat(); });

setupBookForm();
