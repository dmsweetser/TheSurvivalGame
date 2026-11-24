// v131craftpad.js — v1.31 (audit P8/H10): Buddy Craft search starts
// from a gamepad. Before v1.31 the only ways in were the "/" key and
// a touch tap — a pad player could not search the thirty-row craft
// list at all. Now pad X starts the search; the press that opens it
// must not read as the menu's X-closes-it press in the same frame,
// and typing then filters for real.
// Its own suite because the fake pads ride in from boot (the
// audit130-pad harness), which flips the title menu to pad flavour —
// entry here is by dbg seam, like craftsearchtest.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }
const PADSCRIPT = () => {
  window.__pads = [0, 1].map(i => {
    const buttons = [];
    for (let k = 0; k < 18; k++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + i, index: i, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0], timestamp: 0 };
  });
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
};
const PX = 2;   // standard-mapping X button

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT);
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const key = async (code, ms = 160) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const btn = async (pi, bi, hold = 200, wait = 300) => { await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]); await sleep(hold); await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]); await sleep(wait); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  let st = await call('dbg_state');
  if (st === 7) { await key('Space', 500); }
  T('run started', (await poll('dbg_state', v => v === 1, 6000)) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT (mode 4)', (await poll('dbg_bwmode', v => v === 4, 4000)) === 4, await call('dbg_bwmode'));
  await key('KeyF', 250); await sleep(300);
  T('the craft menu opens', (await call('dbg_craftmenu')) === 1, await call('dbg_craftmenu'));
  T('search starts off', (await call('dbg_craftsearch')) === 0, await call('dbg_craftsearch'));
  await btn(0, PX);   // pad X: the pad's search key
  T('pad X starts the search', (await poll('dbg_craftsearch', v => v === 1, 4000)) === 1, await call('dbg_craftsearch'));
  T('...and the menu stays open (the press is not also its close)', (await call('dbg_craftmenu')) === 1, await call('dbg_craftmenu'));
  await page.keyboard.type('AXE', { delay: 90 });
  await sleep(400);
  T('typing in the pad-started search filters to the two axes', (await poll('dbg_craftrows', v => v === 2, 4000)) === 2, await call('dbg_craftrows'));
  await key('Escape', 250); await sleep(300);
  T('Escape ends the search, menu stays', (await call('dbg_craftsearch')) === 0 && (await call('dbg_craftmenu')) === 1,
    'search=' + (await call('dbg_craftsearch')) + ' menu=' + (await call('dbg_craftmenu')));
  await call('dbg_bwexit'); await sleep(300);

  console.log('v131craftpad: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
