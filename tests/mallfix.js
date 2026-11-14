// Mall readability fix regression: solo tour of all four stations with
// the wider counters, screenshots, and the co-op mall rules — both
// players inside together (merged view), panels splitting the screen,
// and either player inside alone.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function rig(browser) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  return page;
}
function hooks(page) {
  return (async () => {
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    const T = await page.evaluateHandle(() => window.__TSG);
    const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
    return { T, call };
  })();
}
async function poll(call, fn, want, timeout = 10000, step = 200) {
  const t0 = Date.now();
  while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
  return call(fn);
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ---------------- page 1: solo mall tour -------------------------------
  const page = await rig(browser);
  await page.goto(URL);
  const { call } = await hooks(page);
  const state = () => call('dbg_state');
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
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
  ok((await state()) === 0, 'boots to title');
  await key('Space', 400);
  if ((await state()) !== 1) { await key('Enter', 400); }
  ok((await state()) === 1, 'run started');
  await call('dbg_tp', 0, 80);
  await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  const stations = [[-225, 0, 'Bounty'], [75, 2, 'Bank'], [225, 3, 'Buddies']];
  for (const [sx, si, nm] of stations) {
    ok(await walkTo(sx, -6108, 14), `walked to the ${nm} counter`);
    await key('KeyE', 300);
    ok((await state()) === 2 && (await call('dbg_station')) === si, `${nm} panel opens`, `${await state()}/${await call('dbg_station')}`);
    await key('Escape', 300);
    ok((await state()) === 1, `${nm} panel closes`);
  }
  ok(await walkTo(-170, -5908, 14), 'walked to the Bug Bowl counter');
  await key('KeyE', 300);
  ok((await state()) === 2 && (await call('dbg_station')) === 4, 'bowling view opens');
  await key('Escape', 300);
  // screenshots: mid-room (all four counters) and up close at the north row
  await call('dbg_tp', 0, -5940);
  await sleep(700);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/mall-full.png' });
  await call('dbg_tp', 60, -6030);
  await sleep(500);
  await cv.screenshot({ path: __dirname + '/mall-north.png' });
  ok(await walkTo(0, -5850, 10), 'walked to the exit mat');
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 0, 'exit mat leaves the mall');
  await page.close();

  // ---------------- page 2: co-op, together and alone --------------------
  const page2 = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page2.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page2.addInitScript(() => {
    function mkpad(index) {
      const buttons = [];
      for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
      return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    }
    window.__pads = [mkpad(0), mkpad(1)];
    navigator.getGamepads = () => window.__pads;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  await page2.goto(URL);
  const h2 = await hooks(page2);
  const call2 = h2.call;
  const key2 = async (k, ms = 120) => { await page2.keyboard.press(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 130) => {
    await page2.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page2.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  const hold = async (pi, bi, ms) => {
    await page2.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page2.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(100);
  };
  ok((await call2('dbg_state')) === 0, 'co-op page boots to title');
  for (let i = 0; i < 5; i++) await key2('ArrowDown', 160);   // to the 2P CO-OP row (EDDIE row joined the menu)
  await key2('Space', 300);                                    // toggle co-op on
  for (let i = 0; i < 5; i++) await key2('ArrowUp', 160);      // back to START
  await key2('Space', 500);                                    // START -> slot picker (state 7)
  if ((await call2('dbg_state')) !== 1) { await key2('Enter', 500); }   // pick the slot
  ok((await call2('dbg_state')) === 1, 'co-op run started', await call2('dbg_state'));
  await call2('dbg_tp', 0, 80);                                // P1 waits at the hut
  await sleep(200);
  await btn(1, 0, 160);                                        // any pad-2 press: P2 drops in
  const p2on = await poll(call2, 'dbg_p2', v => v > 0, 6000);
  ok(p2on > 0, 'P2 dropped in', p2on);
  await btn(1, 1, 160);                                        // P2 interacts at the hut
  const split1 = await poll(call2, 'dbg_split', v => v === 1, 6000);
  ok(split1 === 1, 'P2 inside alone: screen splits (P1 still outside)', split1);
  await key2('KeyE', 300);                                     // P1 follows her in
  ok((await call2('dbg_mall')) === 1, 'P1 entered the mall too');
  const split2 = await poll(call2, 'dbg_split', v => v === 0, 6000);
  ok(split2 === 0, 'both inside together: one merged view', split2);
  // P2 strolls north to the Bank and opens her panel in her half
  await hold(1, 12, 1150);                                     // up to the counter line
  await hold(1, 15, 380);                                      // right toward the Bank (the Gallery is gone)
  let split3 = 0;
  for (let i = 0; i < 8 && split3 !== 1; i++) {
    await btn(1, 1, 140);                                      // interact
    split3 = await call2('dbg_split');
    if (split3 !== 1) { await hold(1, 15, 150); }
  }
  ok(split3 === 1, 'P2 shopping in her half while P1 browses the mall', split3);
  const cv2 = await page2.$('canvas');
  await cv2.screenshot({ path: __dirname + '/mall-split.png' });
  await btn(1, 8, 160);                                        // Back closes P2's panel
  const split4 = await poll(call2, 'dbg_split', v => v === 0, 6000);
  ok(split4 === 0, 'panel closed: merged again', split4);
  // P2 heads out alone: pin to the west wall along the counter line
  // (known x), walk the measured distance back to the carpet, then
  // straight down it to the exit mat
  await hold(1, 14, 1600);                                     // west wall: x = -284 for sure
  await hold(1, 15, 1420);                                     // ~284px east: the carpet, x ~ 0
  await hold(1, 13, 1250);                                     // down the carpet to the mat
  let p2out = 0;
  for (let i = 0; i < 8 && !p2out; i++) {
    await btn(1, 1, 140);                                      // interact
    if ((await call2('dbg_split')) === 1) {
      await btn(1, 8, 140);                                    // a panel opening also splits...
      if ((await call2('dbg_split')) === 1) p2out = 1;         // ...still split after Back = really out
    }
    if (!p2out) { await hold(1, 15, 200); await hold(1, 13, 200); }
  }
  ok(p2out === 1, 'P2 left the mall on her own (P1 still inside)');
  // P1 walks to the exit mat (he entered at the spawn, just north of it)
  { const t0 = Date.now(); let on = false;
    while (Date.now() - t0 < 8000 && !on) {
      const x = await call2('dbg_px'), y = await call2('dbg_py');
      if (Math.abs(x - 0) <= 10 && Math.abs(y - -5850) <= 10) { on = true; break; }
      const keys = [];
      if (x < -10) keys.push('ArrowRight'); else if (x > 10) keys.push('ArrowLeft');
      if (y < -5860) keys.push('ArrowDown'); else if (y > -5840) keys.push('ArrowUp');
      for (const k of keys) await page2.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page2.keyboard.up(k);
    }
    ok(on, 'P1 reached the exit mat'); }
  await key2('KeyE', 300);
  ok((await call2('dbg_mall')) === 0, 'P1 left the mall');
  const split5 = await poll(call2, 'dbg_split', v => v === 0, 6000);
  ok(split5 === 0, 'P2 had already left on her own: views merged outside', split5);

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
