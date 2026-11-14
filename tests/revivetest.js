// Co-op death, Daniel's rules: the split vanishes the moment a
// player goes down, a red arrow marks the body, and the standing
// player buys them back with carcasses — priced off the DOWNED
// player's capability (a fresh P2 costs 30 + 58/4 + 100/8 = 56 KG).
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

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT, 2);
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 140) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  async function poll(fn, args, want, timeout = 10000, step = 150) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }
  async function walkTo(tx, ty, tol = 12, timeout = 25000) {
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

  // the co-op row sits just above the NAME field at the menu's
  // bottom; its index shifts with the device in use (a keyboard
  // shows a SETTINGS row, a pad hides it), so walk to the wrap
  // point and step back one row
  { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;
    for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
      if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
    if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
    else { await key('ArrowUp', 160); } }
  await key('Space', 300);
  for (let i = 0; i < 6; i++) await key('ArrowUp', 160);
  await key('Space', 500);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 500); }
  ok((await call('dbg_state')) === 1, 'co-op run started');
  await btn(1, 0, 150);
  ok((await poll('dbg_p2', [], v => v > 0, 6000)) > 0, 'P2 dropped in');

  // pull them far apart so the screen is fully split
  await call('dbg_tp', -900, 0);
  ok((await poll('dbg_splitf', [], v => v >= 99, 8000)) >= 99, 'screen fully split while apart');

  // P2 goes down: the split must vanish at once, not ease away
  await call('dbg_kill2');
  ok((await poll('dbg_p2hp', [], v => v <= 0, 6000)) <= 0, 'P2 is down');
  // (polled briefly: the death-zero lands on the next rendered
  // frame, and a single instantaneous sample races it — but the
  // ease-out it must NOT do would take 600ms+, so 400ms separates
  // "vanished on death" from "eased away")
  const sf = await poll('dbg_splitf', [], v => v <= 2, 400, 50);
  ok(sf <= 2, 'split screen gone right away', sf);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/revive-arrow.png' });

  // broke rescuer first: no hat, no revive
  await walkTo(await call('dbg_p2x'), await call('dbg_p2y'), 14);
  await key('KeyE', 200);
  await sleep(500);
  ok((await call('dbg_p2hp')) <= 0, 'no carcasses, no revive');

  // load P1 with 60 KG and buy P2 back (fresh P2 costs 56 KG)
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  const before = await call('dbg_carried');
  await key('KeyE', 200);
  const hp = await poll('dbg_p2hp', [], v => v > 0, 6000);
  ok(hp === 50, 'P2 revived at half health', hp);
  const after = await call('dbg_carried');
  ok(before - after >= 540 && before - after <= 580, 'revive cost ~56 KG off the rescuer', `${before} -> ${after}`);

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
