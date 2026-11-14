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
  await call('dbg_bwtele_site', 1);   // the farm: concrete + surrounding meadow
  await sleep(500);
  await page.screenshot({ path: __dirname + '/land-farm.png' });
  await call('dbg_bwtele_npc', 2);    // Glitch at the frozen lake
  await sleep(500);
  await page.screenshot({ path: __dirname + '/land-lake.png' });
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
