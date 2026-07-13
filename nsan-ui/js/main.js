// ============ backend detection ============
let backendHealth = null;

async function checkHealth() {
  try {
    const res = await fetch('/api/health', { cache: 'no-store' });
    if (!res.ok) throw new Error('bad response');
    backendHealth = await res.json();
  } catch (e) {
    backendHealth = null;
  }
  updateStatusStrip();
  return backendHealth;
}

function updateStatusStrip() {
  const dot = document.getElementById('status-dot');
  const text = document.getElementById('status-text');
  const footerMode = document.getElementById('footer-mode');
  if (!backendHealth) {
    dot.className = 'status-dot offline';
    text.textContent = 'Verification server offline — running in simulated modeling mode. Start "python3 server.py" for live compiler execution.';
    footerMode.textContent = 'simulated modeling mode';
    return;
  }
  const missing = [];
  if (!backendHealth.nsanClangFound) missing.push('nsan-clang++');
  if (!backendHealth.passPluginFound) missing.push('libnsan_pass.so');
  if (missing.length) {
    dot.className = 'status-dot offline';
    text.textContent = `Server active, but ${missing.join(' and ')} not located under ${backendHealth.repoRoot}. Execute build script (./build.sh) or configure environment variables.`;
    footerMode.textContent = 'server active (toolchain binaries required)';
  } else {
    dot.className = 'status-dot live';
    text.textContent = `Live Toolchain Connected — executing ${backendHealth.nsanClangPath} with pass plugin ${backendHealth.passPluginPath}`;
    footerMode.textContent = 'live toolchain mode (active compiler & runtime)';
  }
}


function backendReady() {
  return backendHealth && backendHealth.nsanClangFound && backendHealth.passPluginFound;
}

// ============ shared math (used by simulated mode + as a display fallback) ============
const state = { a: 1000000.12544, b: 1000000.0, op: '-', eps: 1e-5 };

const opSymbols = { '+': '+', '-': '−', '*': '×', '/': '÷' };
const irOpName  = { '+': 'fadd', '-': 'fsub', '*': 'fmul', '/': 'fdiv' };

function computeLanes(a, b, op) {
  const fa = Math.fround(a), fb = Math.fround(b);
  let floatResult, shadowResult;
  switch (op) {
    case '+': floatResult = Math.fround(fa + fb); shadowResult = a + b; break;
    case '-': floatResult = Math.fround(fa - fb); shadowResult = a - b; break;
    case '*': floatResult = Math.fround(fa * fb); shadowResult = a * b; break;
    case '/': floatResult = Math.fround(fa / fb); shadowResult = a / b; break;
  }
  let relErr;
  if (shadowResult === 0) relErr = floatResult === 0 ? 0 : Infinity;
  else relErr = Math.abs(floatResult - shadowResult) / Math.abs(shadowResult);
  return { floatResult, shadowResult, relErr };
}

function fmtNum(n) {
  if (!isFinite(n)) return n > 0 ? '+inf' : (n < 0 ? '-inf' : 'nan');
  if (n === 0) return '0.000000';
  const abs = Math.abs(n);
  if (abs >= 1e6 || abs < 1e-4) return n.toExponential(4);
  return n.toFixed(6);
}
function fmtErr(e) { return isFinite(e) ? e.toExponential(2) : '>> threshold'; }
function widthFor(n) {
  const abs = Math.abs(n);
  if (abs === 0) return 4;
  return Math.max(4, Math.min(100, 8 + Math.log10(abs + 1) * 22));
}
function meterWidth(relErr) {
  if (relErr === 0) return 3;
  const ratio = Math.max(0, Math.log10(relErr) + 8);
  return Math.min(100, Math.max(4, ratio * 9));
}

