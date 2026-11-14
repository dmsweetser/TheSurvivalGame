// perfbase.js — quick frame-time probe: menu scene + gameplay scene.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const measure = () => page.evaluate(() => new Promise(res => {
    let n = 0; const t0 = performance.now();
    const tick = () => { n++; if (performance.now() - t0 < 4000) requestAnimationFrame(tick); else res(n / ((performance.now() - t0) / 1000)); };
    requestAnimationFrame(tick);
  }));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const menuFps = await measure();
  await page.keyboard.down('Space'); await sleep(150); await page.keyboard.up('Space'); await sleep(600);
  if ((await call('dbg_state')) !== 1) { await page.keyboard.down('Enter'); await sleep(150); await page.keyboard.up('Enter'); await sleep(600); }
  // make the scene busy: a fat hat (zoom out + lots of stack drawing) and some walking
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await page.keyboard.down('KeyD');
  const gameFpsP = measure();
  await sleep(4200); await page.keyboard.up('KeyD');
  const gameFps = await gameFpsP;
  const cv = await page.evaluate(() => { const c = document.getElementById('cv'); return c.width + 'x' + c.height; });
  console.log('canvas:', cv, '| menu fps:', menuFps.toFixed(1), '| gameplay fps:', gameFps.toFixed(1));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
