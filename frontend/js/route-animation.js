/*******************************************************************************
 * route-animation.js
 * Lightweight CSS/DOM route + tracking visualizations. No 3D needed here --
 * these render the exact path array / status string returned by the C++
 * backend (GET /api/route, GET /api/deliveries), just styled as a flight path.
 ******************************************************************************/
window.DDX = window.DDX || {};

/** path: string[] of city names from the real Dijkstra result. distanceKm: number */
DDX.renderRouteViz = function (containerId, path, distanceKm) {
  const el = document.getElementById(containerId);
  if (!el) return;
  if (!path || path.length < 2) { el.innerHTML = ""; return; }
  let html = `<div class="route-viz">`;
  path.forEach((city, i) => {
    const isEnd = i === 0 || i === path.length - 1;
    html += `<span class="route-node${isEnd ? " endpoint" : ""}">${city}</span>`;
    if (i < path.length - 1) {
      html += `<span class="route-arrow"><span class="drone-dot">🛸</span></span>`;
    }
  });
  html += `</div><div class="sub">${path.length - 1} hop${path.length - 1 === 1 ? "" : "s"} across the city graph &middot; ${distanceKm.toFixed(1)} km total (Dijkstra shortest path)</div>`;
  el.innerHTML = html;
};

const DDX_TIMELINE_STAGES = [
  { key: "PENDING", label: "Order Created" },
  { key: "ASSIGNED", label: "Drone Assigned" },
  { key: "IN_TRANSIT", label: "In Transit" },
  { key: "DELIVERED", label: "Delivered" },
];

/** delivery: the real Delivery JSON object from the backend. */
DDX.renderTrackingTimeline = function (containerId, delivery) {
  const el = document.getElementById(containerId);
  if (!el) return;

  if (delivery.status === "CANCELLED") {
    el.innerHTML = `<div class="ai-box" style="border-color:var(--danger);color:#ffd7d7">This delivery was cancelled.</div>`;
    return;
  }

  const currentIndex = DDX_TIMELINE_STAGES.findIndex((s) => s.key === delivery.status);
  let html = `<div class="timeline-track">`;
  DDX_TIMELINE_STAGES.forEach((stage, i) => {
    let cls = "";
    if (i < currentIndex) cls = "done";
    else if (i === currentIndex) cls = "active";
    html += `<div class="timeline-step ${cls}"><div class="line"></div><div class="dot"></div><div class="label">${stage.label}</div></div>`;
  });
  html += `</div>`;
  el.innerHTML = html;
};