// ============ Shadow Lab ============
function renderLabSimulated() {
  const { a, b, op, eps } = state;
  const { floatResult, shadowResult, relErr } = computeLanes(a, b, op);
  document.getElementById('lab-mode-badge').textContent = 'simulated';
  document.getElementById('lab-mode-badge').className = 'mode-badge';
  document.getElementById('lab-terminal').style.display = 'none';
  paintLab(floatResult, shadowResult, relErr, eps);
  renderIRSim();
}


function el(id) {
  const e = document.getElementById(id);
  if (!e) console.error('NSan UI: missing element #' + id);
  return e;
}

function paintLab(floatResult, shadowResult, relErr, eps) {
  const { a, b, op } = state;
  const exprEl = el('lab-expr');
  if (exprEl) exprEl.textContent = `${a} ${opSymbols[op]} ${b}`;
  const fvEl = el('lab-float-val');
  if (fvEl) fvEl.textContent = fmtNum(floatResult);
  const svEl = el('lab-shadow-val');
  if (svEl) svEl.textContent = fmtNum(shadowResult);
  const ffEl = el('lab-float-fill');
  if (ffEl) ffEl.style.width = widthFor(floatResult) + '%';
  const sfEl = el('lab-shadow-fill');
  if (sfEl) sfEl.style.width = widthFor(shadowResult) + '%';
  const rlEl = el('lab-relerr-lbl');
  if (rlEl) rlEl.textContent = fmtErr(relErr);

  const meter = el('lab-meter');
  const warn = relErr > eps;
  if (meter) {
    meter.style.width = meterWidth(relErr) + '%';
    meter.style.background = warn ? 'var(--warn)' : 'var(--pass)';
  }

  const badge = el('lab-badge');
  if (badge) {
    badge.className = 'badge ' + (warn ? 'warn' : 'pass');
    badge.textContent = warn
      ? `⚠ WARN — rel_err ${fmtErr(relErr)} > ε`
      : `✓ SILENT — rel_err ${fmtErr(relErr)} ≤ ε`;
  }
}


async function runLabLive() {
  const btn = document.getElementById('lab-run-live');
  const terminal = document.getElementById('lab-terminal');
  if (!backendReady()) {
    terminal.style.display = 'block';
    terminal.textContent = 'server.py is not connected (or the plugin isn\'t built yet) — see the status bar at the top of the page.';
    return;
  }
  btn.disabled = true;
  btn.textContent = 'Compiling with nsan-clang++ …';
  terminal.style.display = 'block';
  terminal.textContent = '$ compiling and running through the real toolchain…';

  try {
    const res = await fetch('/api/lab', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ a: state.a, b: state.b, op: state.op, eps: state.eps }),
    });
    const data = await res.json();

    if (!data.ok) {
      terminal.textContent = `$ nsan-clang++ -fsanitize=numerical lab.cpp\n\n[compile failed]\n${data.error || (data.compile && data.compile.stderr) || 'unknown error'}`;
      document.getElementById('lab-mode-badge').textContent = 'live — error';
      document.getElementById('lab-mode-badge').className = 'mode-badge';
      return;
    }

    const runOut = (data.run?.stdout || '') + (data.run?.stderr || '');
    terminal.textContent = `$ nsan-clang++ -fsanitize=numerical lab.cpp -o lab && ./lab\n\n${runOut.trim() || '(no output — no WARN means SILENT, i.e. rel_err stayed under the threshold)'}`;
    document.getElementById('lab-mode-badge').textContent = 'live — real plugin';
    document.getElementById('lab-mode-badge').className = 'mode-badge live';

    // Parse a NSan-style warning block out of the real stdout if present:
    //   float32 = ...
    //   shadow  = ...
    //   rel_err = ...
    const floatMatch = runOut.match(/float32\s*=\s*([\-+0-9.eE]+)/);
    const shadowMatch = runOut.match(/shadow\s*=\s*([\-+0-9.eE]+)/);
    const errMatch = runOut.match(/rel_err\s*=\s*([\-+0-9.eE]+)/);
    if (floatMatch && shadowMatch) {
      const fr = parseFloat(floatMatch[1]);
      const sr = parseFloat(shadowMatch[1]);
      const re = errMatch ? parseFloat(errMatch[1]) : Math.abs(fr - sr) / Math.abs(sr || 1);
      paintLab(fr, sr, re, state.eps);
    } else {
      // runtime stayed silent (rel_err <= eps) — compute locally just to paint the bars
      const { floatResult, shadowResult, relErr } = computeLanes(state.a, state.b, state.op);
      paintLab(floatResult, shadowResult, relErr, state.eps);

    }
  } catch (e) {
    terminal.textContent = `Request failed: ${e}`;
  } finally {
    btn.disabled = false;
    btn.textContent = '▶ Run on real compiler';
  }
}

