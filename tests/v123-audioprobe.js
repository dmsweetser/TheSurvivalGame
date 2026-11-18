// v123-audioprobe.js — v1.23 MIDI rehash balance check: for EVERY piece,
// collect js_note traffic for exactly one full loop (len x step seconds,
// so the measurement is phase-independent) and report note counts per
// channel plus energy RMS by the Nov 6 audioprobe method:
//   voicePeak(ch, vel, instr): v = min(1, vel/110); noise x0.7,
//   kazoo x(0.26/0.32), else x(ch0? 0.30 : 0.36)
//   voice dur: instr 3 -> 0.12, ch 1 -> 0.24, else 0.16
//   energy RMS = sqrt(sum(peak^2 * dur) / window_seconds), sfx ch excluded.
// Usage: node v123-audioprobe.js  (prints one line per piece + a JSON blob)
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));

function voicePeak(ch, vel, instr) {
  const v = Math.min(1, vel / 110);
  if (instr === 3) return v * 0.7;
  if (instr === 4) return v * (ch === 0 ? 0.26 : 0.32);
  return v * (ch === 0 ? 0.30 : 0.36);
}
// len and step mirror the PIECES table in src/music.c
const PIECES = [
  { name: 'M0 Moonlight I (title/calm)', len: 96, step: 0.205 },
  { name: 'M1 Fur Elise (threat 1)', len: 64, step: 0.170 },
  { name: 'M2 Symphony No.5 (threat 2)', len: 64, step: 0.145 },
  { name: 'M3 Moonlight III (threat 3)', len: 128, step: 0.078 },
  { name: 'M4 Sheep May Safely Graze (BW/Craft)', len: 336, step: 0.135 },
  { name: 'M5 Mountain King (Entrances)', len: 128, step: 0.140 },
  { name: 'M6 Bumblebee (Run)', len: 192, step: 0.072 },
  { name: 'M7 Infernal Galop (Tank)', len: 128, step: 0.100 },
  { name: 'M8 Greensleeves (Castle)', len: 96, step: 0.170 },
  { name: 'M9 William Tell (Cart)', len: 128, step: 0.095 },
  { name: 'M10 Valkyries (WAR!)', len: 96, step: 0.150 },
];

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    window.__notes = []; window.__collect = 0;
    const wrap = (imports) => {
      try {
        const orig = imports.env.js_note;
        imports.env.js_note = function (ch, note, vel, instr) {
          if (window.__collect) window.__notes.push([ch, note, vel, instr]);
          return orig(ch, note, vel, instr);
        };
      } catch (e) {}
      return imports;
    };
    const inst = WebAssembly.instantiate.bind(WebAssembly);
    WebAssembly.instantiate = (bytes, imports) => inst(bytes, wrap(imports));
    const instS = WebAssembly.instantiateStreaming && WebAssembly.instantiateStreaming.bind(WebAssembly);
    if (instS) WebAssembly.instantiateStreaming = (src, imports) => instS(src, wrap(imports));
  });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(400);
  if ((await call('dbg_state')) !== 1) { console.log('ERR game did not start'); process.exit(1); }
  await call('dbg_adopt', 0);

  async function collect(sec) {
    await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
    await sleep(sec * 1000);
    return page.evaluate(() => { window.__collect = 0; return window.__notes; });
  }
  function stats(notes, sec) {
    let energy = 0, maxStep = 0;
    const counts = [0, 0, 0];
    let cur = null; const steps = [];
    for (const [ch, note, vel, instr] of notes) {
      if (ch === 3) continue;                    // sfx channel: not music
      if (ch === 0 && (instr === 3 || instr === 6)) continue;  // cart pad/jingle one-offs on ch 0
      if (instr === 3) { energy += Math.pow(voicePeak(ch, vel, instr), 2) * 0.12; continue; }  // storm heartbeat (ch2 noise): energy only, not a voice count
      counts[ch]++;
      const pk = voicePeak(ch, vel, instr);
      const dur = ch === 1 ? 0.24 : 0.16;
      energy += pk * pk * dur;
      if (!cur || cur[ch] !== undefined) { cur = {}; steps.push(cur); }
      if (cur[ch] === undefined || pk > cur[ch]) cur[ch] = pk;
    }
    for (const s of steps) { const t = Object.values(s).reduce((a, b) => a + b, 0); if (t > maxStep) maxStep = t; }
    return { lead: counts[0], bass: counts[1], mid: counts[2], total: counts[0] + counts[1] + counts[2],
             rms: Math.sqrt(energy / sec), maxStep };
  }
  async function measure(idx) {
    const P = PIECES[idx];
    const got = await poll('dbg_piece', v => v === idx, 9000);
    if (got !== idx) { console.log('ERR piece never became', idx, 'stuck at', got); process.exit(1); }
    await sleep(700);   // let the previous piece's tail clear
    const sec = P.len * P.step;
    const notes = await collect(sec);
    const s = stats(notes, sec);
    console.log(P.name.padEnd(36), 'notes L/B/M', String(s.lead).padStart(3), String(s.bass).padStart(3), String(s.mid).padStart(3),
      'total', String(s.total).padStart(3), 'RMS', s.rms.toFixed(4), 'maxStep', s.maxStep.toFixed(3));
    return { idx, ...s };
  }

  const out = [];
  out.push(await measure(0));                       // calm overworld
  // Banzai block while the session is calm (entering BW after the
  // threat ladder leaves the hop fighting a hot overworld)
  for (let t = 0; t < 6 && (await call('dbg_bwmode')) < 0; t++) { await call('dbg_bwenter'); await sleep(800); }
  out.push(await measure(4));                       // Banzai World roam
  for (const [mode, idx] of [[2, 5], [3, 6], [5, 7], [6, 8], [7, 9], [8, 10]]) {
    await call('dbg_bwentm', mode); await sleep(500);
    out.push(await measure(idx));
  }
  await call('dbg_bwexit'); await sleep(800);
  for (const [t, idx] of [[25, 1], [45, 2], [70, 3]]) { await call('dbg_setthreat', t); out.push(await measure(idx)); }
  await call('dbg_setthreat', -1);
  console.log('JSON ' + JSON.stringify(out));
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
