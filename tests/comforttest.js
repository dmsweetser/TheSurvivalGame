// comforttest.js — v1.25 feature 2: COMFORT SETTINGS.
// The settings screen gains four rows over the key bindings — MUSIC
// VOLUME, SOUND FX VOLUME, MUTE ALL, SCREEN SHAKE — persisted in the
// global blob (volm/vols/mute/shk line keys) and applied at the one
// audio funnel (update_audio in music.c) and the one shake gate
// (fx_shake in world.c). dbg_comfort packs the state:
// vol_music*10000 + vol_sfx*100 + mute_all*10 + shake_on (defaults
// read 101001). Audio is verified through the Nov 6 js_note probe:
// wrap WebAssembly.instantiate so imports.env.js_note records every
// note (ch, vel, instr) before the wasm binds it.
// Title-menu layouts (keyboard flavour): no save = 6 rows (SETTINGS
// at 2, ZEN at 4); with a save = 7 rows (CONTINUE first, SETTINGS
// at 3). menuGoTo navigates by the live dbg_menusel, never by
// counting from an assumed start.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
const median = a => { if (!a.length) return -1; const s = [...a].sort((x, y) => x - y); return s[s.length >> 1]; };

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
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 12000) => {
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(200); }
    return v;
  };
  const collect = async ms => {
    await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
    await sleep(ms);
    return page.evaluate(() => { window.__collect = 0; return window.__notes; });
  };
  const blob = () => page.evaluate(() => localStorage.getItem('tsg_save') || '');
  async function bootFresh() {
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(400);
  }
  async function menuGoTo(idx) {
    // menu_sel walks the items PLUS the name-field row below them
    // (menu_input wraps over menu_count + MENU_FIELDS), so the
    // cycle length is dbg_menun + 1 on this build.
    const n = (await call('dbg_menun')) + 1;
    let cur = await call('dbg_menusel');
    const steps = ((idx - cur) % n + n) % n;
    for (let i = 0; i < steps; i++) await key('ArrowDown', 110);
    return (await call('dbg_menusel')) === idx;
  }
  async function openSettings(settingsIdx) {
    await menuGoTo(settingsIdx);
    await key('Space', 400);
    return (await poll('dbg_state', v => v === 6, 6000)) === 6;
  }

  // ============ defaults + the music at full volume ============
  await bootFresh();
  ok((await call('dbg_state')) === 0, 'boots to the title menu');
  ok((await call('dbg_comfort')) === 101001, 'comfort defaults are 10/10, unmuted, shake on', await call('dbg_comfort'));
  const full = await collect(3000);
  const fullMid = full.filter(n => n[0] === 2).map(n => n[1]);
  ok(full.filter(n => n[0] <= 2).length > 10, 'menu music is playing (notes on ch 0-2)', full.length);
  ok(median(fullMid) === 40, 'arp channel at full menu velocity 40', median(fullMid));

  // ============ music volume halves the sequencer ============
  ok(await openSettings(2), 'settings screen opens');
  for (let i = 0; i < 5; i++) await key('ArrowLeft', 120);
  ok((await call('dbg_comfort')) === 51001, 'MUSIC VOLUME left x5 -> 5', await call('dbg_comfort'));
  const half = await collect(3000);
  const halfMid = half.filter(n => n[0] === 2).map(n => n[1]);
  ok(median(halfMid) === 20, 'arp channel halved to velocity 20', median(halfMid));
  const ratio = median(halfMid) / median(fullMid);
  ok(ratio > 0.35 && ratio < 0.65, 'music velocity ratio ~0.5', ratio);

  // ============ sfx: the preview click scales with the knob ============
  await key('ArrowDown', 120);   // SFX row (selected row 1)
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  await key('ArrowLeft', 350);   // 10 -> 9: the row previews sfx(1), vel 34 -> 30
  await sleep(400);
  const clickNotes = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  ok(clickNotes.some(n => n[0] === 3 && n[1] === 30), 'preview click fired at scaled velocity 30', JSON.stringify(clickNotes.filter(n => n[0] === 3)));
  for (let i = 0; i < 9; i++) await key('ArrowLeft', 90);
  ok((await call('dbg_comfort')) === 50001, 'SFX VOLUME down to 0', await call('dbg_comfort'));

  // ============ sfx at 0: a real menu action is silent ============
  await key('Escape', 400);
  ok((await call('dbg_state')) === 0, 'back at the title');
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  await menuGoTo(4);             // ZEN row
  await key('Space', 500);       // menu_activate plays sfx(6) — at vol 0 it must not exist
  const silent = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  ok(silent.filter(n => n[0] === 3).length === 0, 'SFX at 0: the menu toggle makes no ch-3 sound', JSON.stringify(silent.filter(n => n[0] === 3)));
  ok(silent.filter(n => n[0] <= 2).length > 0, '...while the music (vol 5) still plays');
  await key('Space', 300);       // ZEN back off (still silent)

  // ============ sfx restored: the same toggle is audible ============
  ok(await openSettings(2), 'settings reopens');
  await key('ArrowDown', 120);   // SFX row
  for (let i = 0; i < 10; i++) await key('ArrowRight', 90);
  ok((await call('dbg_comfort')) === 51001, 'SFX VOLUME back to 10', await call('dbg_comfort'));
  await key('Escape', 400);
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  await menuGoTo(4);
  await key('Space', 500);
  const audible = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  ok(audible.some(n => n[0] === 3 && n[1] === 22), 'SFX at 10: the toggle click lands at velocity 22', JSON.stringify(audible.filter(n => n[0] === 3)));
  await key('Space', 300);       // ZEN back off

  // ============ mute: everything stops, then everything returns ============
  ok(await openSettings(2), 'settings opens for mute');
  await key('ArrowDown', 120); await key('ArrowDown', 120);   // MUTE row
  await key('ArrowRight', 250);
  ok((await call('dbg_comfort')) === 51011, 'MUTE ALL on', await call('dbg_comfort'));
  await key('Escape', 400);
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  await menuGoTo(4); await key('Space', 300);   // a menu action while muted
  await sleep(2200);
  const muted = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  ok(muted.length === 0, 'muted: zero notes on every channel for 2.5s', muted.length);
  await key('Space', 300);       // ZEN back off (silently)
  ok(await openSettings(2), 'settings opens to unmute');
  await key('ArrowDown', 120); await key('ArrowDown', 120);
  await key('ArrowLeft', 250);
  ok((await call('dbg_comfort')) === 51001, 'MUTE ALL off again', await call('dbg_comfort'));
  const back = await collect(2000);
  ok(back.filter(n => n[0] <= 2).length > 5, 'unmuted: the music returns', back.length);
  await key('Escape', 400);

  // ============ persistence: values ride the global blob ============
  ok(await openSettings(2), 'settings opens for persistence');
  await key('ArrowDown', 120);   // SFX row
  for (let i = 0; i < 3; i++) await key('ArrowLeft', 100);
  await key('ArrowDown', 120); await key('ArrowDown', 120);   // SHAKE row
  await key('ArrowRight', 250);  // shake off
  ok((await call('dbg_comfort')) === 50700, 'distinct values set: music 5, sfx 7, shake off', await call('dbg_comfort'));
  await key('Escape', 400);
  const saved = await blob();
  ok(/(^|\n)volm=5(\n|$)/.test(saved), 'blob carries volm=5');
  ok(/(^|\n)vols=7(\n|$)/.test(saved), 'blob carries vols=7');
  ok(/(^|\n)mute=0(\n|$)/.test(saved), 'blob carries mute=0');
  ok(/(^|\n)shk=0(\n|$)/.test(saved), 'blob carries shk=0');
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(400);
  ok((await call('dbg_comfort')) === 50700, 'values survive a reload', await call('dbg_comfort'));

  // ============ an old save (no comfort lines) gets the defaults ============
  const stripped = saved.replace(/\n?(volm|vols|mute|shk)=[^\n]*/g, '');
  ok(!/volm=|vols=|shk=/.test(stripped) && /bind=/.test(stripped), 'strip leaves the rest of the blob intact');
  await page.evaluate(s => localStorage.setItem('tsg_save', s), stripped);
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(400);
  ok((await call('dbg_state')) === 0, 'old save still boots to the menu');
  ok((await call('dbg_comfort')) === 101001, 'old save loads comfort defaults', await call('dbg_comfort'));

  // ============ shake: the gate governs the value and the camera ============
  await bootFresh();
  await menuGoTo(0);             // START
  await key('Space', 500);       // slot picker (new game)
  await key('Enter', 500);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'run started');
  ok((await call('dbg_fxshake')) === 300, 'shake ON: fx_shake raises G.shake to 3.00', await call('dbg_shake'));
  let maxOff = 0;
  for (let i = 0; i < 16; i++) { const v = Math.abs(await call('dbg_shkoff')); if (v > maxOff) maxOff = v; await sleep(55); }
  ok(maxOff >= 100, 'shake ON: the render camera actually jitters (>=1px)', maxOff);
  await sleep(1200);
  ok((await call('dbg_shake')) === 0, 'shake decays back to exactly 0', await call('dbg_shake'));
  // turn shake OFF through the real screen, then continue the run
  await key('KeyP', 500);
  ok((await poll('dbg_state', v => v === 5, 6000)) === 5, 'pause opens');
  await key('ArrowDown', 120); await key('ArrowDown', 120);   // SAVE + QUIT (row 2, chain or not)
  await key('Space', 600);
  ok((await poll('dbg_state', v => v === 0, 8000)) === 0, 'saved + quit to title');
  ok(await openSettings(3), 'settings opens (save layout)');
  await key('ArrowDown', 120); await key('ArrowDown', 120); await key('ArrowDown', 120);   // SHAKE row
  await key('ArrowRight', 250);
  ok((await call('dbg_comfort')) === 101000, 'SCREEN SHAKE off', await call('dbg_comfort'));
  await key('Escape', 400);
  await menuGoTo(0);             // CONTINUE
  await key('Space', 500);       // load picker
  await key('Enter', 500);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'run continued');
  ok((await call('dbg_fxshake')) === 0, 'shake OFF: fx_shake cannot raise G.shake', await call('dbg_shake'));
  maxOff = 0;
  for (let i = 0; i < 12; i++) { const v = Math.abs(await call('dbg_shkoff')); if (v > maxOff) maxOff = v; await sleep(55); }
  ok(maxOff === 0, 'shake OFF: the camera never jitters', maxOff);

  // ============ the screen still does its old job ============
  await key('KeyP', 500);
  await key('ArrowDown', 120); await key('ArrowDown', 120);
  await key('Space', 600);       // SAVE + QUIT
  await poll('dbg_state', v => v === 0, 8000);
  ok(await openSettings(3), 'settings opens for the rebind check');
  for (let i = 0; i < 4; i++) await key('ArrowDown', 110);     // row 4 = MOVE UP
  await key('Space', 300);       // capture
  await key('KeyJ', 400);        // bind MOVE UP to J (PK 9)
  let b = await blob();
  ok(/(^|\n)bind=9,/.test(b), 'rebind still works: bind= now starts 9,', (b.match(/bind=[^\n]*/) || [''])[0]);
  for (let i = 0; i < 14; i++) await key('ArrowDown', 90);     // row 18 = RESET DEFAULTS
  await key('Space', 400);
  b = await blob();
  ok(/(^|\n)bind=22,/.test(b), 'RESET DEFAULTS restores W (22)', (b.match(/bind=[^\n]*/) || [''])[0]);
  ok((await call('dbg_comfort')) === 101001, 'RESET DEFAULTS restores the comfort four too', await call('dbg_comfort'));

  console.log('comforttest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(1); });
