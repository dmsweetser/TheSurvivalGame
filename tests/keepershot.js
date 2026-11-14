// keepershot.js — visual check: the five Woz keepers behind their
// counters, and a buddy wearing its new glasses next to the DEV.
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
  await key('Space', 500);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 500); }
  await call('dbg_tp', 0, 80); await key('KeyE', 400);          // into the mall
  await call('dbg_tp', -225, -6118); await sleep(600);          // at the west counters, shopping distance
  await page.screenshot({ path: __dirname + '/keeper-west.png' });
  await call('dbg_tp', 225, -6118); await sleep(600);           // east counters, shopping distance
  await page.screenshot({ path: __dirname + '/keeper-east.png' });
  await call('dbg_tp', -258, -5820); await sleep(600);          // bowling counter (south-west)
  await page.screenshot({ path: __dirname + '/keeper-bowl.png' });
  // out in the world with a buddy: adopt one first via the panel flow is
  // long; instead check the buddy panel keeper + adopted rock close-up
  await call('dbg_tp', 225, -6108); await sleep(300);
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await key('KeyE', 400);                                        // buddies panel
  await page.screenshot({ path: __dirname + '/keeper-buddies.png' });
  await browser.close();
  console.log('shots done');
})();
