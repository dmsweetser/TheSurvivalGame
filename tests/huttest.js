// The Bug Mall storefront is solid: players stop at its faces, and a
// bug shoved inside the footprint gets pushed back out.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
const ok = (c, m, x) => { if (c) { pass++; console.log('PASS', m); } else { fail++; console.log('FAIL', m, x !== undefined ? ' [' + x + ']' : ''); } };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (f, ...a) => page.evaluate(([f, a]) => window.__TSG[f](...a), [f, a]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await key('Enter', 600);
  ok((await call('dbg_state')) === 1, 'run started', await call('dbg_state'));

  // from the north: walk south into the hut's back wall
  await call('dbg_tp', 0, -220);
  await page.keyboard.down('ArrowDown'); await sleep(1600); await page.keyboard.up('ArrowDown');
  const pyN = await call('dbg_py');
  ok(pyN <= -124 && pyN >= -132, 'player stops at the hut north face', pyN);

  // from the plaza: walk north into the storefront (counter side)
  await call('dbg_tp', 0, 60);
  await page.keyboard.down('ArrowUp'); await sleep(1600); await page.keyboard.up('ArrowUp');
  const pyS = await call('dbg_py');
  ok(pyS >= -6 && pyS <= 2, 'player stops at the storefront south face', pyS);

  // from the west: walk east into the hut's side
  await call('dbg_tp', -220, -60);
  await page.keyboard.down('ArrowRight'); await sleep(1600); await page.keyboard.up('ArrowRight');
  const pxW = await call('dbg_px');
  ok(pxW <= -100 && pxW >= -108, 'player stops at the hut west face', pxW);

  // a bug teleported inside the footprint is pushed out, not left in
  await call('dbg_tp', 0, 300);                       // player waits south, out of the way
  await call('dbg_spawnbug', 0, -60);
  await sleep(700);
  const nb = await call('dbg_bugs');
  let inside = 0, near = 0;
  for (let i = 0; i < nb; i++) {
    const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i);
    if (bx > -95 && bx < 95 && by > -119 && by < -11) inside++;
    if (Math.abs(bx) < 200 && by > -260 && by < 60) near++;
  }
  ok(inside === 0, 'no bug left inside the storefront footprint', inside);
  ok(near > 0, 'the spawned bug is alive at the hut edge', near);

  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
