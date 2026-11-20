// tilltest2.js — batch counting (Nov 10): a 500 KG bean paid from a
// hat of 275 little carcasses used to take ~28 s of one-by-one pops.
// The till now counts in handfuls scaled to the bill: the whole
// payment should settle in a couple of seconds, still land the item,
// still take EXACTLY 500 KG, and the till lock must still hold while
// it counts.
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
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(60); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));

  for (let i = 0; i < 55; i++) await call('dbg_feed');   // 275 carcasses, 550 KG
  T('hat holds 550 KG', (await call('dbg_carried')) === 5500, await call('dbg_carried'));

  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -315, -6148); await sleep(250);
  for (let i = 0; i < 5 && (await call('dbg_state')) !== 2; i++) await key('KeyE', 250);
  T('bounty panel open (GS_STORE)', (await call('dbg_state')) === 2, await call('dbg_state'));
  for (let i = 0; i < 40 && (await call('dbg_storesel')) !== 12; i++) await key('ArrowDown', 100);
  T('cursor on the bean row', (await call('dbg_storesel')) === 12, await call('dbg_storesel'));

  const t0 = Date.now();
  await key('Space', 60);   // buy: the till starts counting
  T('the till is counting', (await poll('dbg_pay', v => v > 0, 3000)) > 0, await call('dbg_pay'));
  // the lock must hold mid-count: Escape bounces off
  await page.keyboard.press('Escape');
  await sleep(120);
  T('Escape does NOT close the panel mid-count', (await call('dbg_state')) === 2 && (await call('dbg_pay')) > 0,
    'state=' + (await call('dbg_state')) + ' pay=' + (await call('dbg_pay')));
  let b = 0;
  while (Date.now() - t0 < 15000) { b = await call('dbg_beans', -1); if (b === 1) break; await sleep(100); }
  const elapsed = Date.now() - t0;
  T('the bean lands', b === 1, b);
  T('the whole 500 KG count took under 5 s (was ~28 s)', elapsed < 5000, elapsed + 'ms');
  console.log('bean payment elapsed: ' + elapsed + 'ms');
  T('exactly 500 KG was counted out (50 KG left)', (await call('dbg_carried')) === 500, await call('dbg_carried'));
  T('till idle after the count', (await call('dbg_pay')) === 0, await call('dbg_pay'));
  await key('Escape', 300);
  T('Escape closes the panel after the count', (await poll('dbg_state', v => v === 1)) === 1, await call('dbg_state'));

  console.log('tilltest2: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
