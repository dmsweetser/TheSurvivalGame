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
  // STORY: stand next to Elder Patchina in the village -> "E - TALK"
  await call('dbg_bwentm', 1);
  await sleep(600);
  await call('dbg_bwtp', -2600, -1400 + 60);
  await sleep(300);
  await page.keyboard.down('ArrowUp'); await sleep(120); await page.keyboard.up('ArrowUp');
  await sleep(400);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/probe-talk.png' });
  await key('Escape', 300); await sleep(400);
  // CRAFT: screenshot IMMEDIATELY on entry (biome note bar visible)
  await call('dbg_bwentm', 4);
  await sleep(350);
  await cv.screenshot({ path: __dirname + '/probe-craftnote.png' });
  await browser.close();
  console.log('done');
})().catch(e => { console.error('ERR', e); process.exit(1); });
