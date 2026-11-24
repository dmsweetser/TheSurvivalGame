// audit130-kb.js — v1.30 "EVERY BUTTON" end-to-end USER audit, keyboard
// + mouse phase. Real player paths only (Playwright key events, real
// mouse clicks); dbg_* seams OBSERVE state, dbg_tp stages position
// (the e2e-pad precedent), dbg_give* stage inventory. Findings print
// as FINDING lines; OK lines record verified-good coverage.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const SHOT = __dirname + '/v130-shots/';
require('fs').mkdirSync(SHOT, { recursive: true });
const sleep = ms => new Promise(r => setTimeout(r, ms));
let ok = 0, find = 0;
function OK(name) { ok++; console.log('OK', name); }
function F(sev, name, extra) { find++; console.log('FINDING', sev, name, extra !== undefined ? '-> ' + String(extra).slice(0, 200) : ''); }
function T(name, cond, extra) { if (cond) OK(name); else F('DEFECT?', name, extra); }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(120); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const shot = async n => { await page.screenshot({ path: SHOT + n }); console.log('SHOT', n); };
  const str = fn => page.evaluate(f => { const ptr = window.__TSG[f](); const mem = new Uint8Array(window.__TSG.memory.buffer); let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]); return s; }, fn);
  const lbl = act => page.evaluate(a => { const ptr = window.__TSG.dbg_lbl(a); const mem = new Uint8Array(window.__TSG.memory.buffer); let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]); return s; }, act);
  // game-coord -> page px for mouse clicks
  const clickGame = async (gx, gy) => { const box = await page.locator('#cv').boundingBox(); await page.mouse.click(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height); await sleep(400); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);

  // ---------- TITLE (keyboard flavour: no save yet) ----------
  T('title: state is menu', (await call('dbg_state')) === 0, await call('dbg_state'));
  T('title: device reads keyboard', (await call('dbg_dev')) === 0, await call('dbg_dev'));
  const n0 = await call('dbg_menucount');
  console.log('NOTE menucount(no save, kb)=', n0);
  await shot('title-kb.png');
  // labels the HUD will use on keyboard
  console.log('NOTE kb labels: fire=', await lbl(0), 'int=', await lbl(1), 'wall=', await lbl(2), 'bean=', await lbl(3));
  // HOW TO PLAY (row 1)
  await key('ArrowDown'); await key('Space', 200, 400);
  T('help opens from title', (await poll('dbg_state', v => v === 4, 4000)) === 4, await call('dbg_state'));
  await shot('help-kb.png');
  await key('Escape', 200, 400);
  T('ESC leaves help back to menu', (await poll('dbg_state', v => v === 0, 4000)) === 0, await call('dbg_state'));
  // SETTINGS (row 2 in kb flavour)
  await key('ArrowDown'); await key('ArrowDown'); await key('Space', 200, 500);
  T('settings opens', (await poll('dbg_state', v => v === 6, 4000)) === 6, await call('dbg_state'));
  await shot('settings-kb.png');
  await key('ArrowDown'); await key('ArrowDown'); await key('ArrowUp');
  await shot('settings-kb-nav.png');
  await key('Escape', 200, 400);
  T('ESC leaves settings to menu', (await poll('dbg_state', v => v === 0, 4000)) === 0, await call('dbg_state'));
  // NAME field: rows are 0..n-1 items, row n = name field
  for (let i = 0; i < n0; i++) await key('ArrowDown', 120, 130);
  await page.keyboard.type('AUDIT', { delay: 60 }); await sleep(300);
  const nm = await str('dbg_myname');
  T('name field accepts typed letters', /AUDIT/.test(nm), nm);
  // START -> slots
  for (let i = 0; i < n0 + 1; i++) await key('ArrowUp', 120, 130);   // wrap back to row 0 area
  await key('ArrowDown', 120, 130); await key('ArrowUp', 120, 130); // settle on row 0 (START) if wrapped right
  await key('Space', 200, 500);
  let st = await call('dbg_state');
  if (st !== 7 && st !== 1) { // selection may not be on START; use Enter on row0 via ups to top
    for (let i = 0; i < 10; i++) await key('ArrowUp', 100, 110);
    await key('Space', 200, 500); st = await call('dbg_state');
  }
  T('START opens the slot picker', st === 7 || st === 1, st);
  if (st === 7) { await shot('slots-kb.png'); await key('Space', 200, 600); }
  T('slot pick starts the run', (await poll('dbg_state', v => v === 1, 6000)) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  // ---------- OVERWORLD (keyboard) ----------
  const x0 = await call('dbg_px'), y0 = await call('dbg_py');
  await hold('KeyD', 700); const x1 = await call('dbg_px');
  T('D walks east', x1 > x0 + 15, x0 + '->' + x1);
  await hold('ArrowUp', 700); const y1 = await call('dbg_py');
  T('ArrowUp walks north', y1 < y0 - 15, y0 + '->' + y1);
  await hold('KeyA', 500); await hold('ArrowDown', 500);
  // ESC in play: does it pause?
  await key('Escape', 200, 500);
  st = await call('dbg_state');
  console.log('NOTE state after ESC in play =', st, '(5=pause)');
  if (st === 5) { await key('Escape', 200, 400); T('ESC unpause works', (await poll('dbg_state', v => v === 1, 3000)) === 1, await call('dbg_state')); }
  // P pause + controls panel
  await key('KeyP', 200, 500);
  T('P opens pause', (await poll('dbg_state', v => v === 5, 4000)) === 5, await call('dbg_state'));
  await shot('pause-kb.png');
  await key('ArrowDown'); await key('Space', 200, 500);   // CONTROLS row
  await shot('controls-kb.png');
  await key('Escape', 200, 500);
  st = await call('dbg_state');
  console.log('NOTE state after ESC from controls panel =', st, '(5=pause rows, 1=play)');
  if (st === 5) { await key('Escape', 200, 400); }
  T('resumed to play', (await poll('dbg_state', v => v === 1, 4000)) === 1, await call('dbg_state'));
  // bean on key 6
  await call('dbg_givebean'); await call('dbg_tp', 0, 900); await sleep(250);   // far from the plaza
  await key('Digit6', 200, 300);
  T('key 6 fires the Backfire Bean', (await poll('dbg_beant', v => v > 0, 4000)) > 0, await call('dbg_beant'));
  await sleep(2500);
  // ---------- MOUSE on title ----------
  await key('KeyP', 200, 500);
  await key('ArrowDown'); await key('ArrowDown'); await key('Space', 200, 600);  // SAVE + QUIT
  T('save+quit returns to title', (await poll('dbg_state', v => v === 0, 5000)) === 0, await call('dbg_state'));
  const n1 = await call('dbg_menucount');
  console.log('NOTE menucount(with save, kb)=', n1);
  await shot('title-kb-save.png');
  // click CONTINUE (row 0, y=108+9=117 centre of the bar)
  await clickGame(320, 117);
  st = await call('dbg_state');
  T('mouse click on CONTINUE opens slots', st === 7, st);
  if (st === 7) {
    await clickGame(320, 138);   // slot card 1 centre (y=116..160)
    T('mouse click on slot card starts the run', (await poll('dbg_state', v => v === 1, 5000)) === 1, await call('dbg_state'));
  }
  // mouse in play: press-and-hold left of the player (drag channel) — does the player walk?
  if ((await call('dbg_state')) === 1) {
    const bx = await page.locator('#cv').boundingBox();
    const px0 = await call('dbg_px');
    await page.mouse.move(bx.x + bx.width * 0.25, bx.y + bx.height * 0.5);
    await page.mouse.down(); await sleep(1400); await page.mouse.up(); await sleep(300);
    const px1 = await call('dbg_px');
    console.log('NOTE mouse drag-hold in play moved px', px0, '->', px1);
  }

  // ---------- MALL stations (keyboard) ----------
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  T('mall door opens with E', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  const seen = {};
  for (const sx of [-280, -210, -140, -70, 0, 70, 140, 210, 280]) {
    await call('dbg_tp', sx, -5940); await sleep(160);
    const s = await call('dbg_station');
    if (s >= 0 && seen[s] === undefined) seen[s] = [sx, -5940];
  }
  for (const sx of [-280, -180, -70, 0, 70, 180, 280]) {
    await call('dbg_tp', sx, -5868); await sleep(160);
    const s = await call('dbg_station');
    if (s >= 0 && seen[s] === undefined) seen[s] = [sx, -5868];
  }
  console.log('NOTE stations found:', JSON.stringify(seen));
  for (const [sid, pos] of Object.entries(seen)) {
    await call('dbg_tp', pos[0], pos[1]); await sleep(250);
    await key('KeyE', 300);
    const inStore = await poll('dbg_store1', v => v === 1, 3000);
    const stateNow = await call('dbg_state');
    console.log('NOTE station', sid, 'opened: store1=', inStore, 'state=', stateNow);
    if (inStore === 1 || stateNow === 2) {
      OK('station ' + sid + ' panel opens with E');
      if (sid === '0') await shot('store-kb.png');
      if (sid === '4') await shot('bowl-kb.png');
      await key('ArrowDown'); await key('ArrowUp');
      await key('Escape', 200, 400);
      const closed = (await call('dbg_store1')) === 0;
      T('station ' + sid + ' panel closes with ESC', closed, await call('dbg_store1'));
      if (!closed) { await key('KeyE', 300); }
    } else F('DEFECT?', 'station ' + sid + ' panel did not open with E', 'store1=' + inStore + ' state=' + stateNow);
  }

  // ---------- SOURCE ROOM (keyboard) ----------
  await call('dbg_givetrophy'); await call('dbg_givetrophy'); await call('dbg_givetrophy');
  await call('dbg_tp', -210, -6180); await sleep(250);
  await key('KeyE', 300);
  const srcIn = await poll('dbg_mall', v => v === 1, 2000); // still mall-flagged; room entry observed via terminal below
  await call('dbg_tp', 0, -18100); await sleep(250);   // inside the Source Room (suite precedent)
  await key('KeyE', 300);
  T('source terminal opens', (await poll('dbg_srcterm', v => v === 1, 4000)) === 1, await call('dbg_srcterm'));
  await shot('src-browser-kb.png');
  await key('ArrowDown', 120, 150); await key('ArrowDown', 120, 150);
  await key('Space', 300);
  T('editor opens on a file', (await poll('dbg_srcterm', v => v === 2, 4000)) === 2, await call('dbg_srcterm'));
  await shot('src-editor-kb.png');
  await key('Escape', 200, 400);
  T('ESC returns editor -> browser', (await poll('dbg_srcterm', v => v === 1, 3000)) === 1, await call('dbg_srcterm'));
  await key('Escape', 200, 400);
  T('ESC closes the terminal', (await poll('dbg_srcterm', v => v === 0, 3000)) === 0, await call('dbg_srcterm'));

  // ---------- ARCADE cabinets (keyboard) ----------
  await call('dbg_tp', 350, -5976); await sleep(250);
  await key('KeyE', 300);
  T('arcade door opens with E', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  const CABS = [
    ['ENTRANCES', -256, -12108, 2], ['SUPER BUDDY RUN', 0, -12108, 3], ['BUDDY CRAFT', 256, -12108, 4],
    ['TANK RESCUE', -256, -11898, 5], ['CASTLE', 0, -11898, 6], ['SUPER BUDDY CART', 256, -11898, 7],
    ['WAR!', 0, -11758, 8], ['MORDOR FUN RUN', -256, -11758, 9],
  ];
  for (const [name, cx, cy, mode] of CABS) {
    await call('dbg_tp', cx, cy); await sleep(250);
    const cab = await call('dbg_cabinet');
    await key('KeyE', 300);
    const gate = await poll('dbg_arcgate', v => v === 1, 3000);
    if (gate !== 1) { F('DEFECT?', name + ': cabinet panel did not open with E', 'cab=' + cab + ' gate=' + gate); continue; }
    OK(name + ': panel opens (cabinet ' + cab + ')');
    if (name === 'WAR!') await shot('war-panel-kb.png');
    if (name === 'MORDOR FUN RUN') await shot('mordor-panel-kb.png');
    await key('Space', 300);
    const m = await poll('dbg_bwmode', v => v === mode, 5000);
    T(name + ': launches mode ' + mode, m === mode, await call('dbg_bwmode'));
    if (m !== mode) { await call('dbg_bwexit'); await sleep(300); continue; }
    await sleep(600);
    if (name === 'MORDOR FUN RUN') await shot('mordor-road-kb.png');
    if (name === 'WAR!') await shot('war-field-kb.png');
    // exit the player's way: ESC first
    await key('Escape', 250, 600);
    let left = (await call('dbg_bwmode')) === -1;
    let how = 'ESC';
    if (!left) { await key('KeyP', 250, 600); left = (await call('dbg_bwmode')) === -1; how = 'P'; }
    if (!left) { await call('dbg_bwexit'); await sleep(400); left = (await call('dbg_bwmode')) === -1; how = 'dbg seam (no player exit found)'; }
    T(name + ': exits (' + how + ')', left && (await call('dbg_inarcade')) === 1, 'mode=' + (await call('dbg_bwmode')));
    if (how.startsWith('dbg')) F('DEFECT?', name + ': no keyboard exit found (ESC/P both failed)', '');
  }

  console.log('AUDIT-KB DONE: ok=' + ok + ' findings=' + find);
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
