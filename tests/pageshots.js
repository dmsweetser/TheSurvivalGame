// pageshots.js — fresh itch-page screenshots for the batch-5 publish:
// title, field action with buddy + energy bar, mall gate (reframed).
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
  const shot = n => page.screenshot({ path: __dirname + '/page-' + n + '.png' });

  // 1) title
  await sleep(1200);
  await shot('title');

  // run + buddy + a hat, energy partly spent so the blue bar shows
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);                       // puppy
  for (let i = 0; i < 14; i++) await call('dbg_feed');  // a respectable hat
  await call('dbg_setbudenergy', 46);
  await sleep(900);                                  // let bugs drift in
  await shot('field');

  // 2) mall gate, reframed: stand NW of the gate so the mall fills the frame
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);                            // into the mall
  await call('dbg_tp', 40, -6000 + 30); await sleep(500);
  await shot('gate');

  console.log('pageshots done');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
