const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  await page.goto('http://localhost:8903/the-survival-game.html', { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(1500);
  await page.screenshot({ path: 'menu-title.png' });
  // start a run, then pause
  await page.keyboard.press('Space'); await sleep(400);
  await page.keyboard.press('Enter'); await sleep(900);
  await page.evaluate(() => window.__TSG.dbg_pause ? window.__TSG.dbg_pause() : null).catch(()=>{});
  await page.keyboard.press('KeyP'); await sleep(500);
  await page.screenshot({ path: 'menu-pause.png' });
  console.log('state:', await page.evaluate(() => window.__TSG.dbg_state()));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
