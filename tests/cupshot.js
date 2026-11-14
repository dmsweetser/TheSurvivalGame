const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwstory');
  await sleep(400);
  // walk the buddy east into open meadow
  await page.keyboard.down('ArrowRight'); await sleep(5000); await page.keyboard.up('ArrowRight');
  await sleep(300);
  await page.screenshot({ path: __dirname + '/cup-meadow.png' });
  console.log('bw pos', await call('dbg_bwx'), await call('dbg_bwy'));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
