// tilltest.js — the till lock (Nov 6): while the Bug Bounty's till is
// counting your carcasses for a purchase, you cannot leave the buy
// menu and cannot start another purchase. When the counting ends,
// the item lands and the menu unlocks.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 130) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));

  // 500 KG of hat, in little 2 KG carcasses so the counting takes a while
  for (let i = 0; i < 50; i++) await call('dbg_feed');
  T('hat holds 500 KG', (await call('dbg_carried')) >= 4990, await call('dbg_carried'));

  // into the mall, up to the Bug Bounty counter, panel open
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -315, -6148); await sleep(250);
  for (let i = 0; i < 5 && (await call('dbg_state')) !== 2; i++) await key('KeyE', 250);
  T('bounty panel open (GS_STORE)', (await call('dbg_state')) === 2, await call('dbg_state'));

  // walk the cursor to the bean (row 12), verifying as we go
  for (let i = 0; i < 40 && (await call('dbg_storesel')) !== 12; i++) await key('ArrowDown', 100);
  T('cursor on the bean row', (await call('dbg_storesel')) === 12, await call('dbg_storesel'));
  await key('Space', 250);   // buy: the till starts counting
  T('the till is counting', (await poll('dbg_pay', v => v > 0, 4000)) > 0, await call('dbg_pay'));

  // exit attempts must bounce off while the till counts
  await key('Escape', 250);
  T('Escape does NOT close the panel mid-count', (await call('dbg_state')) === 2 && (await call('dbg_pay')) > 0,
    'state=' + (await call('dbg_state')) + ' pay=' + (await call('dbg_pay')));
  await key('KeyE', 250);
  T('Interact does NOT close the panel mid-count', (await call('dbg_state')) === 2 && (await call('dbg_pay')) > 0,
    'state=' + (await call('dbg_state')) + ' pay=' + (await call('dbg_pay')));
  // a second purchase attempt must also bounce. (Nov 10: counting is
  // batched now and finishes in ~1 s, so there is no time to navigate
  // to another row mid-count like this test used to — fire the second
  // attempt immediately, on the same row, and prove at the end that
  // exactly ONE bean was ever bought.)
  await key('Space', 120);
  T('second buy attempt bounces off the counting till', (await call('dbg_state')) === 2 && (await call('dbg_pay')) > 0,
    'state=' + (await call('dbg_state')) + ' pay=' + (await call('dbg_pay')));

  // the counting finishes: exactly one bean, exactly 500 KG gone
  { const t0 = Date.now(); let b = 0;
    while (Date.now() - t0 < 45000) { b = await call('dbg_beans', -1); if (b === 1) break; await sleep(300); }
    T('the bean lands when the counting ends', b === 1, b); }
  const carried = await call('dbg_carried');
  T('exactly 500 KG was counted out', carried <= 15, carried);

  // and now the menu unlocks again
  await key('Escape', 300);
  T('Escape closes the panel after the count', (await poll('dbg_state', v => v === 1)) === 1, await call('dbg_state'));

  console.log('tilltest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
