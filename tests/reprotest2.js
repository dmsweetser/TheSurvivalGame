// Repro 2: save while INSIDE the mall, then continue.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (f, ...a) => page.evaluate(([f, a]) => window.__TSG[f](...a), [f, a]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500); await key('Enter', 800);      // new run, slot 1
  console.log('state:', await call('dbg_state'));
  await call('dbg_enter'); await sleep(500);             // into the mall
  console.log('pos in mall:', await call('dbg_px'), await call('dbg_py'));
  await call('dbg_tp', 150, -5950); await sleep(300);    // wander deep into the mall
  console.log('pos before save:', await call('dbg_px'), await call('dbg_py'));
  await key('KeyP', 500); await key('ArrowDown', 250); await key('Space', 700);
  console.log('state after save+quit:', await call('dbg_state'));
  await key('Space', 600); await key('Enter', 900);      // continue, slot 1
  console.log('state after continue:', await call('dbg_state'));
  console.log('pos after continue:', await call('dbg_px'), await call('dbg_py'));
  await page.screenshot({ path: 'repro2-after.png' });
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
