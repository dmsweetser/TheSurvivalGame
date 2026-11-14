// scratch probe: catch the oblong rock mid-roll while it trails a walking player
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_tp', -500, 900);               // open meadow, room to walk
  await sleep(200);
  await call('dbg_adopt', 4);                    // pet rock, beside the player
  await sleep(300);                              // let it settle at heel
  const cv = await page.$('canvas');
  await page.keyboard.down('ArrowDown');         // walk south; the rock trails, rolling
  for (let i = 0; i < 3; i++) {
    await sleep(420);
    await cv.screenshot({ path: __dirname + '/roll-' + i + '.png' });
  }
  await page.keyboard.up('ArrowDown');
  await browser.close();
  console.log('done');
})().catch(e => { console.error('ERROR', e); process.exit(2); });
