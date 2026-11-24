// v131test.js — v1.31 "EVERY BUTTON, TOO": the four behavioural
// fixes that a suite can drive end-to-end.
//   P9/H11: Settings has a tappable BACK row — a touch player can
//           leave Settings at all.
//   P6/H2:  the main game draws the BW-style START pause chip on
//           touch; tapping it opens the pause (both schemes).
//   P7/H6:  the WAR! battle pause takes arrow navigation alongside
//           keys 1/2/3: Up/Down move the selection (dbg_warpsel),
//           Enter confirms, the digits still pick directly.
//   P8/H10: Buddy Craft search from a gamepad lives in its own
//           suite (v131craftpad.js) — fake pads connected from boot
//           flip the title menu to its pad flavour and would bend
//           every row index here.
// (P1's Tank DRAG deck is covered in tanktest.js; P2/P3/P4 are copy;
// P5 is native-only and rides the Linux smoke test.)
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
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const key = async (code, ms = 160) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };   // held presses: an instant press can fall between frames and vanish (the suites' standing lesson)
  const tap = async (x, y) => { await call('touch_point', 9, x, y, 0); await sleep(90); await call('touch_point', 9, x, y, 2); await sleep(160); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(400);
  T('title menu up', (await call('dbg_state')) === 0, await call('dbg_state'));

  // ---------- P9: Settings' BACK row is tappable ----------
  await key('ArrowDown'); await key('ArrowDown'); await key('Space', 500);   // SETTINGS = row 2, keyboard flavour
  T('settings opens', (await poll('dbg_state', v => v === 6, 4000)) === 6, await call('dbg_state'));
  await call('set_mobile', 1);   // now a touch player: no Esc key exists for them
  await tap(320, 310);           // the BACK row (row 19, y 304..314): first tap selects it (the settings tap idiom)
  await tap(320, 310);           // second tap on the selected row activates it
  T('tapping BACK leaves Settings for the menu', (await poll('dbg_state', v => v === 0, 4000)) === 0, await call('dbg_state'));

  // ---------- start the run (for the pause chip + WAR) ----------
  // menu_sel still sits on SETTINGS (row 2) after the trip above —
  // walk it back to row 0 (START) by the seam, not by counting rows.
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 0) break; await key('ArrowUp', 110); }
  T('menu selection back on START', (await call('dbg_menusel')) === 0, await call('dbg_menusel'));
  await key('Space', 500);
  let st = await poll('dbg_state', v => v === 7 || v === 1, 4000);   // START lands on the slot picker or straight in
  if (st === 7) { await key('Space', 500); }
  T('run started', (await poll('dbg_state', v => v === 1, 6000)) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // ---------- P6: the main-game pause chip, both touch schemes ----------
  for (const [scheme, nm] of [[1, 'STICK'], [0, 'DRAG']]) {
    await call('dbg_touchctl', scheme);
    await tap(280, 104);   // the chip: (244,92) 72x24, centre (280,104)
    T(nm + ': tapping the START chip opens the pause', (await poll('dbg_state', v => v === 5, 4000)) === 5, await call('dbg_state'));
    await key('Escape', 300);
    T(nm + ': ESC resumes', (await poll('dbg_state', v => v === 1, 4000)) === 1, await call('dbg_state'));
  }

  // ---------- P7: WAR! battle pause — arrows alongside digits ----------
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('in the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(200);
  T('standing at the WAR! cabinet', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('WAR! panel opened', (await poll('dbg_arcgate', v => v === 1, 4000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('WAR! launched (bw_mode 8)', (await poll('dbg_bwmode', v => v === 8, 4000)) === 8, await call('dbg_bwmode'));
  await sleep(700);   // entry grace (wartest's)
  await tap(320, 73);   // region 1's node
  T('region 1 battle launched', (await poll('dbg_warstate', v => v === 1, 5000)) === 1, await call('dbg_warstate'));
  await key('Escape', 300);
  T('battle pause opens', (await poll('dbg_warpause', v => v === 1, 4000)) === 1, await call('dbg_warpause'));
  T('selection starts on RESUME (0)', (await call('dbg_warpsel')) === 0, await call('dbg_warpsel'));
  await key('ArrowDown', 250);
  T('Down moves the selection to RESTART (1)', (await poll('dbg_warpsel', v => v === 1, 3000)) === 1, await call('dbg_warpsel'));
  await key('ArrowDown', 250);
  T('Down again to SAVE & QUIT (2)', (await poll('dbg_warpsel', v => v === 2, 3000)) === 2, await call('dbg_warpsel'));
  await key('ArrowUp', 250);
  T('Up walks back to RESTART (1)', (await poll('dbg_warpsel', v => v === 1, 3000)) === 1, await call('dbg_warpsel'));
  await key('Enter', 300);
  T('Enter confirms RESTART: pause closes, battle relaunches', (await poll('dbg_warpause', v => v === 0, 4000)) === 0 && (await call('dbg_warstate')) === 1,
    'pause=' + (await call('dbg_warpause')) + ' warstate=' + (await call('dbg_warstate')));
  await key('Escape', 300);
  T('pause reopens with the selection reset', (await poll('dbg_warpause', v => v === 1, 4000)) === 1 && (await call('dbg_warpsel')) === 0,
    'pause=' + (await call('dbg_warpause')) + ' sel=' + (await call('dbg_warpsel')));
  await key('Digit1', 300);
  T('digit 1 still picks RESUME directly', (await poll('dbg_warpause', v => v === 0, 4000)) === 0, await call('dbg_warpause'));
  await key('Escape', 300);
  T('pause reopens for SAVE & QUIT', (await poll('dbg_warpause', v => v === 1, 4000)) === 1, await call('dbg_warpause'));
  await key('Digit3', 300);
  T('digit 3 still picks SAVE & QUIT (back at the world map)', (await poll('dbg_warpause', v => v === 0, 4000)) === 0 && (await poll('dbg_warstate', v => v === 0, 4000)) === 0,
    'pause=' + (await call('dbg_warpause')) + ' warstate=' + (await call('dbg_warstate')));
  await key('Escape', 300);   // out of WAR! entirely
  T('WAR! exited', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));

  console.log('v131test: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
