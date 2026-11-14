// batch6shots.js — eyeball shots for the Nov 5 evening batch:
// title halo, shop (quantities left + the BEAN row), Whales Only
// scrolled, Buddy Craft shore foam, the bean himself mid-sputter.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(900);
  await page.screenshot({ path: __dirname + '/b6-title.png' });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  // the shop: feed a hat, open the Bounty, select the BEAN row
  for (let i = 0; i < 55; i++) await call('dbg_feed');   // 550 KG on the hat
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300); await sleep(300);              // mall
  await call('dbg_tp', -225, -6108); await sleep(200);
  await key('KeyE', 350); await sleep(400);              // Bug Bounty counter (station 0)
  if ((await call('dbg_state')) === 2 && (await call('dbg_station')) === 0) {
    await key('ArrowUp', 120, 250);                       // wrap to row 12: THE BEAN
    await sleep(300);
    await page.screenshot({ path: __dirname + '/b6-shop-bean.png' });
    const beans0 = await call('dbg_beans', -1);
    await key('Space', 300);                              // buy it: the till counts 500 KG off the hat
    await poll('dbg_beans', v => v === beans0 + 1, 30000);
    console.log('BEAN BOUGHT through the till, beans=', await call('dbg_beans', -1), 'carried=', await call('dbg_carried'));
    await key('Escape', 200, 300);
  } else console.log('shop not open, station=', await call('dbg_station'), 'state=', await call('dbg_state'));
  // Whales Only, scrolled to the bottom
  await call('dbg_tp', -75, -6108); await sleep(200);
  await key('KeyE', 350); await sleep(400);
  if ((await call('dbg_station')) === 1) {
    await key('ArrowUp', 120, 300);
    await page.screenshot({ path: __dirname + '/b6-whale.png' });
    await key('Escape', 200, 300);
  } else console.log('whale not open, station=', await call('dbg_station'));
  // Buddy Craft shore foam: enter craft, stand near water
  await call('dbg_bwentm', 4); await poll('dbg_bwmode', v => v === 4); await sleep(500);
  await page.screenshot({ path: __dirname + '/b6-craft.png' });
  await call('dbg_bwexit'); await sleep(300);
  // out of the mall, bean ride mid-sputter
  const walkTo = async (tx, ty, tol) => { tol = tol || 12;
    for (let i = 0; i < 60; i++) { const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const ks = []; if (x < tx - tol) ks.push('ArrowRight'); else if (x > tx + tol) ks.push('ArrowLeft');
      if (y < ty - tol) ks.push('ArrowDown'); else if (y > ty + tol) ks.push('ArrowUp');
      for (const k of ks) await page.keyboard.down(k); await sleep(90); for (const k of ks) await page.keyboard.up(k); } return false; };
  await call('dbg_tp', 0, -5850); await sleep(250); await key('KeyE', 300); await sleep(300);   // exit mat, straight out
  await call('dbg_tp', 1600, 900); await sleep(200);
  await call('dbg_beans', 1);
  await key('Digit6', 100, 60);
  await sleep(180);   // mid-sputter
  await page.screenshot({ path: __dirname + '/b6-bean-sputter.png' });
  await sleep(500);   // mid-descent
  await page.screenshot({ path: __dirname + '/b6-bean-flight.png' });
  console.log('SHOTS DONE');
  await browser.close();
})().catch(e => { console.log('ERROR', e.message); process.exit(2); });
