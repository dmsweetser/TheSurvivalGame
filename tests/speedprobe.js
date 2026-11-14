// speedprobe.js — Espresso Legs, measured (Nov 9 rebalance).
// dbg_speed reads player_speed() directly, dbg_overhat(n) sets a
// hat of n carcasses (3kg each), dbg_setupg(4, lv) sets the speed
// upgrade. Grid: levels {0,5,10,25,60} x hats {0, 99, 240}kg.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = process.env.URL || 'http://localhost:8903/the-survival-game.html';
const LABEL = process.env.LABEL || 'BUILD';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Enter', 300, 500);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 7, null, { timeout: 10000 });
  await key('Enter', 300, 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  console.log(`== speedprobe ${LABEL} (px/s) ==`);
  console.log('level | hat 0kg | hat 99kg | hat 240kg');
  for (const lv of [0, 5, 10, 25, 60]) {
    await call('dbg_setupg', 4, lv);
    const row = [];
    for (const n of [0, 33, 80]) {
      await call('dbg_overhat', n);
      await sleep(200);
      row.push(((await call('dbg_speed')) / 10).toFixed(1));
    }
    console.log(`L${lv} | ${row.join(' | ')}`);
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
