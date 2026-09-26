/*******************************************************************************
 * dsa-viz.js
 * Frontend visualizations of the data structures the C++ backend actually
 * uses. These are illustrative diagrams built from REAL API responses
 * (drone IDs, pending deliveries, route results, report rows) -- the C++
 * server doesn't expose step-by-step algorithm traces, so the diagrams show
 * the real structure/result rather than a live instruction-by-instruction
 * replay of the algorithm.
 ******************************************************************************/
window.DDX = window.DDX || {};

DDX.initDSAPanels = function () {
  const tabs = document.querySelectorAll(".dsa-tabs button");
  if (!tabs.length) return;
  tabs.forEach((btn) => {
    btn.addEventListener("click", () => {
      tabs.forEach((b) => b.classList.remove("active"));
      document.querySelectorAll(".dsa-panel").forEach((p) => p.classList.remove("active"));
      btn.classList.add("active");
      document.getElementById("dsa-" + btn.dataset.dsa).classList.add("active");
      DDX.loadDSAPanel(btn.dataset.dsa);
    });
  });
  DDX.loadDSAPanel(tabs[0].dataset.dsa);
};

DDX.loadDSAPanel = async function (name) {
  try {
    if (name === "dijkstra") await DDX._renderDijkstraPanel();
    if (name === "queue") await DDX._renderQueuePanel();
    if (name === "bst") await DDX._renderBSTPanel();
    if (name === "merge") await DDX._renderMergePanel();
  } catch (e) { /* panel shows its own empty state below */ }
};

// ---------------- Dijkstra: real shortest route between two real cities ----
DDX._renderDijkstraPanel = async function () {
  const root = document.getElementById("dsa-dijkstra");
  if (!root || root.dataset.loaded) { return; }
  const cities = await apiGet("/api/cities");
  const from = cities[0], to = cities[Math.min(4, cities.length - 1)];
  const result = await apiGet(`/api/route?pickup=${encodeURIComponent(from)}&destination=${encodeURIComponent(to)}`);

  const path = result.path;
  const w = Math.max(560, path.length * 130);
  const y = 70;
  let nodes = "", edges = "";
  path.forEach((city, i) => {
    const x = 60 + i * ((w - 120) / Math.max(1, path.length - 1));
    if (i > 0) {
      const px = 60 + (i - 1) * ((w - 120) / Math.max(1, path.length - 1));
      edges += `<line x1="${px}" y1="${y}" x2="${x}" y2="${y}" stroke="#22d3ee" stroke-width="3" />`;
    }
    nodes += `<circle cx="${x}" cy="${y}" r="20" fill="#0d1526" stroke="#22d3ee" stroke-width="2" />` +
      `<text x="${x}" y="${y + 40}" text-anchor="middle" class="node-label">${city}</text>`;
  });
  root.innerHTML = `<svg viewBox="0 0 ${w} 140" style="width:100%;height:auto">${edges}${nodes}</svg>` +
    `<div class="sub" style="margin-top:6px">Shortest path (Dijkstra) from <b>${from}</b> to <b>${to}</b>: ` +
    `<b>${result.distanceKm.toFixed(1)} km</b> via ${path.join(" \u2192 ")}. Pick any pickup/destination in the Customer Portal to compute a different route.</div>`;
  root.dataset.loaded = "1";
};

// ---------------- Priority Queue: real pending deliveries -------------------
DDX._renderQueuePanel = async function () {
  const root = document.getElementById("dsa-queue");
  if (!root) return;
  const deliveries = await apiGet("/api/deliveries");
  const pending = deliveries.filter((d) => d.status === "PENDING")
    .sort((a, b) => (b.priority - a.priority) || (a.createdAt - b.createdAt));

  if (!pending.length) {
    root.innerHTML = `<div class="sub">No deliveries are currently waiting in the queue -- book one from the Customer Portal to see it appear here.</div>`;
    return;
  }
  const tag = (p) => (p >= 3 ? "Critical" : p === 2 ? "Urgent" : "Normal");
  const cls = (p) => (p >= 3 ? "p3" : p === 2 ? "p2" : "");
  root.innerHTML = `<div class="pq-lane">` + pending.map((d) =>
    `<div class="pq-chip ${cls(d.priority)}"><div class="pq-id">#${d.id}</div><div class="pq-tag">${tag(d.priority)}</div><div class="sub" style="margin-top:6px">${d.pickup} \u2192 ${d.destination}</div></div>`
  ).join("") + `</div><div class="sub">Ordered exactly as the backend's priority queue will serve them next (highest priority, then earliest booked).</div>`;
};

