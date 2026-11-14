// Directional, gradual split-screen: the seam follows the direction
// and size of the separation, opens gradually, and each view carries
// the HUD that fits it.
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
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  async function poll(fn, want, timeout = 12000, step = 200) {
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
  const cv = await page.$('canvas');

  // co-op on, run started, P2 in — both at the plaza, merged
  { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;   // co-op row: just above the NAME field; its index shifts with the device in use
    for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
      if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
    if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
    else { await key('ArrowUp', 160); } }
  await key('Space', 300);
  for (let i = 0; i < 6; i++) await key('ArrowUp', 160);   // back to START
  await key('Space', 500);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 500); }
  ok((await call('dbg_state')) === 1, 'co-op run started', await call('dbg_state'));
  await btn(1, 0, 160);
  ok((await poll('dbg_p2', v => v > 0, 6000)) > 0, 'P2 dropped in');
  await sleep(1200);
  ok((await call('dbg_split')) === 0 && (await call('dbg_splitf')) <= 2,
     'together at the plaza: one shared view', await call('dbg_splitf'));

  // P1 teleports far NORTH: separation is vertical-dominant -> the
  // screen should stack (horizontal seam), opening gradually
  await call('dbg_tp', 0, -1500);
  const fEarly = await poll('dbg_splitf', v => v > 3 && v < 92, 5000, 60);
  ok(fEarly > 3 && fEarly < 92, 'split opens gradually, not in one jump', fEarly);
  const fFull = await poll('dbg_splitf', v => v >= 97, 9000);
  ok(fFull >= 97, 'split settles fully open when far apart', fFull);
  ok((await call('dbg_orient')) === 0, 'north-south apartness: stacked views', await call('dbg_orient'));
  await sleep(600);
  await cv.screenshot({ path: __dirname + '/split-stacked.png' });

  // P1 teleports far EAST instead: east-west apartness -> side by side
  await call('dbg_tp', 1500, 0);
  const o2 = await poll('dbg_orient', v => v === 1, 9000);
  ok(o2 === 1, 'east-west apartness: side-by-side views', o2);
  await sleep(600);
  await cv.screenshot({ path: __dirname + '/split-side.png' });

  // P1 returns: the seam sweeps back out and the views merge
  await call('dbg_tp', 0, 60);
  const fBack = await poll('dbg_splitf', v => v <= 2, 12000);
  ok(fBack <= 2 && (await call('dbg_split')) === 0, 'coming back together merges the view', fBack);
  await cv.screenshot({ path: __dirname + '/split-merged.png' });

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