function wireLab() {
  const opA = document.getElementById('opA');
  const opB = document.getElementById('opB');
  const epsSlider = document.getElementById('epsSlider');
  const epsLbl = document.getElementById('epsLbl');
  const opBtns = document.querySelectorAll('.opbtns button');

  function syncFromInputs() {
    state.a = parseFloat(opA.value);
    state.b = parseFloat(opB.value);
    state.eps = Math.pow(10, -parseInt(epsSlider.value));
    epsLbl.textContent = '1e-' + epsSlider.value;
    if (!isNaN(state.a) && !isNaN(state.b)) renderLabSimulated();
  }

  opBtns.forEach(btn => btn.addEventListener('click', () => {
    opBtns.forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    state.op = btn.dataset.op;
    renderLabSimulated();
  }));

  [opA, opB, epsSlider].forEach(el => el.addEventListener('input', syncFromInputs));

  document.querySelectorAll('.presets button').forEach(btn => {
    btn.addEventListener('click', () => {
      opA.value = btn.dataset.a;
      opB.value = btn.dataset.b;
      state.op = btn.dataset.op;
      opBtns.forEach(b => b.classList.toggle('active', b.dataset.op === state.op));
      syncFromInputs();
    });
  });

  document.getElementById('lab-run-live').addEventListener('click', runLabLive);

  syncFromInputs();
}

// ============ IR instrumentation viewer ============
let irStage = 'original';   // 'original' | 'instrumented'
let irUiMode = 'sim';       // 'sim' | 'live'
let irLiveCache = {};       // keyed by testfile

function irLinesSimulated(stage) {
  const { floatResult, shadowResult } = computeLanes(state.a, state.b, state.op);
  const opName = irOpName[state.op];
  const original = [
    '%x = load float, float* %x_ptr',
    '%y = load float, float* %y_ptr',
    `%z = ${opName} float %x, %y`,
    'store float %z, float* %z_ptr',
  ];
  if (stage === 'original') return original.map(l => ({ text: l, added: false, type: 'normal' }));
  return [
    { text: original[0], added: false, type: 'normal' },
    { text: original[1], added: false, type: 'normal' },
    { text: original[2], added: false, type: 'normal' },
    { text: '%x_shadow = fpext float %x to double', added: true, type: 'math', badge: 'SHADOW MATH' },
    { text: '%y_shadow = fpext float %y to double', added: true, type: 'math', badge: 'SHADOW MATH' },
    { text: `%z_shadow = ${opName} double %x_shadow, %y_shadow`, added: true, type: 'math', badge: 'SHADOW MATH' },
    { text: original[3], added: false, type: 'normal' },
    { text: 'call void @__nsan_check_consistency(', added: true, type: 'check', badge: 'CHECK' },
    { text: `    float %z    /* = ${fmtNum(floatResult)} */,`, added: true, type: 'check' },
    { text: `    double %z_shadow /* = ${fmtNum(shadowResult)} */,`, added: true, type: 'check' },
    { text: `    i8* @filename, i32 @line)`, added: true, type: 'check' },
  ];
}