// ---------------- Binary Search Tree: real drone IDs ------------------------
DDX._renderBSTPanel = async function () {
  const root = document.getElementById("dsa-bst");
  if (!root) return;
  const drones = await apiGet("/api/drones"); // already returned sorted by ID
  if (!drones.length) { root.innerHTML = `<div class="sub">No drones registered yet.</div>`; return; }
  const ids = drones.map((d) => d.id);

  // Build a balanced BST shape from the sorted ID list (mirrors how a
  // balanced tree over sorted keys would look) purely for layout purposes.
  function build(arr) {
    if (!arr.length) return null;
    const mid = Math.floor(arr.length / 2);
    return { val: arr[mid], left: build(arr.slice(0, mid)), right: build(arr.slice(mid + 1)) };
  }
  const tree = build(ids);

  const w = Math.max(560, ids.length * 70);
  const nodeR = 20;
  let svg = "";
  function layout(node, depth, xMin, xMax) {
    if (!node) return;
    const x = (xMin + xMax) / 2;
    const y = 40 + depth * 60;
    node._x = x; node._y = y;
    layout(node.left, depth + 1, xMin, x);
    layout(node.right, depth + 1, x, xMax);
  }
  layout(tree, 0, 40, w - 40);

  function draw(node) {
    if (!node) return;
    if (node.left) svg += `<line x1="${node._x}" y1="${node._y}" x2="${node.left._x}" y2="${node.left._y}" stroke="#23324d" stroke-width="2" />`;
    if (node.right) svg += `<line x1="${node._x}" y1="${node._y}" x2="${node.right._x}" y2="${node.right._y}" stroke="#23324d" stroke-width="2" />`;
    draw(node.left); draw(node.right);
  }
  draw(tree);
  function drawNodes(node) {
    if (!node) return;
    svg += `<circle cx="${node._x}" cy="${node._y}" r="${nodeR}" fill="#0d1526" stroke="#3b82f6" stroke-width="2" />` +
      `<text x="${node._x}" y="${node._y + 5}" text-anchor="middle" class="node-label">${node.val}</text>`;
    drawNodes(node.left); drawNodes(node.right);
  }
  drawNodes(tree);
  const maxDepth = Math.ceil(Math.log2(ids.length + 1)) * 60 + 60;
  root.innerHTML = `<svg viewBox="0 0 ${w} ${maxDepth}" style="width:100%;height:auto">${svg}</svg>` +
    `<div class="sub">Drone registry (IDs: ${ids.join(", ")}) laid out as a balanced binary search tree for O(log n) average lookup.</div>`;
};

// ---------------- Merge Sort: real report rows ------------------------------
DDX._renderMergePanel = async function () {
  const root = document.getElementById("dsa-merge");
  if (!root) return;
  const rows = await apiGet("/api/reports?range=weekly");
  if (!rows.length) { root.innerHTML = `<div class="sub">No deliveries in range yet -- book a few to populate this week's report.</div>`; return; }

  const shuffled = [...rows].sort(() => Math.random() - 0.5);
  const chip = (d, dim) => `<div class="pq-chip"${dim ? ' style="opacity:.55"' : ""}><div class="pq-id">#${d.id}</div><div class="pq-tag">${new Date(d.createdAt * 1000).toLocaleDateString()}</div></div>`;
  root.innerHTML = `<div class="sub">Unsorted report rows:</div><div class="pq-lane">${shuffled.map((d) => chip(d, true)).join("")}</div>` +
    `<div class="sub" style="margin-top:10px">Merge-sorted by booking time (as the backend's report generator returns them):</div>` +
    `<div class="pq-lane">${rows.map((d) => chip(d, false)).join("")}</div>`;
};
