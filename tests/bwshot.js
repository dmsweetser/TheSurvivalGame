// bwshot.js — Banzai World palette before/after from the same spot.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const tag = process.argv[2] || 'before';
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwenter');
  await sleep(500);
  // stroll north-east into the trees for a representative view
  await page.keyboard.down('KeyW'); await page.keyboard.down('KeyD');
  await sleep(2600);
  await page.keyboard.up('KeyW'); await page.keyboard.up('KeyD');
  await sleep(300);
  await page.screenshot({ path: __dirname + '/bw-pal-' + tag + '.png' });
  console.log('shot', tag, 'state', await call('dbg_state'));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
