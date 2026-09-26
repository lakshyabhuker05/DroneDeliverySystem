/*******************************************************************************
 * three-scene.js
 * Small shared Three.js helpers used by the hero drone and the 3D fleet
 * views. Pure presentation layer -- never touches API data itself.
 ******************************************************************************/
window.DDX = window.DDX || {};

/** True once we've confirmed the browser can actually do WebGL. */
DDX.hasWebGL = function () {
  try {
    const canvas = document.createElement("canvas");
    return !!(window.WebGLRenderingContext &&
      (canvas.getContext("webgl") || canvas.getContext("experimental-webgl")));
  } catch (e) {
    return false;
  }
};

/**
 * Creates a renderer + scene + camera bound to a container element.
 * Handles resize + capped devicePixelRatio for performance.
 */
DDX.createScene = function (container, opts) {
  opts = opts || {};
  const width = container.clientWidth || 300;
  const height = container.clientHeight || 300;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(opts.fov || 45, width / height, 0.1, 1000);
  camera.position.set(opts.camX ?? 0, opts.camY ?? 2.2, opts.camZ ?? 7);

  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.setSize(width, height);
  container.appendChild(renderer.domElement);

  const resize = () => {
    const w = container.clientWidth || width;
    const h = container.clientHeight || height;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
  };
  window.addEventListener("resize", resize);

  const ambient = new THREE.AmbientLight(0x8fa8ff, 0.55);
  const key = new THREE.DirectionalLight(0xbfe0ff, 1.1);
  key.position.set(4, 6, 4);
  const rim = new THREE.DirectionalLight(0x22d3ee, 0.5);
  rim.position.set(-5, 2, -4);
  scene.add(ambient, key, rim);

  return { scene, camera, renderer, resize, dispose: () => window.removeEventListener("resize", resize) };
};

/** Simple ground grid used as a futuristic city-network backdrop. */
DDX.addGrid = function (scene, opts) {
  opts = opts || {};
  const grid = new THREE.GridHelper(opts.size || 40, opts.divisions || 40, 0x22d3ee, 0x14233d);
  grid.material.transparent = true;
  grid.material.opacity = opts.opacity ?? 0.25;
  grid.position.y = opts.y ?? -1.6;
  scene.add(grid);
  return grid;
};

/** Lightweight ambient particle field (kept small for performance). */
DDX.addParticles = function (scene, count) {
  count = count || 140;
  const geo = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  for (let i = 0; i < count; i++) {
    positions[i * 3] = (Math.random() - 0.5) * 30;
    positions[i * 3 + 1] = Math.random() * 10 - 2;
    positions[i * 3 + 2] = (Math.random() - 0.5) * 30;
  }
  geo.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  const mat = new THREE.PointsMaterial({ color: 0x6fd9ff, size: 0.045, transparent: true, opacity: 0.55 });
  const points = new THREE.Points(geo, mat);
  scene.add(points);
  return points;
};

DDX.statusColor = function (status) {
  switch (status) {
    case "AVAILABLE": return 0x22c55e;
    case "ASSIGNED": return 0x3b82f6;
    case "IN_TRANSIT": return 0x22d3ee;
    case "CHARGING": return 0xf59e0b;
    case "MAINTENANCE": return 0xf59e0b;
    case "OFFLINE": return 0xef4444;
    default: return 0x93a2c2;
  }
};
