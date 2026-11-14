// arcadeshot.js — visual check: the arcade room + the tank game.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(150); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_arcade'); await sleep(600);
  await page.screenshot({ path: __dirname + '/arcadeshot-room.png' });
  await call('dbg_tp', 225, -12108); await sleep(300);
  await page.screenshot({ path: __dirname + '/arcadeshot-cabinet.png' });
  await key('KeyE', 250); await sleep(300);
  await page.screenshot({ path: __dirname + '/arcadeshot-panel.png' });
  await key('Space', 300); await sleep(800);
  await page.keyboard.down('ArrowUp'); await sleep(2200); await page.keyboard.up('ArrowUp');
  await page.keyboard.down('Space'); await sleep(600); await page.keyboard.up('Space');
  await page.screenshot({ path: __dirname + '/arcadeshot-tank.png' });
  console.log('tank lvl', await call('dbg_tanklvl'), 'hull', await call('dbg_tankhull'), 'pass', await call('dbg_tankpass'));
  await browser.close();
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
