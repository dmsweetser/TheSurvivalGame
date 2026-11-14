// playmallshot.js — recapture ONLY the Bug Mall phone screenshot
// (screenshot-03-mall.png) from the current build, for the Google
// Play packet. Same scene/framing as playscreenshots.js step 3:
// enter the mall, teleport to (-75,-6060), shoot at 1920x1080.
// Output: tests/playscreens/screenshot-03-mall.png
const { chromium } = require('playwright-core');
const fs = require('fs');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = __dirname + '/playscreens';
const sleep = ms => new Promise(r => setTimeout(r, ms));
fs.mkdirSync(OUT, { recursive: true });

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  async function poll(fn, args, want, timeout = 9000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }

  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(900);

  await key('Enter', 300, 500);
  await poll('dbg_state', [], v => v === 7, 6000);
  await key('Enter', 300, 600);
  const st = await poll('dbg_state', [], v => v === 1, 10000);
  console.log('run state', st);
  await call('dbg_adopt', 0);
  // match the original capture's big-hat look (playscreenshots.js fed
  // the hat through its combat/wall scenes before the mall shot)
  for (let i = 0; i < 10; i++) await call('dbg_feed');
  console.log('hat stack_n', await call('dbg_carried'));

  // BUG MALL interior — identical framing to playscreenshots.js
  await call('dbg_enter');
  await call('dbg_tp', -75, -6060); await sleep(800);
  console.log('in_mall', await call('dbg_mall'), 'px', await call('dbg_px'), 'py', await call('dbg_py'));
  await page.screenshot({ path: OUT + '/screenshot-03-mall.png' });
  console.log('SHOT screenshot-03-mall.png');
  await browser.close();
})().catch(e => { console.error('FATAL', e); process.exit(1); });
