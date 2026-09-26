/*******************************************************************************
 * fleet.js
 * 3D fleet visualization driven ENTIRELY by real data from GET /api/drones.
 * No drone, status, battery, speed or location shown here is invented --
 * every value comes straight from the C++ backend response.
 *
 * Usage: DDX.initFleetScene("fleetScene") where #fleetScene is an empty
 * container with a fixed height (see .scene-wrap in style.css).
 ******************************************************************************/
window.DDX = window.DDX || {};

DDX.initFleetScene = function (containerId, opts) {
  opts = opts || {};
  const container = document.getElementById(containerId);
  if (!container) return null;

  if (!window.THREE || !DDX.hasWebGL()) {
    container.innerHTML = '<div class="scene-empty">3D view unavailable in this browser -- see the table below for live fleet data.</div>';
    return null;
  }

  const tooltip = document.createElement("div");
  tooltip.className = "drone-tooltip";
  container.appendChild(tooltip);

  const { scene, camera, renderer } = DDX.createScene(container, { camX: 0, camY: 4.2, camZ: 8.5, fov: 48 });
  camera.lookAt(0, 0, 0);
  DDX.addGrid(scene, { size: 30, divisions: 24, y: -1.2, opacity: 0.18 });

  const raycaster = new THREE.Raycaster();
  const mouse = new THREE.Vector2();
  let hovered = null;
  let droneMeshes = []; // { group, halo, data }

  function layout(n) {
    // Simple circular layout so drones read as a "fleet" rather than a grid.
    const positions = [];
    const radius = Math.max(2.4, n * 0.55);
    for (let i = 0; i < n; i++) {
      const angle = (i / n) * Math.PI * 2;
      positions.push([Math.cos(angle) * radius, 0, Math.sin(angle) * radius]);
    }
    return positions;
  }

  function rebuild(drones) {
    droneMeshes.forEach((d) => scene.remove(d.group));
    droneMeshes = [];
    if (!drones.length) return;
    const positions = layout(drones.length);
    drones.forEach((d, i) => {
      const color = DDX.statusColor(d.status);
      const group = DDX.buildDrone({ color, scale: 0.85 });
      const [x, y, z] = positions[i];
      group.position.set(x, 0.3, z);
      group.userData.baseY = 0.3;
      scene.add(group);

      const halo = new THREE.Mesh(
        new THREE.RingGeometry(0.55, 0.68, 24),
        new THREE.MeshBasicMaterial({ color, transparent: true, opacity: 0.55, side: THREE.DoubleSide })
      );
      halo.rotation.x = -Math.PI / 2;
      halo.position.set(x, -1.18, z);
      scene.add(halo);

      droneMeshes.push({ group, halo, data: d });
    });
  }

  function onMove(evt) {
    const rect = renderer.domElement.getBoundingClientRect();
    mouse.x = ((evt.clientX - rect.left) / rect.width) * 2 - 1;
    mouse.y = -((evt.clientY - rect.top) / rect.height) * 2 + 1;
    raycaster.setFromCamera(mouse, camera);
    const targets = droneMeshes.map((d) => d.group);
    const hits = raycaster.intersectObjects(targets, true);
    if (hits.length) {
      let obj = hits[0].object;
      while (obj.parent && !droneMeshes.find((d) => d.group === obj)) obj = obj.parent;
      const match = droneMeshes.find((d) => d.group === obj);
      if (match) {
        hovered = match;
        const d = match.data;
        tooltip.innerHTML = `<b>DRONE #${d.id}</b><div class="row-line"><span>Model</span><span>${d.model}</span></div>` +
          `<div class="row-line"><span>Status</span><span>${d.status}</span></div>` +
          `<div class="row-line"><span>Battery</span><span>${d.battery}%</span></div>` +
          `<div class="row-line"><span>Speed</span><span>${d.speed} km/h</span></div>` +
          `<div class="row-line"><span>Location</span><span>${d.location}</span></div>`;
        tooltip.style.left = Math.min(evt.clientX - rect.left + 14, rect.width - 190) + "px";
        tooltip.style.top = Math.max(evt.clientY - rect.top - 10, 4) + "px";
        tooltip.classList.add("show");
        return;
      }
    }
    hovered = null;
    tooltip.classList.remove("show");
  }
  renderer.domElement.addEventListener("mousemove", onMove);
  renderer.domElement.addEventListener("mouseleave", () => tooltip.classList.remove("show"));

  let running = true;
  document.addEventListener("visibilitychange", () => { running = !document.hidden; });

  const clock = new THREE.Clock();
  (function loop() {
    if (running) {
      const t = clock.getElapsedTime();
      const dt = clock.getDelta();
      droneMeshes.forEach((d, i) => {
        DDX.animateDrone(d.group, t + i * 0.7, { dt, bobAmount: 0.12, yawSpeed: hovered === d ? 0.6 : 0.05, bank: 0.03, propSpeed: d.data.status === "OFFLINE" ? 0 : 1.4 });
        d.halo.rotation.z += 0.003;
      });
      camera.position.x = Math.sin(t * 0.05) * 1.2;
      camera.lookAt(0, 0, 0);
      renderer.render(scene, camera);
    }
    requestAnimationFrame(loop);
  })();

  async function refresh() {
    try {
      const drones = await apiGet("/api/drones");
      rebuild(drones);
      if (opts.onData) opts.onData(drones);
    } catch (e) { /* dashboard/admin already surfaces API errors elsewhere */ }
  }
  refresh();
  const interval = setInterval(refresh, opts.pollMs || 8000);

  return { refresh, stop: () => clearInterval(interval) };
};
