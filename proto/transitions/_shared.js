/* Espelha 1:1 as curvas de source/anim.c -- qualquer curva usada aqui tem
 * que existir lá com o mesmo nome/formato, senão o protótipo mente sobre o
 * que vai rodar no 3DS (spec v6 secao 0). */
const Ease = {
  linear: t => t,
  outCubic: t => 1 - Math.pow(1 - t, 3),
  inOutCubic: t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2,
  outBack: (t, amp = 1.70158) => { const c3 = amp + 1; return 1 + c3 * Math.pow(t - 1, 3) + amp * Math.pow(t - 1, 2); },
  inCubic: t => t * t * t,
  spring: (t, dampingRatio = 0.75) => {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    const w = 2 * Math.PI * 1.1;
    return 1 - Math.exp(-dampingRatio * w * t) * Math.cos(w * t * Math.sqrt(1 - dampingRatio * dampingRatio));
  },
};
function clamp(v, a, b) { return Math.max(a, Math.min(b, v)); }
function lerp(a, b, t) { return a + (b - a) * t; }

/* Mini "tela" generica (nao a UI real) so para demonstrar a mecanica de
 * movimento da transicao -- cards/icones aqui sao placeholders. */
function buildMockScreen(label, accent) {
  const el = document.createElement('div');
  el.className = 'mock-screen';
  el.innerHTML = `
    <div class="mock-topbar"><span class="dot r"></span><span class="dot y"></span><span class="dot g"></span><b>${label}</b></div>
    <div class="mock-card" style="border-color:${accent}33;">
      <div class="mock-icon" style="background:${accent}22;color:${accent};">★</div>
      <div class="mock-text">
        <div class="mock-title">Item de exemplo</div>
        <div class="mock-value">valor atual</div>
      </div>
    </div>
    <div class="mock-row">
      <div class="mock-chip" style="border-color:${accent}55"></div>
      <div class="mock-chip" style="border-color:${accent}55"></div>
      <div class="mock-chip" style="border-color:${accent}55"></div>
    </div>`;
  return el;
}