function escapeHTML(s) { return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;'); }

function paintIRLines(lines, containerId) {
  const container = document.getElementById(containerId);
  if (!container) return;
  container.innerHTML = lines.map((l, i) => {
    const typeCls = l.type && l.type !== 'normal' ? ` type-${l.type}` : (l.added ? ' added' : '');
    const gutter = l.added || (l.type && l.type !== 'normal') ? '+' : '&nbsp;';
    const badgeHTML = l.badge ? `<span class="ir-badge badge-${l.type || 'added'}">${l.badge}</span>` : '';
    return `<div class="ir-line${typeCls}" style="animation-delay:${Math.min(i * 15, 300)}ms"><span class="ir-line-code"><span class="gutter">${gutter}</span>${escapeHTML(l.text)}</span>${badgeHTML}</div>`;
  }).join('');
}

function getSimulatedSource() {
  const opSymbol = state.op;
  return [
    '// Shadow Computation Lab (Simulated C++ snippet)',
    '#include <cmath>',
    '#include <cstdio>',
    '',
    'extern "C" void __nsan_check_consistency(float orig, double shadow);',
    '',
    'void simulated_lane() {',
    `    float x = ${state.a}f;`,
    `    float y = ${state.b}f;`,
    `    float z = x ${opSymbol} y; // target operation`,
    `    __nsan_check_consistency(z, (double)x ${opSymbol} (double)y);`,
    '}'
  ];
}

function renderIRSim() {
  if (irUiMode !== 'sim') return;
  document.getElementById('ir-status').textContent = '';
  document.getElementById('ir-status').className = 'ir-status';
  paintIRLines(getSimulatedSource().map(l => ({ text: l, added: false, type: 'normal' })), 'ir-code-src');
  paintIRLines(irLinesSimulated('original'), 'ir-code-orig');
  paintIRLines(irLinesSimulated('instrumented'), 'ir-code-instr');
}


function cleanLLVMIR(irText) {
  if (!irText) return '';
  return irText.split('\n')
    .map(line => line
      .replace(/\s*,?\s*!tbaa\s+![0-9]+/g, '')
      .replace(/\s*,?\s*!dbg\s+![0-9]+/g, '')
      .replace(/\s+#[0-9]+/g, '')
      .replace(/\s+local_unnamed_addr/g, '')
      .replace(/\s+noundef/g, '')
      .replace(/\s+nonnull/g, '')
    )
    .filter(line => {
      const trimmed = line.trim();
      if (trimmed.startsWith('call void @llvm.lifetime.')) return false;
      if (trimmed.startsWith('declare ')) return false;
      if (trimmed.startsWith('attributes #')) return false;
      if (trimmed.startsWith('!')) return false;
      if (trimmed === '') return false;
      return true;
    })
    .join('\n');
}

function categorizeDiffLines(origText, instrText) {
  const cleanOrig = cleanLLVMIR(origText);
  const cleanInstr = cleanLLVMIR(instrText);
  const origLinesSet = new Set(cleanOrig.split('\n').map(l => l.trim()).filter(Boolean));

  return cleanInstr.split('\n').map(l => {
    const trimmed = l.trim();
    if (!trimmed) return null;

    if (trimmed.includes('@__nsan_check_consistency')) {
      return { text: l, added: true, type: 'check', badge: 'CHECK' };
    }
    if (trimmed.includes('@__nsan_shadow_ptr_') || trimmed.includes('@__nsan_get_shadow_ptr_')) {
      return { text: l, added: true, type: 'mem', badge: 'SHADOW MEM' };
    }
    if (trimmed.includes('to double') || /\b(fadd|fsub|fmul|fdiv)\s+double\b/.test(trimmed) || trimmed.includes('%shadow.')) {
      return { text: l, added: true, type: 'math', badge: 'SHADOW MATH' };
    }
    if (!origLinesSet.has(trimmed) && !trimmed.startsWith('define ') && !trimmed.startsWith('entry:') && !trimmed.startsWith('ret ') && !trimmed.startsWith('}') && !trimmed.startsWith('if.end') && !trimmed.startsWith('br ')) {
      return { text: l, added: true, type: 'added', badge: '+ PASS OP' };
    }
    return { text: l, added: false, type: 'normal' };
  }).filter(Boolean);
}

async function renderIRLive() {
  const select = document.getElementById('ir-tc-select');
  const tc = select.value;
  document.getElementById('ir-filename').textContent = `${tc} → (compiled live side-by-side)`;
  const statusEl = document.getElementById('ir-status');

  if (!backendReady()) {
    statusEl.className = 'ir-status error';
    statusEl.textContent = 'Backend not ready — check the status bar above (needs server.py running + project built).';
    document.getElementById('ir-code-src').textContent = '';
    document.getElementById('ir-code-orig').textContent = '';
    document.getElementById('ir-code-instr').textContent = '';
    return;
  }

  if (!irLiveCache[tc]) {
    statusEl.className = 'ir-status';
    statusEl.textContent = 'Compiling original and instrumented IR from the real test file…';
    document.getElementById('ir-code-src').textContent = 'Loading...';
    document.getElementById('ir-code-orig').textContent = 'Loading...';
    document.getElementById('ir-code-instr').textContent = 'Loading...';
    try {
      const res = await fetch(`/api/ir?tc=${encodeURIComponent(tc)}&fn=main`);
      irLiveCache[tc] = await res.json();
    } catch (e) {
      statusEl.className = 'ir-status error';
      statusEl.textContent = 'Request failed: ' + e;
      return;
    }
  }

  const data = irLiveCache[tc];
  const orig = data.original;
  const instr = data.instrumented;

  if (!orig || !orig.ok || orig.returncode !== 0) {
    statusEl.className = 'ir-status error';
    statusEl.textContent = 'clang++ failed to produce original IR: ' + (orig && (orig.stderr || orig.error) || 'unknown error');
    document.getElementById('ir-code-src').textContent = '';
    document.getElementById('ir-code-orig').textContent = '';
    document.getElementById('ir-code-instr').textContent = '';
    return;
  }
  if (!instr || !instr.ok || (instr.returncode !== undefined && instr.returncode !== 0)) {
    statusEl.className = 'ir-status error';
    statusEl.textContent = 'Pass plugin compile failed: ' + (instr && (instr.stderr || instr.error) || 'libnsan_pass.so not found');
    const srcText = data.source || '// Source not available';
    paintIRLines(srcText.split('\n').map(l => ({ text: l, added: false, type: 'normal' })), 'ir-code-src');
    paintIRLines(cleanLLVMIR(orig.ir).split('\n').map(l => ({ text: l, added: false, type: 'normal' })), 'ir-code-orig');
    document.getElementById('ir-code-instr').textContent = 'Pass plugin compile failed.';
    return;
  }

  statusEl.className = 'ir-status';
  statusEl.textContent = `Live from ${backendHealth.passPluginPath} (3-column transformation view)`;
  const srcText = data.source || '// Source code not available';
  paintIRLines(srcText.split('\n').map(l => ({ text: l, added: false, type: 'normal' })), 'ir-code-src');
  const cleanOrigLines = cleanLLVMIR(orig.ir).split('\n').map(l => ({ text: l, added: false, type: 'normal' }));
  paintIRLines(cleanOrigLines, 'ir-code-orig');
  paintIRLines(categorizeDiffLines(orig.ir, instr.ir), 'ir-code-instr');
}


function renderIR() {
  if (irUiMode === 'sim') renderIRSim();
  else renderIRLive();
}

async function populateTcSelect() {
  const select = document.getElementById('ir-tc-select');
  let files = [];
  try {
    const res = await fetch('/api/testcases');
    const data = await res.json();
    files = data.files || [];
  } catch (e) { /* backend offline, fall back below */ }
  if (!files.length) files = testCases.map(tc => tc.file);
  select.innerHTML = files.map(f => `<option value="${f}">${f}</option>`).join('');
}

function wireIR() {
  document.getElementById('ir-mode-tabs').querySelectorAll('.ir-tab').forEach(tab => {
    tab.addEventListener('click', () => {
      document.getElementById('ir-mode-tabs').querySelectorAll('.ir-tab').forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      irUiMode = tab.dataset.mode;
      renderIR();
    });
  });
  document.getElementById('ir-tc-select').addEventListener('change', () => {
    if (irUiMode === 'live') renderIRLive();
  });
}


// ============ Test suite ============
const testCases = [
  { id: 'TC1', file: 'tc1_cancellation.cpp', name: 'Catastrophic cancellation', status: 'warn', err: '1.90e-01',
    desc: '(1e6 + 1.234) − 1e6 in float32. The fractional part exceeds available mantissa bits and vanishes.',
    out: 'float32 = 1.000000\nshadow  = 1.234000\nrel_err = 1.900e-01 (threshold 1.0e-05)' },
  { id: 'TC2', file: 'tc2_naive_sum.cpp', name: 'Naive summation drift', status: 'warn', err: '9.58e-03',
    desc: '0.1f added 1,000,000 times. Each addition rounds; as the total grows, new additions round away entirely.',
    out: 'float32 = +100958.3438\nshadow  = +100000\nrel_err = 9.583e-03 (threshold 1.0e-05)' },
  { id: 'TC3', file: 'tc3_kahan.cpp', name: 'Kahan compensated sum', status: 'silent', err: '0.00e+00',
    desc: 'Same sum as TC2, but a compensation variable tracks and re-adds lost rounding error every iteration.',
    out: 'TC3 [Kahan Sum - SILENT] float=100000.000000 ref=100000.000000 rel_err=0.00e+00' },
  { id: 'TC4', file: 'tc4_alternating.cpp', name: 'Alternating harmonic series', status: 'warn', err: '1.06e-05',
    desc: '1 − 1/2 + 1/3 − 1/4 … converges near ln(2), but accumulates rounding along the way — just barely past ε.',
    out: 'float32 = 0.693097\nshadow  = 0.693147\nrel_err = 1.06e-05 (threshold 1.0e-05)' },
  { id: 'TC5', file: 'tc5_poly.cpp', name: 'Polynomial near root', status: 'warn', err: '>1e-05',
    desc: 'p(x) = x²−x−1 evaluated near the golden ratio root. High condition number amplifies tiny input error.',
    out: 'float32 = 0.000000\nshadow  = 0.000023\nrel_err = >> threshold' },
  { id: 'TC6', file: 'tc6_newton.cpp', name: 'Newton reciprocal', status: 'silent', err: '0.00e+00',
    desc: 'Division-free reciprocal via xₙ₊₁ = xₙ(2 − a·xₙ). Quadratic convergence lands float and shadow together.',
    out: 'TC6 [Newton Reciprocal - SILENT] result=0.333333 rel_err=0.00e+00' },
  { id: 'TC7', file: 'tc7_variance.cpp', name: 'One-pass variance', status: 'warn', err: '4.54e+04',
    desc: 'σ² = Σx²/N − mean². Two nearly-equal huge numbers subtracted; the tiny true variance is destroyed.',
    out: 'float32 = +45400.00\nshadow  = +1.000000\nrel_err = 4.540e+04 (threshold 1.0e-05)' },
  { id: 'TC8', file: 'tc8_exact_sum.cpp', name: 'Exact integer sum', status: 'silent', err: '0.00e+00',
    desc: 'Integers as floats, summed below 2²⁴ — every operation is exactly representable, no rounding occurs.',
    out: 'TC8 [Exact Integer Sum - SILENT] result=5050.000000 ref=5050 rel_err=0.00e+00' },
  { id: 'TC9', file: 'tc9_fma.cpp', name: 'FMA near-zero cancellation', status: 'warn', err: '8.22e+05',
    desc: 'a·b + c where a·b ≈ −c. Sequential rounding vs fused multiply-add diverges most extremely in the suite.',
    out: 'float32 = +8.222e+05\nshadow  = +1.000000\nrel_err = 8.220e+05 (threshold 1.0e-05)' },
  { id: 'TC10', file: 'tc10_sigmoid.cpp', name: 'Sigmoid, stable domain', status: 'silent', err: '0.00e+00',
    desc: 'S(x)=1/(1+e⁻ˣ) for x in [−5,5] — well-conditioned. Critical false-positive control for ML workloads.',
    out: 'TC10 [Sigmoid - SILENT] result=0.731059 rel_err=0.00e+00' },
  { id: 'TC11', file: 'tc11_mixed_dot.cpp', name: 'Mixed-magnitude dot product', status: 'warn', err: '>1e-05',
    desc: 'Alternating huge/tiny vector elements — large terms mask small ones that the shadow lane still retains.',
    out: 'float32 = 0.000000\nshadow  = 0.001234\nrel_err = >> threshold' },
  { id: 'TC12', file: 'tc12_newton_sqrt.cpp', name: 'Newton–Raphson √2', status: 'silent', err: '0.00e+00',
    desc: 'xₙ₊₁ = (xₙ + 2/xₙ)/2 — quadratic convergence again lands both lanes on the identical rounded answer.',
    out: 'TC12 [Newton sqrt - SILENT] result=1.414214 ref=1.414214 rel_err=0.00e+00' },
];

function renderTestGrid(filter) {
  const grid = document.getElementById('tc-grid');
  grid.innerHTML = '';
  testCases.filter(tc => filter === 'all' || tc.status === filter).forEach(tc => {
    const card = document.createElement('div');
    card.className = 'tc-card';
    card.innerHTML = `
      <span class="tc-badge badge ${tc.status === 'warn' ? 'warn' : 'pass'}">${tc.status === 'warn' ? 'WARN' : 'SILENT'}</span>
      <div class="tc-id">${tc.id} · ${tc.file}</div>
      <div class="tc-name">${tc.name}</div>
      <div class="tc-err">rel_err ≈ ${tc.err}</div>
    `;
    card.addEventListener('click', () => openTcDetail(tc));
    grid.appendChild(card);
  });
}

function openTcDetail(tc) {
  const detail = document.getElementById('tc-detail');
  detail.classList.add('show');
  detail.innerHTML = `
    <div class="dt-title">${tc.id} — ${tc.name}</div>
    ${tc.desc}

    ${tc.out}
  `;
  const btn = document.createElement('button');
  btn.className = 'tc-run-live';
  btn.textContent = '▶ Run this file through nsan-clang++';
  btn.addEventListener('click', () => runTcLive(tc, detail, btn));
  detail.appendChild(document.createElement('br'));
  detail.appendChild(btn);
  const out = document.createElement('pre');
  out.className = 'terminal';
  out.style.display = 'none';
  out.id = 'tc-live-output';
  detail.appendChild(out);
  detail.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

async function runTcLive(tc, detail, btn) {
  const out = document.getElementById('tc-live-output');
  if (!backendReady()) {
    out.style.display = 'block';
    out.textContent = 'server.py is not connected (or the plugin isn\'t built yet) — see the status bar at the top of the page.';
    return;
  }
  btn.disabled = true;
  btn.textContent = 'Compiling…';
  out.style.display = 'block';
  out.textContent = `$ nsan-clang++ -fsanitize=numerical ${tc.file} && ./${tc.id.toLowerCase()}\n\nrunning…`;
  try {
    const res = await fetch('/api/run', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tc: tc.file }),
    });
    const data = await res.json();
    if (!data.ok) {
      out.textContent = `$ nsan-clang++ -fsanitize=numerical ${tc.file}\n\n[compile failed]\n${data.error || (data.stderr) || 'unknown error'}`;
    } else {
      const combined = (data.run?.stdout || '') + (data.run?.stderr || '');
      out.textContent = `$ nsan-clang++ -fsanitize=numerical ${tc.file} -o ${tc.id.toLowerCase()} && ./${tc.id.toLowerCase()}\n\n${combined.trim() || '(no output)'}`;
    }
  } catch (e) {
    out.textContent = 'Request failed: ' + e;
  } finally {
    btn.disabled = false;
    btn.textContent = '▶ Run this file through nsan-clang++';
  }
}

