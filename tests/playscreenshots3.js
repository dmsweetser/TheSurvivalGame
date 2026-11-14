// playscreenshots3.js — wall recapture #3: a 5-stack pile plus plain
// neighbours forming a run, framed in the clean middle band of the screen.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = __dirname + '/playscreens';
const sleep = ms => new Promise(r => setTimeout(r, ms));
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
  await sleep(600);
  await key('Enter', 300, 500);
  await poll('dbg_state', [], v => v === 7, 6000);
  await key('Enter', 300, 600);
  await poll('dbg_state', [], v => v === 1, 10000);
  await call('dbg_adopt', 0);
  for (let i = 0; i < 5; i++) await call('dbg_feed'); // 25 carcasses
  // pile at (500,500) facing south, stacked to 5
  await call('dbg_tp', 500, 500); await sleep(300);
  await key('ArrowDown', 90, 150);
  await key('KeyF', 200, 400);
  console.log('walls', await poll('dbg_walls', [], v => v === 1, 8000));
  for (let want = 2; want <= 5; want++) {
    await key('KeyF', 200, 400);
    const s = await poll('dbg_wallstack', [], v => v >= want, 6000);
    console.log('stack now', s);
    if (s < want) break;
  }
  // extend the run east with two plain walls
  const w0 = await call('dbg_walls');
  for (const x of [548, 596]) {
    await call('dbg_tp', x, 500); await sleep(250);
    await key('ArrowDown', 90, 120);
    await key('KeyF', 200, 400);
    console.log('walls after x=' + x, await poll('dbg_walls', [], v => v > w0, 6000));
  }
  console.log('final walls', await call('dbg_walls'), 'layers', await call('dbg_walllayers'), 'grpmax', await call('dbg_grpmax'));
  // frame it: stand south of the run's middle, face north
  await call('dbg_tp', 548, 640); await sleep(250);
  await key('ArrowUp', 90, 200);
  await sleep(300);
  await page.screenshot({ path: OUT + '/screenshot-04-wall.png' });
  console.log('SHOT wall');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
