// Bowling the way the family actually plays it: gamepad only.
// Page 1: solo, one pad — start the game, AIM with the d-pad (this is
// the path that must move the thrower), and throw a full ball.
// Page 2: co-op — P2 bowls an entire ball on pad 2 in her split half.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
const PADSCRIPT = npads => {
  function mkpad(index) {
    const buttons = [];
    for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
  }
  window.__pads = [];
  for (let i = 0; i < npads; i++) window.__pads.push(mkpad(i));
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
};

async function rig(browser, npads) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT, npads);
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 130) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  const hold = async (pi, bi, ms) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(100);
  };
  async function poll(fn, args, want, timeout = 10000, step = 150) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
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
  }
  return { page, call, key, btn, hold, poll, walkTo };
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ---------------- page 1: solo, pad only at the lanes -------------------
  {
    const { page, call, key, btn, hold, poll, walkTo } = await rig(browser, 1);
    await key('Space', 400);
    if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
    ok((await call('dbg_state')) === 1, 'solo run started');
    await call('dbg_feed'); await call('dbg_feed');
    await call('dbg_tp', 0, 80); await sleep(150);
    await key('KeyE', 300);
    ok((await call('dbg_mall')) === 1, 'entered the mall');
    ok(await walkTo(-180, -5868, 14), 'walked to the bowl counter');
    await key('KeyE', 300);
    ok((await call('dbg_station')) === 4, 'bowling view open');
    ok((await call('dbg_bowl_ph', 0)) === -1, 'lanes at attract (no game yet)');
    await btn(0, 0, 150);                                    // pad A: new game
    ok((await poll('dbg_bowl_ph', [0], v => v === 0)) === 0, 'pad A starts the game (positioning)');
    await hold(0, 14, 900);                                  // d-pad LEFT
    const txL = await call('dbg_bowl_tx', 0);
    ok(txL <= -40, 'd-pad left moves the thrower left', txL);
    await hold(0, 15, 900);                                  // d-pad RIGHT
    const txR = await call('dbg_bowl_tx', 0);
    ok(txR >= 30, 'd-pad right moves the thrower right', txR);
    await hold(0, 14, 350);                                  // roughly recenter
    const carried0 = await call('dbg_carried');
    await btn(0, 0, 150);                                    // ready -> angle sweep
    ok((await call('dbg_bowl_ph', 0)) === 1, 'pad A starts the aim sweep');
    await btn(0, 0, 150); await sleep(250);                  // lock angle
    ok((await call('dbg_bowl_ph', 0)) === 2, 'pad A locks the angle (power next)');
    await btn(0, 0, 150);                                    // throw
    const carried1 = await poll('dbg_carried', [], v => v < carried0, 6000);
    ok(carried1 === carried0 - 20, 'pad A throws: one carcass spent', `${carried0} -> ${carried1}`);
    const fr = await poll('dbg_bowl_frame', [], v => v >= 1, 12000);
    ok(fr >= 1, 'the ball completes and the frame advances', fr);
    await page.close();
  }

  // ---------------- page 2: co-op, P2 bowls on pad 2 ----------------------
  {
    const { page, call, key, btn, hold, poll } = await rig(browser, 2);
    { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;   // co-op row: just above the NAME field; its index shifts with the device in use
      for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
        if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
      if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
      else { await key('ArrowUp', 160); } }
    await key('Space', 300);
    // back to START by the LIVE selection: a fixed count of Ups wraps
    // past row 0 onto the NAME field on the pad-flavoured menu
    for (let i = 0; i < 10 && (await call('dbg_menusel')) !== 0; i++) await key('ArrowUp', 160);
    await key('Space', 500);
    if ((await call('dbg_state')) !== 1) { await key('Enter', 500); }
    ok((await call('dbg_state')) === 1, 'co-op run started');
    await call('dbg_tp', 0, 80); await sleep(150);
    await btn(1, 0, 150);                                    // P2 drops in
    ok((await poll('dbg_p2', [], v => v > 0, 6000)) > 0, 'P2 dropped in');
    await call('dbg_give2'); await call('dbg_give2');        // P2 needs a hat to bowl with
    await btn(1, 1, 150);                                    // P2 enters the mall (B at the hut)
    ok((await poll('dbg_split', [], v => v === 1, 6000)) === 1, 'P2 in the mall (split)');
    // walk P2 to the bowl counter's north face, polling her real
    // position (the counter is against the south wall; its front
    // faces north into the room, so she must come in from y ~ -5910)
    {
      const t0 = Date.now();
      const press = async (bi, on) => page.evaluate(([p, b, o]) => window.__btn(p, b, o), [1, bi, on]);
      while (Date.now() - t0 < 25000) {
        const x = await call('dbg_p2x'), y = await call('dbg_p2y');
        if (Math.abs(x + 170) <= 10 && Math.abs(y + 5910) <= 10) break;
        const want = [];
        if (x < -180) want.push(15); else if (x > -160) want.push(14);
        if (y < -5920) want.push(13); else if (y > -5900) want.push(12);
        for (let bIdx = 12; bIdx <= 15; bIdx++) await press(bIdx, want.includes(bIdx));
        await sleep(90);
      }
      for (let bIdx = 12; bIdx <= 15; bIdx++) await page.evaluate(b => window.__btn(1, b, false), bIdx);
    }
    await btn(1, 1, 150);                                    // P2 opens the lanes
    await sleep(600);
    ok((await call('dbg_store2')) === 1, 'P2 lanes view is open');
    ok((await call('dbg_bowl_ph', 1)) === -1, 'P2 lanes open at attract');
    await btn(1, 0, 150);                                    // P2 pad A: new game
    const ph = await poll('dbg_bowl_ph', [1], v => v === 0, 8000);
    ok(ph === 0, 'P2 starts bowling from her pad', ph);
    await hold(1, 14, 700);
    const tx2 = await call('dbg_bowl_tx', 1);
    ok(tx2 <= -30, 'P2 aims with her d-pad', tx2);
    await hold(1, 15, 350);
    await btn(1, 0, 150);                                    // ready -> angle sweep
    await btn(1, 0, 150); await sleep(250);                  // lock angle
    await btn(1, 0, 150);                                    // throw
    const ph2 = await poll('dbg_bowl_ph', [1], v => v === 0, 14000);
    ok(ph2 === 0, 'P2 completes a full ball from her pad', ph2);
    const cv = await page.$('canvas');
    await cv.screenshot({ path: __dirname + '/bowl-coop.png' });
    await page.close();
  }

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
