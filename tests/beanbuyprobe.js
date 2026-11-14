// beanbuyprobe.js — watch a bean purchase tick by tick.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  for (let i = 0; i < 55; i++) await call('dbg_feed');
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300); await sleep(300);
  await call('dbg_tp', -225, -6108); await sleep(200);
  await key('KeyE', 350); await sleep(400);
  console.log('state', await call('dbg_state'), 'station', await call('dbg_station'), 'carried', await call('dbg_carried'));
  await key('ArrowUp', 120, 250);
  console.log('sel', await call('dbg_storesel'));
  await page.keyboard.down('Space'); await sleep(90); await page.keyboard.up('Space');
  for (let i = 0; i < 240; i++) {
    await sleep(250);
    console.log('t=' + ((i + 1) * 250) + 'ms carried', await call('dbg_carried'), 'beans', await call('dbg_beans', -1), 'state', await call('dbg_state'));
    if ((await call('dbg_beans', -1)) > 0) break;
  }
  await browser.close();
})().catch(e => { console.log('ERROR', e.message); process.exit(2); });