function wireTestSuite() {
  renderTestGrid('all');
  document.querySelectorAll('.filters button').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.filters button').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      document.getElementById('tc-detail').classList.remove('show');
      renderTestGrid(btn.dataset.filter);
    });
  });
}

// ============ Performance chart ============
const perfData = [
  { name: 'NSan (double)', value: 3, display: '2–4×', color: 'var(--pass)' },
  { name: 'NSan (fp128)', value: 17, display: '~17×', color: 'var(--shadow-color)' },
  { name: 'Herbgrind (2017)', value: 100, display: '~100×', color: 'var(--float-color)' },
  { name: 'FpDebug / Valgrind', value: 476, display: '~476×', color: 'var(--float-color)' },
  { name: 'Verificarlo (MCA)', value: 40000, display: '~40,000×', color: 'var(--warn)' },
];

function renderPerfChart() {
  const maxLog = Math.log10(40000);
  const chart = document.getElementById('perf-chart');
  perfData.forEach(d => {
    const row = document.createElement('div');
    row.className = 'perf-row';
    const pct = Math.max(3, (Math.log10(d.value) / maxLog) * 100);
    row.innerHTML = `
      <div class="pname">${d.name}</div>
      <div class="pbar-track"><div class="pbar" style="width:${pct}%; background:${d.color};"></div></div>
      <div class="pval">${d.display}</div>
    `;
    chart.appendChild(row);
  });
}

