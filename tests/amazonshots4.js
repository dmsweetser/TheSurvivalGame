// amazonshots4.js — retake 05-action only: big-hat firing line by the
// epicenter, captured from the current build so the listing set's font
// matches the shipped accessibility build.
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
  const cv = await page.$('canvas');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  await call('dbg_tp', 0, 300); await sleep(200);
  for (let i = 0; i < 9; i++) await call('dbg_feed'); // 90 kg hat
  console.log('carried', await call('dbg_carried'));
  await key('KeyF', 300); // throw down a carcass wall beside the player
  console.log('walls', await call('dbg_walls'));
  await call('dbg_spawnbug', 70, 260);
  await call('dbg_spawnbug', -60, 340);
  await call('dbg_spawnbug', 95, 360);
  await call('dbg_spawnbug', 30, 200);
  console.log('bugs', await call('dbg_bugs'));
  // aim right (aim engages while a direction is held) and fire
  await page.keyboard.down('ArrowRight');
  await sleep(150);
  await page.keyboard.press('Space'); await sleep(120);
  await page.keyboard.press('Space'); await sleep(60);
  await cv.screenshot({ path: OUT + '05-action.png' });
  console.log('SHOT 05-action.png');
  await page.keyboard.up('ArrowRight');
  await browser.close();
  console.log('done');
})().catch(e => { console.error(e); process.exit(1); });
