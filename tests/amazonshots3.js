// amazonshots3.js — retake 04 only: buddy trailing visibly behind the player.
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
  await walkTo(225, -6108, 14);
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await key('KeyE', 350);
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350); await key('Space', 350);
  for (let i = 0; i < 9; i++) await key('ArrowDown', 120);
  await key('Space', 400);
  const t0 = Date.now();
  while (Date.now() - t0 < 15000 && (await call('dbg_buddy')) === 0) await sleep(300);
  console.log('buddy', await call('dbg_buddy'));
  await key('Escape', 300);
  await walkTo(0, -5850, 10);
  await key('KeyE', 300);
  await call('dbg_tp', 0, 300); await sleep(0);
  await sleep(100);
  await sleep(1200);
  await cv.screenshot({ path: OUT + '04-buddy-play.png' });
  console.log('done');
  await browser.close();
})().catch(e => { console.error(e); process.exit(1); });
