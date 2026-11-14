// Repro 3: close the tab mid-run (no pause-save; autosave only), reopen, continue.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--autoplay-policy=no-user-gesture-required'] });
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 } });
  const key = async (page, k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  let page = await ctx.newPage();
  const call = (f, ...a) => page.evaluate(([f, a]) => window.__TSG[f](...a), [f, a]);
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key(page, 'Space', 500); await key(page, 'Enter', 800);
  await call('dbg_tp', -2600, 900);
  await page.keyboard.down('ArrowRight'); await sleep(900); await page.keyboard.up('ArrowRight');  // walk a bit
  await sleep(6000);                                     // let the 5s autosave fire
  console.log('pos at close:', await call('dbg_px'), await call('dbg_py'));
  await page.close();                                    // hard close, no save
  page = await ctx.newPage();
  const call2 = (f, ...a) => page.evaluate(([f, a]) => window.__TSG[f](...a), [f, a]);
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key(page, 'Space', 600);
  console.log('state at slots:', await call2('dbg_state'));
  await key(page, 'Enter', 900);
  console.log('state after continue:', await call2('dbg_state'));
  console.log('pos after continue:', await call2('dbg_px'), await call2('dbg_py'));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
