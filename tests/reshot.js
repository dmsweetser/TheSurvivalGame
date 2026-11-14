// reshot.js — resolution-doubling visual check: title, field, panel.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(900);
  await page.screenshot({ path: __dirname + '/res2-title.png' });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  for (let i = 0; i < 4; i++) await call('dbg_feed');
  await sleep(500);
  await page.screenshot({ path: __dirname + '/res2-field.png' });
  console.log('DONE');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
