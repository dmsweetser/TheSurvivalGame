// touchaudit.js — touch UNIFICATION across Banzai World (Daniel, Nov 5).
// His rule: pick STICK or DRAG once and it works the same in every
// minigame, clearly. Before the unified fill (bw_fill_p1), the touch
// FIRE button produced no `sel` at all in BW — no stick-throwing in
// Entrances, no swinging in Buddy Craft — menu navigation ignored the
// stick, DRAG steered nothing, and no minigame even DREW the buttons.
// This suite drives the real touch path (touch_point exports, the
// same calls the shell makes for real fingers) and checks:
//   STICK: joystick moves the buddy in Craft; FIRE swings (atk_cd);
//          WALL opens the craft menu; a stick flick walks the menu;
//          menu rows take taps (pick, tap again to craft, outside
//          tap closes); FIRE throws a stick in Entrances; FIRE jumps
//          in Super Buddy Run.
//   DRAG:  the finger steers in Craft and the hold swings; a tap is
//          the interact button; drag runs + jumps in the platformer.
//   THE BACKFIRE BEAN: key 6 far from home -> the ride plays
//          (bean_t), you land on the plaza (0,70), the bean is spent;
//          at home (inside the epicenter) the bean is spent as the
//          JETPACK instead (Nov 10; Daniel confirmed Nov 18): free
//          flight until you FIRE or take one hit.
//   WHALES ONLY: with the selection on the last row, the scrolled
//          window's tap math still lands on ENTER UNLOCK CODE.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };
  // real-finger drivers (game coords 640x360)
  const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
  const stickHold = async (dx, dy, ms) => {   // joystick from the left side, displaced by dx,dy
    await finger(1, 100, 180, 0); await sleep(60);
    await finger(1, 100 + dx, 180 + dy, 1); await sleep(ms);
    await finger(1, 100 + dx, 180 + dy, 2); await sleep(120);
  };
  const btnHold = async (x, y, ms) => { await finger(2, x, y, 0); await sleep(ms); await finger(2, x, y, 2); await sleep(120); };
  const dragHold = async (x, y, ms) => { await finger(3, x, y, 0); await sleep(ms); await finger(3, x, y, 2); await sleep(120); };
  const walkTo = async (tx, ty, tol) => {
    tol = tol || 12;
    for (let i = 0; i < 60; i++) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  };

  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await call('set_mobile', 1);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('game started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  // ================= STICK in BUDDY CRAFT =================
  await call('dbg_touchctl', 1);
  T('scheme is STICK', (await call('dbg_touchctl', -1)) === 1);
  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  await sleep(400);
  { const x0 = await call('dbg_bwx');
    await stickHold(42, 0, 700);
    const x1 = await call('dbg_bwx');
    T('STICK: joystick walks the buddy right', x1 > x0 + 10, x0 + ' -> ' + x1); }
  { // FIRE (582,278) held: swings repeat at the weapon's pace
    let swung = 0;
    await finger(2, 582, 278, 0);
    const t0 = Date.now();
    while (Date.now() - t0 < 2500) { if ((await call('dbg_batkcd')) > 0) { swung = 1; break; } await sleep(90); }
    await finger(2, 582, 278, 2);
    T('STICK: FIRE swings in Buddy Craft', swung === 1); }
  // WALL (582,198) opens the craft menu
  await btnHold(582, 198, 250);
  T('STICK: WALL opens the craft menu', (await poll('dbg_craftmenu', v => v === 1, 3000)) === 1, await call('dbg_craftmenu'));
  { const s0 = await call('dbg_craftsel');
    await stickHold(0, 40, 260);   // a flick down, released before the repeat kicks in
    const s1 = await call('dbg_craftsel');
    T('STICK: a flick walks the craft menu', s1 === s0 + 1, s0 + ' -> ' + s1); }
  { // taps on the ONE segmented list: a segment header hops to its
    // first row; tap a row to pick it, tap it again to craft it
    // (WOODEN AXE, needs 3 wood)
    await call('dbg_craftsetm', 60 + 60 * 1000 + 60 * 1000000);
    const gear0 = await call('dbg_craftgear');
    // menu panel: w=380 centred (x=130); since the Nov 14 search row
    // the panel is h=40+7*20+16+24+22=242, y=59, with a search strip
    // at [y+28,y+50) and list rows at y+52 + r*20: TOOLS header
    // ry=111, AXE ry=131, PICK ry=151
    await call('evt_tap', 320, 116); await sleep(300);
    T('tap on a segment header hops to its first row', (await call('dbg_craftsel')) === 1, await call('dbg_craftsel'));
    await call('evt_tap', 320, 156); await sleep(300);
    T('tap selects a row', (await call('dbg_craftsel')) === 2, await call('dbg_craftsel'));
    await call('evt_tap', 320, 136); await sleep(300);
    T('tap selects the axe row', (await call('dbg_craftsel')) === 1, await call('dbg_craftsel'));
    await call('evt_tap', 320, 136); await sleep(400);
    T('tapping the selected row crafts it', ((await call('dbg_craftgear')) & 1) === 1 && ((gear0 & 1) === 0), await call('dbg_craftgear'));
    await call('evt_tap', 20, 20); await sleep(300);   // outside the panel
    T('tapping outside closes the menu', (await call('dbg_craftmenu')) === 0, await call('dbg_craftmenu'));
  }

  // ================= DRAG in BUDDY CRAFT =================
  await call('dbg_touchctl', 0);
  T('scheme is DRAG', (await call('dbg_touchctl', -1)) === 0);
  { const x0 = await call('dbg_bwx');
    let swung = 0;
    await finger(3, 430, 180, 0);   // finger right of the buddy (screen centre 320,180)
    const t0 = Date.now();
    while (Date.now() - t0 < 1600) { if ((await call('dbg_batkcd')) > 0) swung = 1; await sleep(90); }
    const x1 = await call('dbg_bwx');
    await finger(3, 430, 180, 2);
    T('DRAG: the finger steers the buddy right', x1 > x0 + 10, x0 + ' -> ' + x1);
    T('DRAG: the hold swings while you steer', swung === 1); }

  // ================= STICK in ENTRANCES: FIRE throws =================
  await call('dbg_bwexit'); await sleep(300);
  await call('dbg_touchctl', 1);
  await call('dbg_bwentm', 2);
  T('entered ENTRANCES', (await poll('dbg_bwmode', v => v === 2)) === 2, await call('dbg_bwmode'));
  await sleep(500);
  await call('dbg_givestick');
  { const st0 = await call('dbg_entsticks');
    let threw = 0;
    await finger(2, 582, 278, 0);
    const t0 = Date.now();
    while (Date.now() - t0 < 2500) { if ((await call('dbg_entsticks')) < st0 || (await call('dbg_batkcd')) > 0) { threw = 1; break; } await sleep(90); }
    await finger(2, 582, 278, 2);
    T('STICK: FIRE throws a stick in Entrances', threw === 1, 'sticks ' + st0 + ' -> ' + (await call('dbg_entsticks'))); }

  // ================= SUPER BUDDY RUN: jumps =================
  await call('dbg_bwexit'); await sleep(300);
  await call('dbg_bwentm', 3);
  T('entered SUPER BUDDY RUN', (await poll('dbg_bwmode', v => v === 3)) === 3, await call('dbg_bwmode'));
  await sleep(600);
  { const y0 = await call('dbg_bwy');
    let ymin = y0;
    await finger(2, 582, 278, 0);   // FIRE = JUMP
    const t0 = Date.now();
    while (Date.now() - t0 < 1600) { const y = await call('dbg_bwy'); if (y < ymin) ymin = y; await sleep(60); }
    await finger(2, 582, 278, 2);
    T('STICK: FIRE jumps in Super Buddy Run', ymin < y0 - 12, y0 + ' -> min ' + ymin); }
  await call('dbg_touchctl', 0);
  { const x0 = await call('dbg_bwx');
    await dragHold(450, 110, 800);   // right and HIGH: run + jump
    const x1 = await call('dbg_bwx');
    T('DRAG: drag runs the platformer', x1 > x0 + 10, x0 + ' -> ' + x1); }

  // ================= THE BACKFIRE BEAN =================
  await call('dbg_bwexit'); await sleep(300);
  T('walked to the mall exit mat', await walkTo(0, -5810, 10));
  await key('KeyE', 300); await sleep(300);
  T('left the mall (the bean only flies outdoors)', (await call('dbg_mall')) === 0, await call('dbg_mall'));
  await call('dbg_tp', 3000, 3000); await sleep(300);
  await call('dbg_beans', 1);
  await key('Digit6', 120, 150);
  T('the bean ride starts', (await poll('dbg_beant', v => v > 0, 3000)) > 0, await call('dbg_beant'));
  await poll('dbg_beant', v => v === 0, 5000);
  const px = await call('dbg_px'), py = await call('dbg_py');
  T('the bean lands you on the plaza', Math.abs(px) < 30 && Math.abs(py - 70) < 30, px + ',' + py);
  T('the bean is spent', (await call('dbg_beans', -1)) === 0, await call('dbg_beans', -1));
  // at home = inside the epicenter: the bean is the JETPACK (Nov 10,
  // Daniel confirmed Nov 18) — spent on launch, free flight, and FIRE
  // drops you where you are. The pre-Nov-10 "refuses at home" rule this
  // spot used to assert was superseded by that directive; the checks
  // below mirror beanjettest's, at the same strength.
  await call('dbg_beans', 1);
  await key('Digit6', 120, 400);
  T('at home the bean is spent', (await call('dbg_beans', -1)) === 0, await call('dbg_beans', -1));
  T('at home the bean launches the jetpack', (await poll('dbg_jet', v => v === 1, 3000)) === 1, await call('dbg_jet'));
  { const hx = await call('dbg_px');
    await page.keyboard.down('ArrowRight'); await sleep(2000); await page.keyboard.up('ArrowRight'); await sleep(150);
    const jx = await call('dbg_px');
    T('the jetpack flies (400+ px east in 2 s)', jx - hx > 400, hx + ' -> ' + jx);
    await key('Space', 120, 300);
    T('firing drops the jetpack where you are', (await poll('dbg_jet', v => v !== 1, 3000)) !== 1, await call('dbg_jet')); }

  // ================= WHALES ONLY: scrolled taps =================
  { await call('dbg_tp', 0, 80); await sleep(200);
    await key('KeyE', 300); await sleep(300);              // into the mall
    T('in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));
    T('walked to the Whales Only counter', await walkTo(-105, -6148, 14));
    await key('KeyE', 350); await sleep(400);
    T('Whales Only panel open', (await call('dbg_station')) === 1, await call('dbg_station'));
    if ((await call('dbg_station')) === 1) {
      await key('ArrowUp', 120, 250);   // wraps to the last row: ENTER UNLOCK CODE
      T('selection on the last row', (await call('dbg_storesel')) === 15, await call('dbg_storesel'));
      await sleep(300);
      // scrolled window: rows 4..15 visible; row 15 sits at MALL_ROW0 + 11*18 = 278
      await call('evt_tap', 320, 283); await sleep(300);   // tap the selected row -> activate
      T('scrolled tap lands on ENTER UNLOCK CODE', (await call('text_mode')) === 3, await call('text_mode'));
      await key('Escape', 200, 300);
      await key('Escape', 200, 300);
    }
  }

  console.log('TOUCHAUDIT DONE pass=' + pass + ' fail=' + fail);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERROR', e.message); process.exit(2); });
