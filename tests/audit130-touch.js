// audit130-touch.js — v1.30 audit, touch phase: STICK boot then DRAG
// boot, on a Pixel 7 context. Menu taps use the real touchscreen; in-
// game fingers use the touch_point seam (the exact calls the shell
// makes for real fingers — the touchaudit precedent). dbg_* observes.
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
  const tapGame = async (gx, gy, wait = 400) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
  const fresh = async () => {
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(500);
  };
  const startRunByTaps = async () => {
    const n = await call('dbg_menucount');
    await tapGame(320, 108 + 9);                       // START row (row 0)
    if ((await call('dbg_state')) === 7) await tapGame(320, 138);   // slot card 1
    return poll('dbg_state', v => v === 1, 6000);
  };

  // ================= STICK =================
  await fresh();
  await call('set_mobile', 1);
  await sleep(400);
  T('touch device reads TOUCH', (await poll('dbg_dev', v => v === 2, 3000)) === 2, await call('dbg_dev'));
  console.log('NOTE menucount(touch)=', await call('dbg_menucount'), 'touchctl=', await call('dbg_touchctl', -1));
  await shot('title-touch.png');
  T('run starts by taps (STICK)', (await startRunByTaps()) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  await shot('hud-stick.png');
  // joystick: finger down left side, displace right
  { const x0 = await call('dbg_px');
    await finger(1, 100, 180, 0); await sleep(80);
    await finger(1, 145, 180, 1); await sleep(900);
    await finger(1, 145, 180, 2); await sleep(200);
    T('STICK joystick walks east', (await call('dbg_px')) > x0 + 15, x0 + '->' + (await call('dbg_px'))); }
  // USE deck button at the mall door
  await call('dbg_tp', 0, 80); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(400);
  T('USE button opens the mall door', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  // Counter panel by USE, close by tapping outside
  await call('dbg_tp', -315, -6148); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(500);
  const storeOpen = (await call('dbg_state')) === 2 || (await call('dbg_store1')) === 1;
  T('USE opens the Counter panel', storeOpen, 'state=' + (await call('dbg_state')));
  await shot('store-touch.png');
  await tapGame(30, 340, 500);   // far corner, outside the panel
  const storeClosed = (await call('dbg_state')) === 1 && (await call('dbg_store1')) === 0;
  T('tap outside closes the Counter panel', storeClosed, 'state=' + (await call('dbg_state')) + ' store1=' + (await call('dbg_store1')));
  // pause via the shell ESC button
  await page.evaluate(() => { const b = document.getElementById('escbtn'); if (b) { b.style.display = 'block'; } });
  const escVisible = await page.evaluate(() => { const b = document.getElementById('escbtn'); return b ? getComputedStyle(b).display : 'missing'; });
  console.log('NOTE escbtn display =', escVisible);
  await page.tap('#escbtn').catch(e => console.log('NOTE escbtn tap failed:', String(e).slice(0, 120)));
  await sleep(600);
  const pst = await call('dbg_state');
  console.log('NOTE state after ESC-button tap =', pst, '(5=pause)');
  if (pst === 5) {
    await shot('pause-touch.png');
    await tapGame(320, 180, 600);   // CONTINUE row (y=176 text, bar 172..191)
    T('tap CONTINUE resumes', (await poll('dbg_state', v => v === 1, 4000)) === 1, await call('dbg_state'));
  } else F('DEFECT?', 'shell ESC button did not pause the game', 'state=' + pst);
  // WAR! cabinet by taps: USE opens panel, tap the SKIRMISH row
  await call('dbg_tp', 350, -5976); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(400);
  T('USE opens the arcade door', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(250);
  await finger(2, 514, 318, 0); await sleep(200); await finger(2, 514, 318, 2); await sleep(500);
  T('USE opens the WAR! panel', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await shot('war-panel-touch.png');
  // panel rows: tap each candidate band until the mode launches (rows drawn ~y 150-260)
  let launched = -1;
  for (const ry of [168, 196, 224, 252]) {
    await tapGame(320, ry, 700);
    const m = await call('dbg_bwmode');
    if (m === 8) { launched = ry; break; }
    if ((await call('dbg_arcgate')) !== 1) break;
  }
  T('a tap on a WAR! panel row launches the mode', launched > 0, 'bwmode=' + (await call('dbg_bwmode')));
  await sleep(800);
  await shot('war-map-touch.png');
  await page.tap('#escbtn').catch(() => {});
  await sleep(600);
  console.log('NOTE after ESC btn in WAR!: bwmode=', await call('dbg_bwmode'), 'warpause=', await call('dbg_warpause'));
  if ((await call('dbg_bwmode')) === 8) { await call('dbg_bwexit'); await sleep(400); }

  // ================= DRAG =================
  await fresh();
  await call('set_mobile', 1);
  await sleep(400);
  // switch scheme on the title CONTROLS row (row 2 in touch flavour)
  await tapGame(320, 108 + 2 * 22 + 9, 600);
  T('title CONTROLS row switches the scheme to DRAG', (await call('dbg_touchctl', -1)) === 0, await call('dbg_touchctl', -1));
  await shot('title-drag.png');
  T('run starts by taps (DRAG)', (await startRunByTaps()) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  await shot('hud-drag.png');
  // drag steer: finger down near the player (centre), drag right, hold
  { const x0 = await call('dbg_px');
    await finger(1, 320, 180, 0); await sleep(80);
    await finger(1, 380, 180, 1); await sleep(1000);
    await finger(1, 380, 180, 2); await sleep(200);
    T('DRAG finger steers east', (await call('dbg_px')) > x0 + 15, x0 + '->' + (await call('dbg_px'))); }
  // tap = interact at the mall door
  await call('dbg_tp', 0, 80); await sleep(250);
  await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(500);
  T('DRAG tap opens the mall door (interact)', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  // BUG BOWL under DRAG: open, then drag sideways — does the throw position move?
  await call('dbg_tp', -180, -5868); await sleep(250);
  await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(600);
  const bowlOpen = (await call('dbg_state')) === 2 && (await call('dbg_station')) === 4;
  T('DRAG tap opens the Bug Bowl', bowlOpen, 'state=' + (await call('dbg_state')) + ' st=' + (await call('dbg_station')));
  if (bowlOpen) {
    await shot('bowl-drag.png');
    const tx0 = await call('dbg_bowl_tx');
    await finger(1, 200, 300, 0); await sleep(80);
    await finger(1, 300, 300, 1); await sleep(700);
    await finger(1, 300, 300, 2); await sleep(300);
    const tx1 = await call('dbg_bowl_tx');
    console.log('NOTE bowl_tx under DRAG drag:', tx0, '->', tx1);
    T('DRAG finger repositions the bowl throw', tx1 !== tx0, tx0 + '->' + tx1);
    await page.tap('#escbtn').catch(() => {}); await sleep(500);
    if ((await call('dbg_state')) === 2) { await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(400); }
  }
  // TANK under DRAG: drive, then try every fire candidate
  await call('dbg_tp', 350, -5976); await sleep(250);
  await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(500);
  await call('dbg_tp', -256, -11898); await sleep(250);
  await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(500);
  T('DRAG tap opens the Tank cabinet panel', (await call('dbg_arcgate')) === 1, await call('dbg_arcgate'));
  await shot('tank-panel-drag.png');
  await tapGame(320, 180, 800);   // panel's launch row (tap to play)
  const tm = await poll('dbg_bwmode', v => v === 5, 5000);
  T('Tank launches from a panel tap', tm === 5, await call('dbg_bwmode'));
  if (tm === 5) {
    const ty0 = await call('dbg_tanky');
    await finger(1, 320, 120, 0); await sleep(80);           // drag north
    await finger(1, 320, 60, 1); await sleep(1100);
    const tyHold = await call('dbg_tanky');
    const shellsHold = await call('dbg_tankshells');
    await finger(1, 320, 60, 2); await sleep(200);
    console.log('NOTE tank under DRAG: y', ty0, '->', tyHold, 'shells during drag-hold =', shellsHold);
    T('DRAG steers the tank', tyHold < ty0 - 20, ty0 + '->' + tyHold);
    F('EVIDENCE', 'DRAG tank fire check: shells while drag-holding = ' + shellsHold + ' (0 = cannon cannot fire under DRAG)', '');
    await page.tap('#escbtn').catch(() => {}); await sleep(600);
    if ((await call('dbg_bwmode')) === 5) { await call('dbg_bwexit'); await sleep(400); }
  }
  // CART under DRAG: waiting room — tap first, then drag-hold
  await call('dbg_tp', 256, -11898); await sleep(250);
  await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(500);
  await tapGame(320, 180, 800);
  const cm = await poll('dbg_bwmode', v => v === 7, 5000);
  T('Cart launches from a panel tap', cm === 7, await call('dbg_bwmode'));
  if (cm === 7) {
    await sleep(600);
    await shot('cart-wait-drag.png');
    const st0 = await call('dbg_crtstate');
    await finger(1, 320, 180, 0); await sleep(120); await finger(1, 320, 180, 2); await sleep(600);   // a tap
    const stTap = await call('dbg_crtstate');
    await finger(1, 320, 180, 0); await sleep(1500);                                                    // a hold
    const stHold = await call('dbg_crtstate');
    const prog = await call('dbg_crtprog');
    await finger(1, 320, 180, 2); await sleep(200);
    console.log('NOTE cart waiting room under DRAG: state tap:', st0, '->', stTap, '; after hold:', stHold, 'prog=', prog);
    await page.tap('#escbtn').catch(() => {}); await sleep(600);
    if ((await call('dbg_bwmode')) === 7) { await call('dbg_bwexit'); await sleep(400); }
  }

  console.log('AUDIT-TOUCH DONE: ok=' + ok + ' findings=' + find);
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
