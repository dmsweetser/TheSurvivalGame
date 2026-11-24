// audit130-verify3.js — T1 companion: capture still binds a pressed key.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const ctx = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  const page = await ctx.newPage();
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (k, w = 350) => { await page.keyboard.press(k); await sleep(w); };
  const clickGame = async (gx, gy, wait = 450) => {
    const box = await page.locator('#cv').boundingBox();
    await page.mouse.click(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);
  await key('ArrowDown'); await key('ArrowDown'); await key('Space', 600);
  console.log('settings state =', await call('dbg_state'));
  await clickGame(320, 134, 400);   // select MOVE UP
  await clickGame(320, 134, 500);   // capture
  await key('KeyQ', 600);           // binds MOVE UP = Q
  await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-q-bound.png' });
  console.log('done');
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
