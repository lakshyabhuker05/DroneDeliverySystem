const ADMIN_PASSCODE = "admin123"; // demo-only client-side gate; real authorization happens per-request on the server (role: "admin")

function checkAdmin() {
  const pass = document.getElementById("adminPass").value;
  if (pass === ADMIN_PASSCODE) {
    document.getElementById("adminGate").style.display = "none";
    document.getElementById("adminPanel").style.display = "block";
    initAdmin();
  } else {
    showToast("Incorrect passcode", "error");
  }
}

document.querySelectorAll(".tab").forEach(tab => {
  tab.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach(c => (c.style.display = "none"));
    tab.classList.add("active");
    document.getElementById("tab-" + tab.dataset.tab).style.display = "block";
    if (tab.dataset.tab === "deliveries") loadDeliveries();
    if (tab.dataset.tab === "customers") loadCustomers();
  });
});

let adminFleetScene = null;

async function initAdmin() {
  const cities = await apiGet("/api/cities");
  const sel = document.getElementById("dLocation");
  cities.forEach(c => (sel.innerHTML += `<option value="${c}">${c}</option>`));
  await loadDrones();
  if (!adminFleetScene) adminFleetScene = DDX.initFleetScene("adminFleetScene", { pollMs: 10000 });
}

async function loadDrones() {
  const drones = await apiGet("/api/drones");
  document.getElementById("droneBody").innerHTML = drones.map(d => `
    <tr>
      <td>#${d.id}</td><td>${d.model}</td><td><span class="badge ${d.status}">${d.status}</span></td>
      <td>${d.battery}%</td><td>${d.speed} km/h</td><td>${d.maxWeight} kg</td><td>${d.location}</td>
      <td><button class="btn small danger" onclick="removeDrone(${d.id})">Remove</button></td>
    </tr>`).join("");

  const available = drones.filter(d => d.status === "AVAILABLE").length;
  document.getElementById("fleetSummary").innerHTML =
    `Total drones: <b>${drones.length}</b><br>Available now: <b>${available}</b><br>Avg battery: <b>${(drones.reduce((s,d)=>s+d.battery,0)/(drones.length||1)).toFixed(0)}%</b>`;
  if (adminFleetScene) adminFleetScene.refresh();
}

document.getElementById("addDroneForm").addEventListener("submit", async e => {
  e.preventDefault();
  try {
    await apiPost("/api/drones/add", {
      model: document.getElementById("dModel").value,
      battery: parseFloat(document.getElementById("dBattery").value),
      speed: parseFloat(document.getElementById("dSpeed").value),
      maxWeight: parseFloat(document.getElementById("dMaxWeight").value),
      location: document.getElementById("dLocation").value,
    });
    showToast("Drone added to fleet.");
    document.getElementById("addDroneForm").reset();
    loadDrones();
  } catch (err) { showToast(err.message, "error"); }
});

async function removeDrone(id) {
  if (!confirm("Remove drone #" + id + "?")) return;
  await apiPost("/api/drones/remove", { id });
  showToast("Drone removed.");
  loadDrones();
}

async function undoLast() {
  const r = await apiPost("/api/undo", {});
  showToast(r.success ? "Last action undone." : "Nothing to undo.", r.success ? "success" : "error");
  loadDrones(); loadDeliveries();
}

async function assignNext() {
  const r = await apiPost("/api/assign-next", {});
  showToast(r.success ? "Next queued delivery assigned!" : "No pending delivery or no drone available.", r.success ? "success" : "error");
  loadDeliveries(); loadDrones();
}

async function loadDeliveries() {
  const rows = await apiGet("/api/deliveries");
  document.getElementById("deliveryBody").innerHTML = rows.map(d => `
    <tr>
      <td>#${d.id}</td><td>#${d.customerId}</td><td>${d.package}</td><td>${d.pickup} → ${d.destination}</td>
      <td>${d.priority}</td><td>${d.droneId === -1 ? "-" : "#" + d.droneId}</td>
      <td><span class="badge ${d.status}">${d.status}</span></td>
      <td>
        ${d.status === "ASSIGNED" ? `<button class="btn small" onclick="markInTransit(${d.id})">In Transit</button>` : ""}
        ${d.status === "IN_TRANSIT" ? `<button class="btn small success" onclick="markDelivered(${d.id})">Delivered</button>` : ""}
        ${(d.status === "PENDING" || d.status === "ASSIGNED") ? `<button class="btn small danger" onclick="cancelAdmin(${d.id})">Cancel</button>` : ""}
      </td>
    </tr>`).join("");
}
async function markInTransit(id) { await apiPost("/api/deliveries/in-transit", { deliveryId: id }); loadDeliveries(); }
async function markDelivered(id) { await apiPost("/api/deliveries/delivered", { deliveryId: id }); loadDeliveries(); loadDrones(); }
async function cancelAdmin(id) { await apiPost("/api/cancel", { deliveryId: id, role: "admin" }); loadDeliveries(); loadDrones(); }

async function loadCustomers() {
  const rows = await apiGet("/api/customers");
  document.getElementById("customerBody").innerHTML = rows.map(c => `
    <tr><td>#${c.id}</td><td>${c.name}</td><td>${c.email}</td><td>${c.phone}</td><td>${c.address}</td></tr>
  `).join("") || `<tr><td colspan="5" class="muted">No customers registered yet.</td></tr>`;
}

async function loadReport() {
  const range = document.getElementById("reportRange").value;
  const rows = await apiGet(`/api/reports?range=${range}`);
  document.getElementById("reportBody").innerHTML = rows.map(d => `
    <tr><td>#${d.id}</td><td>#${d.customerId}</td><td>${d.package}</td><td>${d.pickup} → ${d.destination}</td>
      <td><span class="badge ${d.status}">${d.status}</span></td><td>${d.distance.toFixed(1)} km</td><td>${fmtDate(d.createdAt)}</td></tr>
  `).join("") || `<tr><td colspan="7" class="muted">No deliveries in this range.</td></tr>`;

  const box = document.getElementById("aiSmartSummary");
  box.style.display = "block";
  box.textContent = "🤖 Generating AI executive summary…";
  try {
    const ai = await apiGet("/api/ai/smart-report");
    box.textContent = "🤖 " + ai.result;
  } catch { box.textContent = "AI summary unavailable."; }
}
