// castleshots.js — screenshots: the five-cabinet arcade, the Castle
// courtyard, and the throne room (demon + pillar) for Daniel.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  await poll('dbg_mall', v => v === 1);
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  await poll('dbg_inarcade', v => v === 1, 4000);
  await call('dbg_tp', 0, -12090); await sleep(600);
  await page.screenshot({ path: '/tmp/arcade5.png' });
  console.log('arcade shot done');
  await call('dbg_tp', 292, -12108); await sleep(200);
  await key('KeyE', 250);
  await poll('dbg_arcgate', v => v === 1, 3000);
  await key('Space', 300);
  await poll('dbg_bwmode', v => v === 6, 4000);
  await sleep(900);
  await page.screenshot({ path: '/tmp/castle-courtyard.png' });
  console.log('courtyard shot done');
  await browser.close();
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
