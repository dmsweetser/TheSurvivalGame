// mallprobe.js — map the mall's walkable space by WALKING lanes in both
// directions and unioning the visited 10px bins. Gaps = solid at that y.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  if ((await call('dbg_mall')) !== 1) { console.log('NOT IN MALL'); process.exit(1); }
  const CY = -6000;
  async function sweep(y, dirKey, x0) {
    const bins = new Set();
    await call('dbg_tp', x0, CY + y); await sleep(120);
    await page.keyboard.down(dirKey);
    let lastX = await call('dbg_px'), stall = 0;
    for (let i = 0; i < 60; i++) {
      await sleep(90);
      const x = await call('dbg_px');
      bins.add(Math.round(x / 10));
      if (Math.abs(x - lastX) < 0.6) { if (++stall >= 6) break; } else stall = 0;
      lastX = x;
      if (Math.abs(x) >= 281) break;
    }
    await page.keyboard.up(dirKey);
    return bins;
  }
  console.log('lane map: x bins of 10px from -280..280; # = never visited (solid), . = walkable');
  for (let y = -165; y <= 160; y += 15) {
    const a = await sweep(y, 'KeyD', -272);
    const b = await sweep(y, 'KeyA', 272);
    let row = '';
    for (let bx = -28; bx <= 28; bx++) row += (a.has(bx) || b.has(bx)) ? '.' : '#';
    console.log(String(y).padStart(4), row);
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
