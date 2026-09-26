/*******************************************************************************
 * drone.js
 * Procedural low-poly drone builder (no external GLTF model required).
 * Body + 4 arms + 4 spinning propellers + landing legs + LED nav lights.
 * If a real model is later dropped into frontend/assets/models/, a loader
 * can be swapped in here without touching any other file.
 ******************************************************************************/
window.DDX = window.DDX || {};

DDX.buildDrone = function (opts) {
  opts = opts || {};
  const accent = opts.color !== undefined ? opts.color : 0x22d3ee;
  const scale = opts.scale || 1;

  const group = new THREE.Group();

  // ---- Body ----
  const bodyMat = new THREE.MeshStandardMaterial({ color: 0x111a2c, metalness: 0.6, roughness: 0.35 });
  const body = new THREE.Mesh(new THREE.BoxGeometry(0.9, 0.22, 0.9), bodyMat);
  body.castShadow = true;
  group.add(body);

  const domeMat = new THREE.MeshStandardMaterial({ color: accent, metalness: 0.2, roughness: 0.15, emissive: accent, emissiveIntensity: 0.25 });
  const dome = new THREE.Mesh(new THREE.SphereGeometry(0.22, 16, 12, 0, Math.PI * 2, 0, Math.PI / 2), domeMat);
  dome.position.set(0, 0.11, 0);
  group.add(dome);

  // Camera/sensor underneath
  const sensor = new THREE.Mesh(new THREE.SphereGeometry(0.09, 10, 8), new THREE.MeshStandardMaterial({ color: 0x030712, metalness: 0.8, roughness: 0.2 }));
  sensor.position.set(0, -0.14, 0.35);
  group.add(sensor);

  // ---- Arms + rotors ----
  const armMat = new THREE.MeshStandardMaterial({ color: 0x1c2a44, metalness: 0.5, roughness: 0.4 });
  const rotorGroups = [];
  const armPositions = [
    [0.62, 0, 0.62], [-0.62, 0, 0.62], [0.62, 0, -0.62], [-0.62, 0, -0.62]
  ];

  armPositions.forEach(([x, y, z]) => {
    const arm = new THREE.Mesh(new THREE.BoxGeometry(Math.abs(x) * 1.15, 0.06, 0.06), armMat);
    arm.position.set(x / 2, 0, z / 2);
    arm.rotation.y = Math.atan2(z, x);
    group.add(arm);

    const motor = new THREE.Mesh(new THREE.CylinderGeometry(0.09, 0.1, 0.14, 10), armMat);
    motor.position.set(x, 0.05, z);
    group.add(motor);

    const rotor = new THREE.Group();
    rotor.position.set(x, 0.13, z);
    const bladeMat = new THREE.MeshStandardMaterial({ color: 0x93a2c2, metalness: 0.3, roughness: 0.5, transparent: true, opacity: 0.85 });
    for (let i = 0; i < 2; i++) {
      const blade = new THREE.Mesh(new THREE.BoxGeometry(0.62, 0.01, 0.07), bladeMat);
      blade.rotation.y = (Math.PI / 2) * i;
      rotor.add(blade);
    }
    group.add(rotor);
    rotorGroups.push(rotor);

    // Nav light (green/red alternating like real aircraft, plus soft glow)
    const lightColor = x > 0 ? 0x22c55e : 0xef4444;
    const led = new THREE.Mesh(new THREE.SphereGeometry(0.035, 8, 8), new THREE.MeshStandardMaterial({ color: lightColor, emissive: lightColor, emissiveIntensity: 1.4 }));
    led.position.set(x, -0.02, z);
    group.add(led);
  });

  // ---- Landing legs ----
  const legMat = new THREE.MeshStandardMaterial({ color: 0x0d1526, metalness: 0.4, roughness: 0.6 });
  [[0.32, 0.32], [-0.32, 0.32], [0.32, -0.32], [-0.32, -0.32]].forEach(([x, z]) => {
    const leg = new THREE.Mesh(new THREE.CylinderGeometry(0.02, 0.02, 0.32, 6), legMat);
    leg.position.set(x, -0.28, z);
    group.add(leg);
  });
  const skidMat = legMat;
  [0.32, -0.32].forEach((z) => {
    const skid = new THREE.Mesh(new THREE.BoxGeometry(0.7, 0.03, 0.03), skidMat);
    skid.position.set(0, -0.43, z);
    group.add(skid);
  });

  group.scale.setScalar(scale);
  group.userData.rotors = rotorGroups;
  group.userData.baseY = group.position.y;
  return group;
};

/** Call every frame: bobs, subtly banks, and spins propellers. */
DDX.animateDrone = function (group, t, opts) {
  opts = opts || {};
  const bob = opts.bobAmount ?? 0.18;
  const bobSpeed = opts.bobSpeed ?? 1.1;
  group.position.y = (group.userData.baseY || 0) + Math.sin(t * bobSpeed) * bob;
  group.rotation.y += (opts.yawSpeed ?? 0.15) * (opts.dt || 0.016);
  group.rotation.z = Math.sin(t * 0.6) * (opts.bank ?? 0.05);
  group.rotation.x = Math.cos(t * 0.5) * (opts.bank ?? 0.05) * 0.6;
  (group.userData.rotors || []).forEach((r, i) => {
    r.rotation.y += (i % 2 === 0 ? 1 : -1) * (opts.propSpeed ?? 1.6);
  });
};
