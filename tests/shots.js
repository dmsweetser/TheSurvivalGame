// Visual checks: the hut sign at the plaza, and the mall entrance
// with its floor-painted name (the wall sign used to hide behind
// the HUD pill).
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
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  await call('dbg_tp', 0, 120); await sleep(600);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/shot-hut.png' });
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 400);
  await sleep(600);
  await cv.screenshot({ path: __dirname + '/shot-mall-entrance.png' });
  console.log('done');
  await browser.close();
})();
