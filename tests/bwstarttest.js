// bwstarttest.js — Banzai World touch Start chip (v1.26 Item 2):
// on touch the Start menu (RESUME / CONTROLS / LEAVE) had no way to
// open — the chip at the view's top-left opens it, its rows were
// already tappable, and the keyboard path (P / Esc) is unchanged.
// Drives the real touch path (touch_point, the shell's own calls).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120) => { await page.keyboard.down(code); await sleep(140); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const tap = async (x, y) => { await call('touch_point', 9, x, y, 0); await sleep(90); await call('touch_point', 9, x, y, 2); await sleep(160); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await call('set_mobile', 1);
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  await call('dbg_touchctl', 1);   // STICK scheme
  await call('dbg_bwentm', 0);     // Free Play
  T('in Banzai World Free Play', (await poll('dbg_bwmode', v => v === 0)) === 0, await call('dbg_bwmode'));
  T('menu closed before the chip', (await call('dbg_bwmenu')) === 0, await call('dbg_bwmenu'));

  // the chip: top-left of the view, 72x24 at (8,8) -> centre (44,20)
  await tap(44, 20);
  T('chip tap opens the Start menu', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));

  // CONTROLS row: panel (170,110) 300x140, rows at y 158/184/210
  await tap(320, 184);
  T('CONTROLS row opens the controls panel', (await poll('dbg_bwmenu', v => v === 2)) === 2, await call('dbg_bwmenu'));
  await tap(320, 300);
  T('a tap backs out to the menu', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));

  // RESUME row closes without leaving
  await tap(320, 158);
  T('RESUME closes the menu, still in BW', (await poll('dbg_bwmenu', v => v === 0)) === 0 && (await call('dbg_bwmode')) === 0,
    'menu=' + (await call('dbg_bwmenu')) + ' mode=' + (await call('dbg_bwmode')));

  // chip again, then LEAVE
  await tap(44, 20);
  T('chip reopens the menu', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));
  await tap(320, 210);
  T('LEAVE exits Banzai World', (await poll('dbg_bwmode', v => v === -1)) === -1, await call('dbg_bwmode'));
  T('menu closed after leaving', (await call('dbg_bwmenu')) === 0, await call('dbg_bwmenu'));

  // the chip again after a scheme-setter call: since v1.33 there is
  // one touch scheme (dbg_touchctl coerces any set to STICK), and
  // the chip — drawn outside the scheme's touch layer — still works
  await call('dbg_touchctl', 0);
  await call('dbg_bwentm', 0);
  T('back in Free Play (scheme pinned to STICK)', (await poll('dbg_bwmode', v => v === 0)) === 0, await call('dbg_bwmode'));
  await tap(44, 20);
  T('chip opens the menu with the scheme pinned too', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));
  await tap(320, 210);
  T('LEAVE exits again', (await poll('dbg_bwmode', v => v === -1)) === -1, await call('dbg_bwmode'));

  // keyboard path unchanged: P opens, Esc closes
  await call('dbg_bwentm', 0);
  T('back in Free Play (keyboard check)', (await poll('dbg_bwmode', v => v === 0)) === 0, await call('dbg_bwmode'));
  await key('KeyP', 300);
  T('P still opens the menu', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));
  await key('Escape', 300);
  T('Esc still closes it', (await poll('dbg_bwmenu', v => v === 0)) === 0, await call('dbg_bwmenu'));

  console.log('bwstarttest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
