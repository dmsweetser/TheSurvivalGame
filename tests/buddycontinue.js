// buddycontinue.js — Daniel's exact report, real path end to end:
// buy a buddy at the Banzai Buddies counter, walk OUT of the mall,
// play in the meadow, let the game save, reload, CONTINUE through
// the slot picker — and check the buddy is actually WITH you in the
// mall (not just present in the save data).
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

  // into the mall, to the buddies counter, buy a pet rock for real
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('entered the mall', (await call('dbg_mall')) === 1);
  T('walked to the buddies counter', await walkTo(225, -6108, 14));
  for (let i = 0; i < 6; i++) await call('dbg_feed');   // 60 KG: a rock's price
  await key('KeyE', 350);
  T('buddies panel open', (await call('dbg_station')) === 3);
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);   // row 4: PET ROCK
  await key('Space', 350);
  await key('Space', 350);                                    // name: row 0
  await key('Space', 400);                                    // color: row 0 → pay
  const bud = await poll('dbg_buddy', v => v !== 0, 20000);
  T('pet rock adopted after payment', bud === 5, bud);
  await key('Escape', 300);

  // out of the mall, deep into the meadow — where a real session saves
  T('walked to the exit mat', await walkTo(0, -5850, 10));
  await key('KeyE', 300);
  T('left the mall', (await call('dbg_mall')) === 0);
  T('buddy followed out', (await poll('dbg_budy', v => v > -1000, 5000)) > -1000);
  await call('dbg_tp', 2000, 1000); await sleep(300);
  const by = await poll('dbg_budy', v => Math.abs(v - 1000) < 160, 30000);
  T('buddy caught up in the meadow', Math.abs(by - 1000) < 160, by);

  // let the autosave take it, confirm the buddy is in the save
  await sleep(7000);
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('slot 1 saved', raw.length > 50, raw.length);
  T('save carries the buddy (bd=)', / bd=\d/.test(raw), raw.slice(0, 160));

  // reload + CONTINUE through the picker, like Daniel does
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500);
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await key('Space', 600);
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(500);
  T('buddy still in the data', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  const py = await call('dbg_py'), by2 = await call('dbg_budy');
  console.log('after Continue: player y=' + py + ' buddy y=' + by2);
  await page.screenshot({ path: __dirname + '/buddycontinue-mall.png' });
  T('BUDDY IS WITH YOU in the mall after Continue', Math.abs(by2 - py) < 300, 'player y=' + py + ' buddy y=' + by2);
  console.log(fails.length ? 'BUDDYCONTINUE FAILURES: ' + fails.join(', ') : 'BUDDYCONTINUE ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
