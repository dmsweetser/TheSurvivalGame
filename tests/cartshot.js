// one-off: eyeball the two-row arcade from the middle corridor
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  await sleep(400);
  await call('dbg_tp', 0, -11990); await sleep(600);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/cart-arcade-rows.png' });
  await browser.close();
  console.log('shot saved');
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