// ============ Navigation Tabs (SPA Mode) ============
function initNavigationTabs() {
  const tabs = document.querySelectorAll('nav .nav-tab');
  const sections = document.querySelectorAll('main section.tab-section');
  const introSection = document.querySelector('main .intro');

  function switchTab(tabId) {
    tabs.forEach(t => {
      if (t.dataset.tab === tabId) t.classList.add('active');
      else t.classList.remove('active');
    });

    if (introSection) {
      introSection.style.display = (tabId === 'flow') ? 'block' : 'none';
    }

    sections.forEach(s => {
      if (s.id === tabId) {
        s.classList.add('active');
      } else {
        s.classList.remove('active');
      }
    });

    if (history.replaceState) {
      history.replaceState(null, null, '#' + tabId);
    } else {
      location.hash = tabId;
    }

    if (tabId === 'ir' && typeof renderIRLive === 'function' && irUiMode === 'live') {
      renderIRLive();
    }
  }

  tabs.forEach(tab => {
    tab.addEventListener('click', (e) => {
      e.preventDefault();
      switchTab(tab.dataset.tab);
    });
  });

  const initialHash = location.hash.replace('#', '');
  const validTabs = ['flow', 'ir', 'lab', 'tests', 'perf'];
  if (initialHash && validTabs.includes(initialHash)) {
    switchTab(initialHash);
  } else {
    switchTab('flow');
  }
}

// ============ init ============
document.addEventListener('DOMContentLoaded', async () => {
  initNavigationTabs();
  await checkHealth();
  await populateTcSelect();
  wireLab();
  wireIR();
  wireTestSuite();
  renderPerfChart();
  renderIRSim();
  setInterval(checkHealth, 15000); // keep status bar honest if server starts later
});

