// amazonshots5.js — retake 03-bowling only: a started Bug Bowl game with
// the full pin rack and power meter, from the current build (the plain
// amazonshots.js pass captures the lane before a game is started).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = process.env.HOME + '/workspace/your_files/amazon-listing/shots-fresh/';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1868, height: 1053 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
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
  const cv = await page.$('canvas');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  console.log('mall', await call('dbg_mall'));
  for (let i = 0; i < 8; i++) await call('dbg_feed'); // balls cost carcasses
  console.log('walk bowl', await walkTo(-170, -5908, 14));
  await key('KeyE', 400);
  console.log('station', await call('dbg_station'));
  await key('Space', 250); await key('Space', 300);
  await sleep(350);
  await cv.screenshot({ path: OUT + '03-bowling.png' });
  console.log('SHOT 03-bowling.png');
  await browser.close();
  console.log('done');
})().catch(e => { console.error(e); process.exit(1); });
