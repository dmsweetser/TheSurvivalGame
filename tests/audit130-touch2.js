// audit130-touch2.js — v1.30 audit, touch phase 2: TRUE scheme runs.
// Boot A forces STICK via the title CONTROLS row (default is DRAG).
// Boot B stays in DRAG and tests the mall: enter by tapping the store,
// then try to use a station and the exit mat with taps alone.
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const SHOT = __dirname + '/v130-shots/';
require('fs').mkdirSync(SHOT, { recursive: true });
const sleep = ms => new Promise(r => setTimeout(r, ms));
let ok = 0, find = 0;
function OK(name) { ok++; console.log('OK', name); }
function F(sev, name, extra) { find++; console.log('FINDING', sev, name, extra !== undefined ? '-> ' + String(extra).slice(0, 220) : ''); }
function T(name, cond, extra) { if (cond) OK(name); else F('DEFECT?', name, extra); }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const dev = devices['Pixel 7'];
  const ctx = await browser.newContext({ ...dev });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const shot = async n => { await page.screenshot({ path: SHOT + n }); console.log('SHOT', n); };
  const tapGame = async (gx, gy, wait = 450) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
  const ftap = async (x, y, wait = 450) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
  const fresh = async () => {
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(500);
    await call('set_mobile', 1); await sleep(300);
  };
  const startRunByTaps = async () => {
    await tapGame(320, 108 + 9);
    if ((await call('dbg_state')) === 7) await tapGame(320, 138);
    return poll('dbg_state', v => v === 1, 6000);
  };

  // ================= Boot A: STICK =================
  await fresh();
  console.log('NOTE default touchctl =', await call('dbg_touchctl', -1), '(0=DRAG)');
  await tapGame(320, 108 + 2 * 22 + 9, 600);   // CONTROLS row toggles the scheme
  T('CONTROLS row toggles DRAG -> STICK', (await call('dbg_touchctl', -1)) === 1, await call('dbg_touchctl', -1));
  T('run starts (STICK)', (await startRunByTaps()) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  await shot('hud-stick.png');
  await call('dbg_tp', 0, 80); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(450);
  T('STICK: USE opens the mall door', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -315, -6148); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(500);
  T('STICK: USE opens the Counter panel', (await call('dbg_state')) === 2 && (await call('dbg_station')) === 0,
    'state=' + (await call('dbg_state')) + ' st=' + (await call('dbg_station')));
  await shot('store-stick.png');
  await tapGame(30, 340, 500);
  T('STICK: tap outside closes the panel', (await poll('dbg_state', v => v === 1, 3000)) === 1, await call('dbg_state'));
  // WAR! cabinet in STICK
  await call('dbg_tp', 350, -5976); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(450);
  T('STICK: USE opens the arcade door', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(500);
  T('STICK: USE opens the WAR! panel', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await shot('war-panel-stick.png');
  let launched = false;
  for (const ry of [150, 178, 206, 234, 262]) {
    await ftap(320, ry, 650);
    if ((await call('dbg_bwmode')) === 8) { launched = true; console.log('NOTE WAR! launched by tap at row y=', ry); break; }
    if ((await call('dbg_arcgate')) !== 1) break;
  }
  T('STICK: a panel-row tap launches WAR!', launched, 'bwmode=' + (await call('dbg_bwmode')));
  if (launched) { await call('dbg_bwexit'); await sleep(400); }

  // ================= Boot B: DRAG (default scheme) =================
  await fresh();
  T('scheme is DRAG by default', (await call('dbg_touchctl', -1)) === 0, await call('dbg_touchctl', -1));
  T('run starts (DRAG)', (await startRunByTaps()) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  await shot('hud-drag.png');
  await call('dbg_tp', 0, 80); await sleep(250);
  await ftap(320, 110, 600);   // tap ON the store building (tap_on_store rect)
  T('DRAG: tap on the store enters the mall', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  // at the Counter disc, tap (centre screen and on the counter itself)
  await call('dbg_tp', -315, -6148); await sleep(250);
  await ftap(320, 180, 500);
  await ftap(320, 110, 500);
  const dragStore = (await call('dbg_state')) === 2;
  console.log('NOTE DRAG: state at the Counter after taps =', await call('dbg_state'), '(2=panel open)');
  T('DRAG: a tap opens the Counter panel', dragStore, 'state=' + (await call('dbg_state')));
  // the exit mat: the trap check
  await call('dbg_tp', 0, -5810); await sleep(250);
  await ftap(320, 180, 600);
  const stillMall = (await call('dbg_mall')) === 1;
  console.log('NOTE DRAG: still in mall after tapping on the exit mat =', stillMall);
  T('DRAG: a tap on the exit mat leaves the mall', !stillMall, 'mall=' + (await call('dbg_mall')));
  // Bug Bowl under DRAG (only reachable if panels open)
  if (dragStore || true) {
    await call('dbg_tp', -180, -5868); await sleep(250);
    await ftap(320, 180, 600);
    const bowlOpen = (await call('dbg_state')) === 2 && (await call('dbg_station')) === 4;
    console.log('NOTE DRAG: bowl open =', bowlOpen, 'state=', await call('dbg_state'), 'st=', await call('dbg_station'));
    if (bowlOpen) {
      await shot('bowl-drag.png');
      const tx0 = await call('dbg_bowl_tx');
      await finger(1, 200, 300, 0); await sleep(80);
      await finger(1, 320, 300, 1); await sleep(800);
      await finger(1, 320, 300, 2); await sleep(300);
      const tx1 = await call('dbg_bowl_tx');
      console.log('NOTE DRAG bowl_tx:', tx0, '->', tx1);
      T('DRAG: dragging repositions the bowl throw', tx1 !== tx0, tx0 + '->' + tx1);
    }
  }

  console.log('AUDIT-TOUCH2 DONE: ok=' + ok + ' findings=' + find);
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
