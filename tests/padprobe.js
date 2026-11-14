// padprobe.js — why doesn't pad d-pad move the title selection?
const { chromium } = require('playwright-core');
const URL = 'http://127.0.0.1:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 200)));
  await page.addInitScript(() => {
    const mkpad = (idx) => ({ index: idx, id: 'Fake Pad', connected: true, mapping: 'standard',
      axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })) });
    window.__pads = [];
    navigator.getGamepads = () => window.__pads;
    window.__mkpad = mkpad;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => { window.__pads = [window.__mkpad(0)]; });
  await page.waitForFunction(() => window.__TSG.dbg_pads() === 1, null, { timeout: 5000 });
  await sleep(300);
  await page.screenshot({ path: 'padprobe-0.png' });
  // hold d-pad down 400ms (auto-repeat territory)
  await page.evaluate(() => window.__btn(0, 13, true));
  await sleep(400);
  await page.evaluate(() => window.__btn(0, 13, false));
  await sleep(300);
  await page.screenshot({ path: 'padprobe-1.png' });
  // direct export taps: act(1) down/up twice
  await page.evaluate(() => { window.__TSG.act(1, 1); });
  await sleep(120);
  await page.evaluate(() => { window.__TSG.act(1, 0); });
  await sleep(300);
  await page.screenshot({ path: 'padprobe-2.png' });
  console.log('state', await page.evaluate(() => window.__TSG.dbg_state()));
  await browser.close();
})().catch(e => { console.error('ERROR', e); process.exit(2); });
