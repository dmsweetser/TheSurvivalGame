// menushot.js — eyeball the ONE-list craft menu: top, and scrolled deep.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 140) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwentm', 4); await sleep(400);
  await call('dbg_craftsetm', 60 + 60 * 1000 + 60 * 1000000); await call('dbg_craftsetcar', 30);
  await key('KeyF', 300); await sleep(400);
  await page.screenshot({ path: __dirname + '/menu-top.png' });
  for (let i = 0; i < 16; i++) await key('ArrowDown', 90);   // down into BUILDING
  await sleep(300);
  await page.screenshot({ path: __dirname + '/menu-deep.png' });
  console.log('sel row id now', await call('dbg_craftrowid'));
  // the daughter test: sloppy diagonals must NOT switch anything —
  // hold RIGHT and tap DOWN repeatedly; selection may only scroll.
  const before = await call('dbg_craftrowid');
  await page.keyboard.down('ArrowRight'); await sleep(120);
  for (let i = 0; i < 3; i++) await key('ArrowDown', 110);
  await page.keyboard.up('ArrowRight');
  console.log('diagonal test: rowid', before, '->', await call('dbg_craftrowid'), '(must have only scrolled down 3 craftable rows)');
  await browser.close();
})().catch(e => { console.log('ERR', e.message); process.exit(1); });
