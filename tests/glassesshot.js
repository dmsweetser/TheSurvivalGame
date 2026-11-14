// glassesshot.js — visual check: an adopted pet rock wearing the
// new glasses, next to its DEV outside the mall.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(60); };
  async function walkTo(tx, ty, tol = 12, timeout = 20000) {
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
  const press = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  await press('Space', 500);
  if ((await call('dbg_state')) !== 1) { await press('Enter', 500); }
  await call('dbg_tp', 0, 80); await press('KeyE', 300);
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await walkTo(225, -6108);
  await press('KeyE', 350);
  for (let i = 0; i < 4; i++) await press('ArrowDown', 130);
  await press('Space', 300);                       // species: PET ROCK
  await press('Space', 300);                       // name: AEGNOR
  for (let i = 0; i < 3; i++) await press('ArrowDown', 130);
  await press('Space', 900);                       // color: SKY (glasses read on grey)
  await press('Escape', 300);
  await walkTo(0, -5850); await press('KeyE', 300);  // out of the mall
  await call('dbg_tp', 0, 300); await sleep(1200);   // let the rock settle beside the DEV
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/buddy-glasses.png' });
  await browser.close();
  console.log('shot done');
})();
