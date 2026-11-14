// ctltest.js — CONTROLS in the menus (Nov 6): the pause menu gains a
// CONTROLS row; inside Banzai World, Start/P opens RESUME / CONTROLS /
// LEAVE instead of dumping you out; and the bean rides the pad's Y.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  // a fake gamepad from before boot: standard mapping, all buttons up
  await page.addInitScript(() => {
    const buttons = Array.from({ length: 18 }, () => ({ pressed: false, touched: false, value: 0 }));
    const pad = { id: 'Fake Pad', connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    navigator.getGamepads = () => [pad];
    window.__fakepad = pad;
  });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 130) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 6000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));

  // ---- the main pause menu: CONTINUE / CONTROLS / SAVE + QUIT ----
  await key('KeyP', 300);
  T('P opens the pause menu', (await poll('dbg_state', v => v === 5)) === 5, await call('dbg_state'));
  T('selection starts on CONTINUE', (await call('dbg_pausesel')) === 0, await call('dbg_pausesel'));
  await key('ArrowDown', 200);
  T('down lands on CONTROLS', (await call('dbg_pausesel')) === 1, await call('dbg_pausesel'));
  await key('ArrowDown', 200);
  T('down again lands on SAVE + QUIT', (await call('dbg_pausesel')) === 2, await call('dbg_pausesel'));
  await key('ArrowUp', 200);
  T('up returns to CONTROLS', (await call('dbg_pausesel')) === 1, await call('dbg_pausesel'));
  await key('Space', 250);
  T('CONTROLS panel opens', (await poll('dbg_pausectl', v => v === 1)) === 1, await call('dbg_pausectl'));
  T('still paused behind it', (await call('dbg_state')) === 5, await call('dbg_state'));
  await page.screenshot({ path: __dirname + '/ctl-main.png' });
  await key('Escape', 250);
  T('Escape backs out to the pause rows', (await call('dbg_pausectl')) === 0 && (await call('dbg_state')) === 5,
    'ctl=' + (await call('dbg_pausectl')) + ' state=' + (await call('dbg_state')));
  await key('Escape', 250);
  T('Escape again resumes the run', (await poll('dbg_state', v => v === 1)) === 1, await call('dbg_state'));

  // ---- inside Banzai World: Start/P opens the BW menu ----
  await call('dbg_adopt', 0);
  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  await key('KeyP', 300);
  T('P opens the BW menu (does NOT leave)', (await poll('dbg_bwmenu', v => v === 1)) === 1 && (await call('dbg_bwmode')) === 4,
    'menu=' + (await call('dbg_bwmenu')) + ' mode=' + (await call('dbg_bwmode')));
  await page.screenshot({ path: __dirname + '/ctl-bwmenu.png' });
  await key('ArrowDown', 200);   // CONTROLS
  await key('Space', 250);
  T('BW CONTROLS panel opens', (await poll('dbg_bwmenu', v => v === 2)) === 2, await call('dbg_bwmenu'));
  await page.screenshot({ path: __dirname + '/ctl-craft.png' });
  await key('Escape', 250);
  T('Escape backs out to the BW menu', (await call('dbg_bwmenu')) === 1, await call('dbg_bwmenu'));
  await key('Escape', 250);
  T('Escape again resumes the minigame', (await poll('dbg_bwmenu', v => v === 0)) === 0 && (await call('dbg_bwmode')) === 4,
    'menu=' + (await call('dbg_bwmenu')) + ' mode=' + (await call('dbg_bwmode')));
  await key('KeyP', 300);
  T('menu reopens', (await poll('dbg_bwmenu', v => v === 1)) === 1, await call('dbg_bwmenu'));
  await key('ArrowDown', 180); await key('ArrowDown', 180);   // LEAVE
  await key('Space', 300);
  T('LEAVE exits to the mall', (await poll('dbg_bwmode', v => v === -1)) === -1 && (await call('dbg_bwmenu')) === 0,
    'mode=' + (await call('dbg_bwmode')) + ' menu=' + (await call('dbg_bwmenu')));

  // ---- the bean on the gamepad's Y button ----
  // (leave the mall properly first: LEAVE left him at the gate indoors,
  // and the bean refuses indoors — walk out the exit mat, THEN teleport)
  await call('dbg_tp', 0, -5850); await sleep(250);
  await key('KeyE', 300); await sleep(300);
  await call('dbg_tp', 3000, 3000); await sleep(250);
  await call('dbg_beans', 1);
  await page.evaluate(() => { window.__fakepad.buttons[3].pressed = true; });
  await sleep(350);
  await page.evaluate(() => { window.__fakepad.buttons[3].pressed = false; });
  T('Y button fires the bean', (await poll('dbg_beant', v => v > 0, 4000)) > 0, await call('dbg_beant'));
  await sleep(2200);
  const px = await call('dbg_px'), py = await call('dbg_py');
  T('bean brought him home', Math.abs(px) < 40 && Math.abs(py - 70) < 40, px + ',' + py);
  T('bean spent', (await call('dbg_beans', -1)) === 0, await call('dbg_beans', -1));

  console.log('ctltest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
