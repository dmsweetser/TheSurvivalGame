// whales2shot.js — visual check for round 2: the 16-row panel, the
// parade duck + aura in the field, crown + bubble helmet on a buddy,
// and the disco mall floor.
const { chromium } = require('playwright-core');
const fs = require('fs');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const { codes } = JSON.parse(fs.readFileSync(__dirname + '/whale-codes.json', 'utf8'));
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const redeemVia = async code => page.evaluate(c => {
    const ptr = window.__TSG.dbg_scratch();
    const u8 = new Uint8Array(window.__TSG.memory.buffer, ptr, 256);
    for (let i = 0; i < 256; i++) u8[i] = i < c.length ? c.charCodeAt(i) : 0;
    return window.__TSG.dbg_redeem();
  }, code);
  const walkTo = async (wx, wy, tol) => {
    for (let s = 0; s < 46; s++) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = wx - x, dy = wy - y;
      if (Math.abs(dx) < tol && Math.abs(dy) < tol) return true;
      const k = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'KeyD' : 'KeyA') : (dy > 0 ? 'KeyS' : 'KeyW');
      await page.keyboard.down(k); await sleep(110); await page.keyboard.up(k); await sleep(30);
    }
    return false;
  };
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  for (let s = 0; s < 15; s++) await redeemVia(codes[s]);
  console.log('own/on:', await call('dbg_whale'), await call('dbg_whaleon'));
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  // field: duck trailing + aura under the player; walk a bit so the duck hops
  await page.keyboard.down('KeyD'); await sleep(1500); await page.keyboard.up('KeyD');
  await sleep(300);
  await page.screenshot({ path: __dirname + '/whale2-field.png' });
  // into the mall: disco floor + panel + buddy close-up happens at the counters
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  await sleep(400);
  await page.screenshot({ path: __dirname + '/whale2-disco.png' });
  await walkTo(-75, -6108, 14);
  await key('KeyE', 350);
  await sleep(400);
  await page.screenshot({ path: __dirname + '/whale2-panel.png' });
  await key('Escape', 300);
  // buddy close-up near the buddies counter (crown + helmet on the puppy)
  await walkTo(225, -6030, 16);
  await sleep(600);
  await page.screenshot({ path: __dirname + '/whale2-buddy.png' });
  console.log('SHOTS DONE');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
