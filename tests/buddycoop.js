// buddycoop.js — the family variant of Daniel's report: LOCAL CO-OP
// on (P2 dropped in), P1 buys a buddy at the real counter, the pair
// play in the meadow, the game saves, reload, CONTINUE — is P1's
// buddy still with him in the mall?
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = [];
const T = (name, ok, info) => { console.log((ok ? 'PASS' : 'FAIL') + ' | ' + name + (ok ? '' : ' | ' + info)); if (!ok) fails.push(name); };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    function mkpad(index) {
      const buttons = [];
      for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
      return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    }
    window.__pads = [mkpad(0), mkpad(1)];
    navigator.getGamepads = () => window.__pads;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (fn, pred, timeout = 15000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < timeout) { v = await call(fn); if (pred(v)) return v; await sleep(250); } return v; };
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

  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  // co-op ON: the co-op row sits just above the NAME field at the
  // bottom of the title menu, but its index shifts with the device
  // in use (keyboard shows a SETTINGS row, a pad hides it — and the
  // first keypress itself flips which set is shown). Walk to the
  // wrap point to find the bottom, then step back one row.
  { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;
    for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
      if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
    if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
    else { await key('ArrowUp', 160); } }
  await key('Space', 300);
  T('co-op toggled on', (await call('dbg_coop')) === 1, await call('dbg_coop'));
  for (let i = 0; i < 6; i++) await key('ArrowUp', 160);
  await key('Space', 500);
  if ((await call('dbg_state')) !== 1) await key('Enter', 500);
  T('co-op run started', (await call('dbg_state')) === 1);
  T('P2 dropped in', (await poll('dbg_p2', v => v > 0, 6000)) > 0, await call('dbg_p2'));

  // P1 buys a pet rock at the real counter
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('P1 entered the mall', (await call('dbg_mall')) === 1);
  T('walked to the buddies counter', await walkTo(225, -6108, 14));
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await key('KeyE', 350);
  T('buddies panel open', (await call('dbg_station')) === 3, await call('dbg_station'));
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350); await key('Space', 350); await key('Space', 400);
  T('pet rock adopted', (await poll('dbg_buddy', v => v !== 0, 20000)) === 5, await call('dbg_buddy'));
  await key('Escape', 300);
  T('walked to the exit mat', await walkTo(0, -5850, 10));
  await key('KeyE', 300);
  T('left the mall', (await call('dbg_mall')) === 0);
  T('buddy followed out', (await poll('dbg_budy', v => v > -1000, 5000)) > -1000);
  await call('dbg_tp', 2000, 1000); await sleep(300);
  T('buddy caught up in the meadow', Math.abs((await poll('dbg_budy', v => Math.abs(v - 1000) < 160, 30000)) - 1000) < 160);

  await sleep(7000);   // autosave
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('save carries the buddy (bd=)', / bd=\d/.test(raw), raw.slice(0, 200));

  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500);
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await key('Space', 600);
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(500);
  T('P2 reclaimed', (await call('dbg_p2')) > 0, await call('dbg_p2'));
  T('buddy still in the data', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  const py = await call('dbg_py'), by2 = await call('dbg_budy');
  console.log('after Continue: player y=' + py + ' buddy y=' + by2);
  await page.screenshot({ path: __dirname + '/buddycoop-mall.png' });
  T('BUDDY IS WITH YOU in the mall after Continue (co-op)', Math.abs(by2 - py) < 300, 'player y=' + py + ' buddy y=' + by2);
  console.log(fails.length ? 'BUDDYCOOP FAILURES: ' + fails.join(', ') : 'BUDDYCOOP ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
