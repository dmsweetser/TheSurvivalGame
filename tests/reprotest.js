// Repro: does CONTINUE restore where you were?
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
  // fresh run: title START (no save yet) -> slots (new-game mode) -> slot 1
  await key('Space', 500);
  console.log('state after START:', await call('dbg_state'));
  await key('Enter', 800);
  console.log('state after slot pick:', await call('dbg_state'));
  // go far away
  await call('dbg_tp', 2000, 1500); await sleep(300);
  console.log('pos before save:', await call('dbg_px'), await call('dbg_py'));
  // pause -> SAVE + QUIT TO TITLE (row 2)
  await key('KeyP', 500);
  console.log('state paused:', await call('dbg_state'));
  await key('ArrowDown', 250);
  await key('Space', 700);
  console.log('state after save+quit:', await call('dbg_state'));
  // title: CONTINUE row -> slots load mode -> slot 1
  await key('Space', 600);
  console.log('state after CONTINUE:', await call('dbg_state'));
  await key('Enter', 900);
  console.log('state after load pick:', await call('dbg_state'));
  console.log('pos after continue:', await call('dbg_px'), await call('dbg_py'));
  await page.screenshot({ path: 'repro-after.png' });
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
