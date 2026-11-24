// audit130-probe2.js — DRAG inside the mall: can taps open a station?
// Can taps leave by the exit mat? (Pre-fix evidence for the v1.30 audit.)
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const ctx = await browser.newContext({ ...devices['Pixel 7'] });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
  const ftap = async (x, y, wait = 500) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
  const tapGame = async (gx, gy, wait = 450) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(500);
  await call('set_mobile', 1); await sleep(300);
  await tapGame(320, 117);
  if ((await call('dbg_state')) === 7) await tapGame(320, 138);
  await sleep(500);
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(250);
  await ftap(320, 30, 600);
  console.log('in mall =', await call('dbg_mall'));
  // Counter disc: taps at centre and at the counter (north on screen)
  await call('dbg_tp', -315, -6148); await sleep(250);
  await page.screenshot({ path: __dirname + '/v130-shots/mall-counter-drag.png' });
  for (const [x, y] of [[320, 180], [320, 100], [320, 60]]) {
    await ftap(x, y, 550);
    console.log('counter tap', x, y, '-> state=', await call('dbg_state'), 'station=', await call('dbg_station'));
    if ((await call('dbg_state')) === 2) break;
  }
  // exit mat
  await call('dbg_tp', 0, -5810); await sleep(250);
  await ftap(320, 180, 600);
  console.log('after exit-mat tap: mall=', await call('dbg_mall'), 'py=', await call('dbg_py'));
  await ftap(320, 240, 600);
  console.log('after second exit-mat tap: mall=', await call('dbg_mall'));
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
