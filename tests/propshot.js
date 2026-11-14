// propshot.js — eyeball the new junk props: hop to a few spots in the
// world and screenshot the playfield.
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
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  const cv = await page.$('canvas');
  const spots = [[300, 1200], [-1400, -900], [2400, -1400], [-2200, 1500], [900, 2100], [-500, -1900]];
  for (let i = 0; i < spots.length; i++) {
    await call('dbg_tp', spots[i][0], spots[i][1]);
    await sleep(500);
    await cv.screenshot({ path: __dirname + '/props-b' + i + '.png' });
  }
  console.log('shots done');
  await browser.close();
})().catch(e => { console.error('ERROR', e); process.exit(2); });
