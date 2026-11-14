// shots-a11y.js — screenshot pass for the accessibility build:
// title, HOW TO PLAY, store panel, buddy panel, in-world HUD.
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
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
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
  await sleep(600);
  await page.screenshot({ path: __dirname + '/a11y-title.png' });
  await key('ArrowDown', 120); await key('ArrowDown', 120); await key('Space', 500);
  await page.screenshot({ path: __dirname + '/a11y-howto.png' });
  await key('Escape', 300);
  await key('ArrowUp', 120); await key('ArrowUp', 120);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);                        // into the mall
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await walkTo(-225, -6108, 14);                 // Bug Bounty counter (station 0)
  await key('KeyE', 500);
  await page.screenshot({ path: __dirname + '/a11y-store.png' });
  await key('Escape', 300);
  await walkTo(225, -6108, 14);                  // Banzai Buddies counter
  await key('KeyE', 500);
  await page.screenshot({ path: __dirname + '/a11y-buddy.png' });
  await key('Escape', 300);
  await walkTo(0, -5850, 10);                    // exit mat
  await sleep(500);
  await page.screenshot({ path: __dirname + '/a11y-world.png' });
  console.log('shots done');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
