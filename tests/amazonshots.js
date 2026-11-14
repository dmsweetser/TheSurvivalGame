// amazonshots.js — capture a fresh Amazon Appstore screenshot set from the
// current build: Mall interior, Banzai Buddies panel, Bug Bowl lanes, and a
// buddy in play outside. Output: ~/workspace/your_files/amazon-listing/shots-fresh/
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = process.env.HOME + '/workspace/your_files/amazon-listing/shots-fresh/';
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1868, height: 1053 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  async function walkTo(tx, ty, tol = 12, timeout = 25000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }
  const cv = await page.$('canvas');
  const shot = async name => { await cv.screenshot({ path: OUT + name }); console.log('SHOT', name); };

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  console.log('state', await call('dbg_state'));
  // enter the mall
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  console.log('mall', await call('dbg_mall'));
  for (let i = 0; i < 6; i++) await call('dbg_feed'); // 60 kg hat for the buddy buy + bowling
  // 1: mall interior, mid-room
  await call('dbg_tp', 0, -5940); await sleep(800);
  await shot('01-mall.png');
  // 2: buddies panel (roster incl. platypus / pet rock / kitchen sink)
  console.log('walk buddies', await walkTo(225, -6108, 14));
  await key('KeyE', 400);
  console.log('station', await call('dbg_station'));
  await sleep(400);
  await shot('02-buddies.png');
  // adopt the pet rock: row 4, name row 0, neon rainbow (row 9)
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350);
  await key('Space', 350);
  for (let i = 0; i < 9; i++) await key('ArrowDown', 120);
  await key('Space', 400);
  await sleep(600);
  console.log('buddy', await call('dbg_buddy'));
  await key('Escape', 300);
  // 3: bowling lanes
  console.log('walk bowl', await walkTo(-170, -5908, 14));
  await key('KeyE', 400);
  console.log('station', await call('dbg_station'));
  await sleep(600);
  await shot('03-bowling.png');
  await key('Escape', 300);
  // 4: outside with the buddy at heel
  console.log('walk exit', await walkTo(0, -5850, 10));
  await key('KeyE', 300);
  console.log('mall after exit', await call('dbg_mall'));
  await call('dbg_tp', 0, 300); await sleep(1200);
  await shot('04-buddy-play.png');
  await browser.close();
  console.log('done');
})().catch(e => { console.error(e); process.exit(1); });
