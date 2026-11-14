// padui.js — gamepads in use hide the other controls: the title menu
// drops its SETTINGS/CONTROLS row, difficulty/zen stay reachable by
// pad, HOW TO PLAY speaks gamepad, and unplugging brings it all back.
const { chromium } = require('playwright-core');
const URL = 'http://127.0.0.1:8903/the-survival-game.html';
let fails = 0;
function ok(name, cond, extra) { console.log((cond ? 'PASS ' : 'FAIL ') + name + (extra !== undefined ? '  [' + extra + ']' : '')); if (!cond) fails++; }
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 200)));
  await page.addInitScript(() => {
    const mkpad = (idx) => ({ index: idx, id: 'Fake Pad', connected: true, mapping: 'standard',
      axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })) });
    window.__pads = [];
    navigator.getGamepads = () => window.__pads;
    window.__mkpad = mkpad;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_menun, null, { timeout: 20000 });
  
  const ev = (fn, arg) => page.evaluate(fn, arg);
  // holds must span several shell polls — a sub-poll tap vanishes
  const tap = async (bi) => { await ev(b => window.__btn(0, b, true), bi); await sleep(260); await ev(b => window.__btn(0, b, false), bi); await sleep(240); };

  ok('no pads: menu has 6 rows (SETTINGS present)', await ev(() => window.__TSG.dbg_menun()) === 6, await ev(() => window.__TSG.dbg_menun()));
  await ev(() => { window.__pads = [window.__mkpad(0)]; });
  await page.waitForFunction(() => window.__TSG.dbg_pads() === 1, null, { timeout: 5000 });
  ok('one pad: menu drops to 5 rows (SETTINGS hidden)', await ev(() => window.__TSG.dbg_menun()) === 5, await ev(() => window.__TSG.dbg_menun()));
  await page.screenshot({ path: 'padui-menu.png' });

  // pad-navigate: START(0) -> HOW(1) -> DIFFICULTY(2) -> flip it
  // (dbg_mdiff/dbg_mzen read the MENU settings; dbg_diff/dbg_zen read run state)
  const d0 = await ev(() => window.__TSG.dbg_mdiff());
  await tap(13); await tap(13); await tap(0);
  ok('DIFFICULTY row still activates by pad', await ev(() => window.__TSG.dbg_mdiff()) === ((d0 + 1) & 3), await ev(() => window.__TSG.dbg_mdiff()));
  // DIFFICULTY(2) -> ZEN(3) -> flip it
  const z0 = await ev(() => window.__TSG.dbg_mzen());
  await tap(13); await tap(0);
  ok('ZEN row still activates by pad', await ev(() => window.__TSG.dbg_mzen()) === (z0 ^ 1), await ev(() => window.__TSG.dbg_mzen()));
  // back up to HOW TO PLAY (sel 3 -> 1) and open it: pad lines should show
  await tap(12); await tap(12); await tap(0);
  await sleep(300);
  ok('HOW TO PLAY opens by pad', await ev(() => window.__TSG.dbg_state()) === 4, await ev(() => window.__TSG.dbg_state()));
  await page.screenshot({ path: 'padui-help.png' });
  await tap(1);   // B: page 2
  await tap(1);   // B again: back to the menu

  // unplug: the SETTINGS row returns
  await ev(() => { window.__pads = []; });
  await page.waitForFunction(() => window.__TSG.dbg_pads() === 0, null, { timeout: 5000 });
  await sleep(200);
  ok('unplugged: menu back to 6 rows', await ev(() => window.__TSG.dbg_menun()) === 6, await ev(() => window.__TSG.dbg_menun()));

  console.log(fails === 0 ? 'ALL PASS' : fails + ' FAILURES');
  await browser.close();
  process.exit(fails ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
