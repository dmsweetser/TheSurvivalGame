// musictest.js — one piece per arcade cabinet (v1.16, Daniel):
// ENTRANCES gets Grieg's Mountain King, RUN gets Flight of the
// Bumblebee (Daniel's pick), TANK RESCUE the Infernal Galop, CASTLE
// Greensleeves, CART the William Tell finale, WAR! the Ride of the
// Valkyries (Daniel's pick). Buddy Craft and the Banzai World plaza
// keep Bach's Sheep May Safely Graze, and leaving Banzai World hands
// the music back to the main game's Beethoven ladder. The audio
// probe (js_note wrapped at instantiate, the audioprobe technique)
// confirms the RUN and WAR! leads really flow — and sound nothing
// alike: the bee is a chromatic swarm, the Valkyries a six-note
// horn call.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  // record every note the wasm asks the synth to play, from the
  // source: wrap instantiate so imports.env.js_note is a recording
  // passthrough before the game binds it
  await page.addInitScript(() => {
    window.__notes = []; window.__collect = 0;
    const wrap = (imports) => {
      try {
        const orig = imports.env.js_note;
        imports.env.js_note = function (ch, note, vel, instr) {
          if (window.__collect) window.__notes.push([ch, note]);
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
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const leadSet = async () => page.evaluate(() => {
    const s = new Set();
    for (const [ch, n] of window.__notes) if (ch === 0) s.add(n);
    return [...s].sort((a, b) => a - b);
  });
  const leadCount = async () => page.evaluate(() => window.__notes.filter(([ch]) => ch === 0).length);
  const collect = on => page.evaluate(o => { if (o) window.__notes = []; window.__collect = o ? 1 : 0; }, on);

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  T('calm main world still plays the ladder (Moonlight)', (await call('dbg_piece')) === 0, await call('dbg_piece'));

  // into the mall
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));

  // the gate's FREE PLAY: inside Banzai World but in no cabinet —
  // the plaza keeps Sheep (bw_piece returns 4 for bw_mode < 2)
  await call('dbg_tp', 215, -5868); await sleep(250);   // the Banzai gate disc (BW_SX 215, MALL_CY+132 — v1.25 MALL ENLARGEMENT)
  await key('KeyE', 250);
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('free play entered Banzai World', (await poll('dbg_bwmode', v => v === 0, 4000)) === 0, await call('dbg_bwmode'));
  T('the Banzai World plaza plays Sheep (piece 4)', (await call('dbg_piece')) === 4, await call('dbg_piece'));
  await call('dbg_bwexit'); await sleep(300);
  T('back in the mall, ladder restored', (await call('dbg_mall')) === 1 && (await call('dbg_piece')) === 0,
    'piece=' + (await call('dbg_piece')));

  // through the east door into THE ARCADE
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));

  const CABS = [
    { tp: [-256, -12108], cab: 0, mode: 2, piece: 5,  name: 'ENTRANCES (Mountain King)' },
    { tp: [0, -12108],    cab: 1, mode: 3, piece: 6,  name: 'RUN (Flight of the Bumblebee)', probe: 5000, minNotes: 40 },
    { tp: [256, -12108],  cab: 2, mode: 4, piece: 4,  name: 'CRAFT (Sheep, unchanged)' },
    { tp: [-256, -11898], cab: 3, mode: 5, piece: 7,  name: 'TANK RESCUE (Infernal Galop)' },
    { tp: [0, -11898],    cab: 4, mode: 6, piece: 8,  name: 'CASTLE (Greensleeves)' },
    { tp: [256, -11898],  cab: 5, mode: 7, piece: 9,  name: 'CART (William Tell finale)' },
    { tp: [0, -11688],    cab: 6, mode: 8, piece: 10, name: 'WAR! (Ride of the Valkyries)', probe: 6000, minNotes: 6 },
  ];
  const probeSets = {};
  for (const c of CABS) {
    await call('dbg_tp', c.tp[0], c.tp[1]); await sleep(200);
    T('standing at the ' + c.name + ' cabinet', (await call('dbg_cabinet')) === c.cab, await call('dbg_cabinet'));
    await key('KeyE', 250);
    T(c.name + ': panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
    if (c.probe) await collect(true);
    await key('Space', 300);
    T(c.name + ': launched (bw_mode ' + c.mode + ')', (await poll('dbg_bwmode', v => v === c.mode, 4000)) === c.mode, await call('dbg_bwmode'));
    T(c.name + ': plays piece ' + c.piece, (await call('dbg_piece')) === c.piece, await call('dbg_piece'));
    if (c.probe) {
      await sleep(c.probe);
      await collect(false);
      probeSets[c.mode] = await leadSet();
      const n = await leadCount();
      // the bee plays a note nearly every 0.072s step; the Valkyries
      // motif is deliberately sparse (~4 notes per 1.8s bar) — each
      // cabinet's bar is set to its own piece's real density
      T(c.name + ': lead notes actually flowed', n >= c.minNotes, 'lead notes=' + n);
    }
    await call('dbg_bwexit'); await sleep(300);
    T(c.name + ': exited back to the arcade', (await call('dbg_inarcade')) === 1 && (await call('dbg_bwmode')) === -1,
      'mode=' + (await call('dbg_bwmode')));
  }

  // the two probed leads are different animals: the bee swarms
  // chromatically (G#4 = 68 in its buzz cell, a dozen-plus pitches),
  // the Valkyries are a bare horn call (A#4 = 70 in the motif, a
  // handful of pitches)
  const run = probeSets[3] || [], war = probeSets[8] || [];
  T('RUN lead is the bee: G#4 (68) in the swarm', run.includes(68), run.join(','));
  T('RUN lead is chromatic — 14+ distinct pitches', run.length >= 14, run.length + ' distinct: ' + run.join(','));
  T('WAR! lead is the Valkyries: A#4 (70) in the horn call', war.includes(70), war.join(','));
  T('WAR! lead is a bare motif — 9 or fewer distinct pitches', war.length > 0 && war.length <= 9, war.length + ' distinct: ' + war.join(','));
  T('the two leads are not the same piece', JSON.stringify(run) !== JSON.stringify(war));

  // out of the arcade, and the main game owns the music again
  await call('dbg_tp', -330, -11976); await sleep(200);
  await key('KeyE', 250);
  T('left the arcade to the mall', (await poll('dbg_inarcade', v => v === 0, 4000)) === 0 && (await call('dbg_mall')) === 1);
  T('the ladder owns the music again (piece 0)', (await call('dbg_piece')) === 0, await call('dbg_piece'));

  console.log('musictest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
