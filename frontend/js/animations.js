/*******************************************************************************
 * animations.js
 * Small, dependency-light UI animation helpers: scroll reveal (uses GSAP if
 * present, otherwise a plain IntersectionObserver fallback) and animated
 * number counters for stat cards. Never fakes data -- only animates the
 * transition to whatever number the caller passes in.
 ******************************************************************************/
window.DDX = window.DDX || {};

DDX.initScrollReveal = function (selector) {
  const els = document.querySelectorAll(selector || ".reveal");
  if (!els.length) return;
  if (window.gsap && window.ScrollTrigger) {
    gsap.registerPlugin(ScrollTrigger);
    els.forEach((el) => {
      gsap.to(el, {
        opacity: 1, y: 0, duration: 0.7, ease: "power3.out",
        scrollTrigger: { trigger: el, start: "top 88%" },
      });
    });
    return;
  }
  const io = new IntersectionObserver((entries) => {
    entries.forEach((entry) => { if (entry.isIntersecting) { entry.target.classList.add("in"); io.unobserve(entry.target); } });
  }, { threshold: 0.15 });
  els.forEach((el) => io.observe(el));
};

/** Animates the text content of el from 0 (or its current value) to `value`. */
DDX.countTo = function (el, value, opts) {
  opts = opts || {};
  const duration = opts.duration || 900;
  const decimals = opts.decimals || 0;
  const suffix = opts.suffix || "";
  const start = performance.now();
  const from = 0;
  function tick(now) {
    const p = Math.min(1, (now - start) / duration);
    const eased = 1 - Math.pow(1 - p, 3);
    const current = from + (value - from) * eased;
    el.textContent = current.toFixed(decimals) + suffix;
    if (p < 1) requestAnimationFrame(tick);
  }
  requestAnimationFrame(tick);
};

/** Finds every [data-count] element inside root and animates it once. */
DDX.animateCounters = function (root) {
  (root || document).querySelectorAll("[data-count]").forEach((el) => {
    const value = parseFloat(el.dataset.count);
    if (isNaN(value)) return;
    DDX.countTo(el, value, { decimals: el.dataset.decimals ? parseInt(el.dataset.decimals, 10) : 0, suffix: el.dataset.suffix || "" });
  });
};
