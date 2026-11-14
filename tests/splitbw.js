// splitbw.js — Banzai World as a per-player mode (Nov 3): in split-screen
// co-op, P1 enters BW and plays in their half while P2 keeps playing the
// field; then P2 takes their own BW trip and leaves with pad Start.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

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
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  async function poll(fn, want, timeout = 10000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 130) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  const holdBtn = async (pi, bi, ms) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });

  // co-op on, run started, P2 in — the co-op row sits just above the
  // NAME field at the menu's bottom; its index shifts with the
  // device in use, so walk to the wrap point and step back one row
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
  await btn(1, 0, 160);
  ok((await poll('dbg_p2', v => v > 0, 6000)) > 0, 'P2 dropped in');
  const p2slot = await call('dbg_p2');
  await call('dbg_adopt', 0);
  await call('dbg_adopt2', 0);
  await sleep(400);

  // P1: into the mall, to the gate, INTERACT -> Banzai World
  await call('dbg_tp', 0, 80);
  await key('KeyE', 200); await sleep(400);
  await call('dbg_tp', 185, -5908);
  await sleep(250);
  await key('KeyE', 200);
  ok((await poll('dbg_gate', v => v === 1, 5000)) === 1, 'gate panel opened for P1');
  await key('Space', 200);
  ok((await poll('dbg_bwstate', v => v === 1, 5000)) === 1, 'P1 entered Banzai World from co-op');
  ok((await call('dbg_state')) === 1, 'the run itself never left GS_PLAY');
  ok((await poll('dbg_splitf', v => v > 50, 8000)) > 50, 'screen split for the BW trip', await call('dbg_splitf'));
  await sleep(600);
  await page.screenshot({ path: __dirname + '/splitbw.png' });

  // P1's buddy walks in BW (keyboard drives the buddy now)
  const bx0 = await call('dbg_bwx');
  await page.keyboard.down('KeyD'); await sleep(750); await page.keyboard.up('KeyD');
  const bx1 = await call('dbg_bwx');
  ok(bx1 > bx0 + 15, 'P1 buddy roams Banzai World', bx0 + ' -> ' + bx1);

  // P2 still plays the field meanwhile (pad dpad right)
  const p2x0 = await call('dbg_p2x');
  await holdBtn(1, 15, 750);
  const p2x1 = await call('dbg_p2x');
  ok(p2x1 > p2x0 + 15, 'P2 keeps playing the field during P1s BW trip', p2x0 + ' -> ' + p2x1);
  ok((await call('dbg_bwstate2')) === 0, 'P2 not dragged into BW');

  // P1 leaves with Esc; DEV is back at the gate in the mall
  await key('Escape', 200);
  ok((await poll('dbg_bwstate', v => v === 0, 5000)) === 0, 'P1 Esc leaves Banzai World');
  ok(Math.abs((await call('dbg_px')) - 185) < 3 && Math.abs((await call('dbg_py')) - -5904) < 4,
     'P1 DEV back at the mall gate', (await call('dbg_px')) + ',' + (await call('dbg_py')));

  // P2's own trip: mall via pad interact at the storefront, gate, B
  await call('dbg_tp2', 0, 80);
  await btn(1, 1, 180); await sleep(400);
  await call('dbg_tp2', 185, -5908);
  await sleep(250);
  await btn(1, 1, 180);
  await sleep(350);
  await btn(1, 0, 180);   // pad A picks the panel's first row
  ok((await poll('dbg_bwstate2', v => v === 1, 5000)) === 1, 'P2 entered Banzai World via the gate panel');
  const b2x0 = await call('dbg_bwx2');
  await holdBtn(1, 15, 750);
  const b2x1 = await call('dbg_bwx2');
  ok(b2x1 > b2x0 + 15, 'P2 buddy roams on pad input', b2x0 + ' -> ' + b2x1);
  // P2 leaves with pad Start — and the run must NOT pause
  await btn(1, 9, 180);
  ok((await poll('dbg_bwstate2', v => v === 0, 5000)) === 0, 'P2 Start leaves Banzai World');
  ok((await call('dbg_state')) === 1, 'run still playing (Start did not pause)', await call('dbg_state'));

  console.log('\nsplitbw: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
