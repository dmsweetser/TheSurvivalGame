// audioprobe.js — audio balance check (Nov 6): hooks js_note in the page
// (the shell hands the wasm a shorthand reference, so reassigning
// window.js_note after boot intercepts every note) and measures the
// per-voice peak gain the CURRENT shell synth would produce, per piece
// and per Entrances sound. Balance targets:
//   - every piece's per-step peak within [0.5x, 2x] of the cross-piece mean
//   - the loudest recurring Entrances voice no hotter than the hottest
//     simultaneous music step, and never above the old p90 sfx ceiling
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 200) : ''); }
}

// mirror of the shell synth's per-voice peak gain (shell.html js_note)
function voicePeak(ch, vel, instr) {
  const v = Math.min(1, vel / 110);
  if (instr === 3) return v * 0.7;                    // noise bus
  if (instr === 4) return v * (ch === 0 ? 0.26 : 0.32); // kazoo
  return v * (ch === 0 ? 0.30 : 0.36);
}

(async () => {
  const browser = await chromium.launch({
    executablePath: EXE,
    args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required']
  });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  // intercept at the source: wrap WebAssembly.instantiate so the game's
  // imports.env.js_note is a recording passthrough BEFORE the wasm binds
  // it (reassigning window.js_note afterwards does nothing — the env
  // object captured the original function by value at boot)
  await page.addInitScript(() => {
    window.__notes = []; window.__collect = 0;
    const wrap = (imports) => {
      try {
        const orig = imports.env.js_note;
        imports.env.js_note = function (ch, note, vel, instr) {
          if (window.__collect) window.__notes.push([ch, vel, instr]);
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
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  // a leftover save boots into the slots menu instead of the title —
  // clear storage and reload for a clean title, like the other suites
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);

  // collect a window of {ch, vel, instr} notes (hook installed pre-boot)
  async function collect(sec) {
    await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
    await sleep(sec * 1000);
    return page.evaluate(() => { window.__collect = 0; return window.__notes; });
  }

  // start the game from the title (audio engine wakes on the gesture);
  // the menu is a key list — a held Space selects the highlighted row,
  // and if the cursor sits elsewhere, Enter still starts (suite pattern)
  await page.keyboard.down('Space'); await sleep(400); await page.keyboard.up('Space');
  if ((await call('dbg_state')) !== 1) { await page.keyboard.down('Enter'); await sleep(400); await page.keyboard.up('Enter'); }
  await sleep(400);
  ok((await call('dbg_state')) === 1, 'game started from title', await call('dbg_state'));

  // ---- pieces: per-step peaks AND time-averaged power. The step-peak
  // median misreads dense pieces (a fast arpeggio has quiet single-note
  // steps but constant overlap), so the balance target is energy RMS:
  // sqrt(sum(peak^2 * voice_dur) / window_seconds).
  function pieceStats(notes, sec) {
    const steps = [];
    let cur = null;
    let energy = 0;
    for (const [ch, vel, instr] of notes) {
      if (ch === 3) continue;
      const pk = voicePeak(ch, vel, instr);
      const dur = instr === 3 ? 0.12 : (ch === 1 ? 0.24 : 0.16);
      energy += pk * pk * dur;
      if (!cur || (cur[ch] !== undefined)) { cur = {}; steps.push(cur); }
      if (cur[ch] === undefined || vel > cur[ch]) cur[ch] = pk;
    }
    const peaks = steps.map(s => Object.values(s).reduce((a, b) => a + b, 0)).filter(p => p > 0);
    peaks.sort((a, b) => a - b);
    const n = peaks.length;
    const med = n ? peaks[n >> 1] : 0;
    return { n, med, max: n ? peaks[n - 1] : 0, rms: Math.sqrt(energy / sec) };
  }

  const pieceNames = ['Moonlight I (menu/calm)', 'Fur Elise', 'Symphony 5', 'Moonlight III', 'Bach (BW)'];
  const pieceStatsAll = [];
  // piece 0 (calm, in game)
  pieceStatsAll.push({ name: pieceNames[0], ...pieceStats(await collect(4), 4) });
  // pieces 1..3 via threat override
  for (const [t, idx] of [[25, 1], [45, 2], [70, 3]]) {
    await call('dbg_setthreat', t);
    await sleep(500);
    ok((await call('dbg_piece')) === idx, 'threat override selects piece ' + idx, await call('dbg_piece'));
    pieceStatsAll.push({ name: pieceNames[idx], ...pieceStats(await collect(4), 4) });
  }
  await call('dbg_setthreat', -1);
  // piece 4: Banzai World free roam (dbg_bwent2 is Entrances = mode 2,
  // which plays the Moonlight by design; the Bach belongs to roam mode)
  await call('dbg_adopt', 0);
  await call('dbg_bwenter');
  await sleep(600);
  ok((await call('dbg_piece')) === 4, 'Banzai World selects piece 4 (Bach)', await call('dbg_piece'));
  pieceStatsAll.push({ name: pieceNames[4], ...pieceStats(await collect(4), 4) });
  await call('dbg_bwent2');   // now switch into Entrances for the chase

  console.log('--- music pieces (master units) ---');
  for (const s of pieceStatsAll) console.log(
    s.name.padEnd(24), 'steps', String(s.n).padStart(3),
    'step-med', s.med.toFixed(3), 'step-max', s.max.toFixed(3), 'energy-RMS', s.rms.toFixed(3));
  const rmss = pieceStatsAll.map(s => s.rms).filter(v => v > 0);
  const meanRms = rmss.reduce((a, b) => a + b, 0) / rmss.length;
  for (const s of pieceStatsAll)
    ok(s.rms >= meanRms * 0.6 && s.rms <= meanRms * 1.67,
      'piece "' + s.name + '" loudness within band of the others', 'rms ' + s.rms.toFixed(3) + ' vs mean ' + meanRms.toFixed(3));
  const musicMax = Math.max(...pieceStatsAll.map(s => s.max));
  ok(musicMax <= 0.85, 'no music step clips against the master', musicMax.toFixed(3));

  // ---- Entrances sounds: walk to room 19, arm the chase, sample its
  // pulse; then force passes with dbg_enttrig in this room (ev 1/2 live
  // here too, but room 19's chase is the recurring loud bed)
  const maxhp = await call('dbg_buddyhp');
  await call('dbg_budup', 60);
  for (let i = 0; i < 40 && (await call('dbg_entroom')) !== 19; i++) {
    // advance east by teleport: stand in the next room's west side
    const r = await call('dbg_entroom');
    await page.evaluate(([x, y]) => window.__TSG.dbg_bwtp(x, y), [(r) * 15 * 48 + 360, 216]);
    await sleep(150);
  }
  ok((await call('dbg_entroom')) === 19, 'reached room 19 for the chase sample', await call('dbg_entroom'));
  await page.evaluate(() => window.__TSG.dbg_bwtp(18 * 15 * 48 + 360 + 120, 216));
  const chaseN = await collect(6);
  const chaseSfx = chaseN.filter(([ch]) => ch === 3);
  const chasePeaks = chaseSfx.map(([ch, vel, instr]) => voicePeak(ch, vel, instr));
  const chaseMax = chasePeaks.length ? Math.max(...chasePeaks) : 0;
  console.log('--- Entrances chase bed ---');
  console.log('chase sfx notes', chasePeaks.length, 'loudest voice peak', chaseMax.toFixed(3),
    '| music hottest step', musicMax.toFixed(3));
  ok(chasePeaks.length >= 10, 'chase pulse actually sounded during the sample', chasePeaks.length);
  ok(chaseMax <= 0.30, 'chase pulse voice no hotter than 0.30 (was 0.288 at vel 88)', chaseMax.toFixed(3));
  ok(chaseMax <= musicMax * 1.15, 'chase pulse sits with the music, not over it',
    'chase ' + chaseMax.toFixed(3) + ' vs music max ' + musicMax.toFixed(3));

  console.log('audioprobe: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
