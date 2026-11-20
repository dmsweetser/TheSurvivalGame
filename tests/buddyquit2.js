// buddyquit2.js — variant B of Daniel's report: buy the buddy for
// real, leave the mall, PAUSE -> SAVE + QUIT TO TITLE (the explicit
// save path, no page reload), then CONTINUE from the title and check
// the buddy is physically with you in the mall.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = [];
const T = (name, ok, info) => { console.log((ok ? 'PASS' : 'FAIL') + ' | ' + name + (ok ? '' : ' | ' + info)); if (!ok) fails.push(name); };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (fn, pred, timeout = 15000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < timeout) { v = await call(fn); if (pred(v)) return v; await sleep(250); } return v; };
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
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

  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('entered the mall', (await call('dbg_mall')) === 1);
  T('walked to the buddies counter', await walkTo(315, -6148, 14));
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await key('KeyE', 350);
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350); await key('Space', 350); await key('Space', 400);
  T('pet rock adopted', (await poll('dbg_buddy', v => v !== 0, 20000)) === 5);
  await key('Escape', 300);
  T('walked to the exit mat', await walkTo(0, -5810, 10));
  await key('KeyE', 300);
  T('left the mall', (await call('dbg_mall')) === 0);
  T('buddy followed out', (await poll('dbg_budy', v => v > -1000, 5000)) > -1000);
  await call('dbg_tp', 2000, 1000); await sleep(300);
  T('buddy caught up in the meadow', Math.abs((await poll('dbg_budy', v => Math.abs(v - 1000) < 160, 30000)) - 1000) < 160);

  // PAUSE -> SAVE + QUIT TO TITLE
  await key('KeyP', 300);
  T('paused', (await call('dbg_state')) === 5, await call('dbg_state'));   // GS_PAUSE is 5 (3 is GS_OVER)
  // pause menu is 3 rows since Nov 6 (Daniel): CONTINUE / CONTROLS /
  // SAVE + QUIT TO TITLE — two Downs reach the quit row.
  await key('ArrowDown', 150);
  await key('ArrowDown', 150);
  await key('Space', 500);
  T('back at title', (await call('dbg_state')) === 0, await call('dbg_state'));
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('slot save carries the buddy (bd=)', / bd=\d/.test(raw), raw.slice(0, 160));

  // CONTINUE from the title
  await key('Space', 500);
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await key('Space', 600);
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(500);
  T('buddy still in the data', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  const py = await call('dbg_py'), by2 = await call('dbg_budy');
  console.log('after Continue: player y=' + py + ' buddy y=' + by2);
  await page.screenshot({ path: __dirname + '/buddyquit2-mall.png' });
  T('BUDDY IS WITH YOU in the mall after Continue', Math.abs(by2 - py) < 300, 'player y=' + py + ' buddy y=' + by2);
  console.log(fails.length ? 'BUDDYQUIT2 FAILURES: ' + fails.join(', ') : 'BUDDYQUIT2 ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
