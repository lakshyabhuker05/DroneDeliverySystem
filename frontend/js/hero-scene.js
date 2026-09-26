/*******************************************************************************
 * hero-scene.js
 * Cinematic hero: one large procedural drone hovering over a glowing grid,
 * with subtle mouse-parallax. Falls back to a plain gradient (already in
 * CSS) if WebGL isn't available -- never blocks the rest of the page.
 ******************************************************************************/
(function () {
  const container = document.getElementById("heroCanvas3d");
  if (!container) return;

  if (!window.THREE || !DDX.hasWebGL()) {
    container.parentElement.classList.add("no-webgl");
    return;
  }

  const { scene, camera, renderer } = DDX.createScene(container, { camX: 0, camY: 1.4, camZ: 6.2, fov: 42 });
  scene.fog = new THREE.FogExp2(0x0b1220, 0.045);

  const isMobile = window.innerWidth < 720;
  DDX.addGrid(scene, { size: 60, divisions: isMobile ? 26 : 50, y: -1.7, opacity: 0.22 });
  DDX.addParticles(scene, isMobile ? 60 : 160);

  const drone = DDX.buildDrone({ scale: 1.6 });
  drone.position.set(0, 0.4, 0);
  scene.add(drone);

  // Soft glowing point light near the drone body for extra "premium" feel.
  const glow = new THREE.PointLight(0x22d3ee, 1.1, 6);
  glow.position.set(0, 0.6, 0.4);
  scene.add(glow);

  let mouseX = 0, mouseY = 0;
  window.addEventListener("mousemove", (e) => {
    mouseX = (e.clientX / window.innerWidth - 0.5) * 2;
    mouseY = (e.clientY / window.innerHeight - 0.5) * 2;
  });

  let running = true;
  document.addEventListener("visibilitychange", () => { running = !document.hidden; });

  const clock = new THREE.Clock();
  function loop() {
    if (!running) { requestAnimationFrame(loop); return; }
    const t = clock.getElapsedTime();
    DDX.animateDrone(drone, t, { dt: clock.getDelta(), bobAmount: 0.22, yawSpeed: 0.08, bank: 0.06 });

    // Gentle parallax: camera drifts toward the mouse position.
    camera.position.x += (mouseX * 1.1 - camera.position.x) * 0.03;
    camera.position.y += (1.4 - mouseY * 0.5 - camera.position.y) * 0.03;
    camera.lookAt(0, 0.3, 0);

    renderer.render(scene, camera);
    requestAnimationFrame(loop);
  }
  loop();
})();
