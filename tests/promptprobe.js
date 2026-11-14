const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwentm', 4);
  await sleep(500);
  let tree = null;
  for (let r = 3; r <= 16 && !tree; r++)
    for (let tx = -r; tx <= r && !tree; tx++)
      for (let ty = -r; ty <= r && !tree; ty++)
        if ((await call('dbg_craftnode', tx, ty)) === 1) tree = [tx, ty];
  console.log('tree tile', tree);
  await call('dbg_bwtp', tree[0] * 48 + 24 - 40, tree[1] * 48 + 24);
  await sleep(300);
  await page.keyboard.down('ArrowRight'); await sleep(120); await page.keyboard.up('ArrowRight');
  await sleep(2600);   // let any biome note expire
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/probe-craftprompt2.png' });
  console.log('buddy x,y via dbg_budy', await call('dbg_budy'));
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
