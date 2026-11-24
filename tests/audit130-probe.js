// audit130-probe.js — find the tap_on_store rect empirically (DRAG).
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
  const ftap = async (x, y, wait = 450) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
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
  await sleep(600);
  console.log('state=', await call('dbg_state'), 'touchctl=', await call('dbg_touchctl', -1));
  await call('dbg_adopt', 0);
  await page.screenshot({ path: __dirname + '/v130-shots/plaza-drag.png' });
  await call('dbg_tp', 0, 80); await sleep(250);
  for (const y of [30, 50, 70, 90, 110, 130]) {
    await ftap(320, y, 550);
    const m = await call('dbg_mall');
    console.log('tap y=' + y, '-> mall=', m);
    if (m === 1) break;
  }
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
