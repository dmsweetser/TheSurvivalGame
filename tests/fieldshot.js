// fieldshot.js — one proper action shot: out in the meadow, bugs closing
// in, buddy alongside with its energy bar spent down so it shows.
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
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);                          // moose — big and photogenic
  for (let i = 0; i < 8; i++) await call('dbg_feed');   // modest hat
  await call('dbg_setbudenergy', 52);
  await call('dbg_tp', 0, 330); await sleep(200); await call('dbg_setthreat', 2);
  // face south and let the swarm arrive; fire a little for life
  await page.keyboard.down('KeyS'); await sleep(260); await page.keyboard.up('KeyS');
  for (let i = 0; i < 30; i++) {
    await sleep(800);
    const bugs = await call('dbg_bugs');
    console.log('poll', i, 'bugs:', bugs, 'state:', await call('dbg_state'));
    if (bugs >= 4) {
      await call('dbg_setbudenergy', 52);
      await page.keyboard.down('Space'); await sleep(150); await page.keyboard.up('Space');
      await page.screenshot({ path: __dirname + '/page-field2.png' });
      break;
    }
    if (i === 29) await page.screenshot({ path: __dirname + '/page-field2.png' });
  }
  console.log('fieldshot done');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
