// taptest.js — the gate panel vs touch, and the phantom station.
// Daniel (Nov 4, mobile): taps on the gate's STORY MODE / FREE PLAY
// rows did nothing, and the game "pulled things out of the bank" —
// the menus were mixed up. Root cause (found in the frame code): the
// solo frame's store-input branch is the ELSE of "playing && buddy
// not in BW && gate panel shut", so with the gate panel OPEN it runs
// station_input against the STALE G.store_station — taps and even
// the Space that picks STORY MODE land on the last shop you visited.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
const CY = -6000;
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const tap = async (x, y) => { await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [x, y]); await sleep(350); };
  async function poll(fn, want, timeout = 6000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(180); }
    return call(fn);
  }
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300); await sleep(300);
  T('in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));

  async function stageBankSelection() {
    for (let i = 0; i < 4; i++) await call('dbg_feed');            // +40 kg
    await call('dbg_tp', 75, CY - 50); await sleep(200);
    await key('KeyE', 300); await sleep(300);                      // bank panel
    T('bank panel open', (await call('dbg_station')) === 2, await call('dbg_station'));
    await key('Space', 350); await sleep(300);                     // row 0: DEPOSIT ALL
    await key('Escape', 300); await sleep(300);
    await key('KeyE', 300); await sleep(300);                      // reopen
    await key('ArrowDown', 150); await key('ArrowDown', 150); await key('ArrowDown', 150); await sleep(200); // WITHDRAW ALL selected
    await key('Escape', 300); await sleep(300);                    // close WITHOUT buying: stale station 2, sel 3
  }
  async function toGatePanel() {
    await call('dbg_tp', 185, CY + 96); await sleep(200);
    await key('KeyE', 300); await sleep(350);
    return (await call('dbg_gate')) === 1;
  }

  // ---- phase 1: the Space that picks STORY MODE must not touch the bank
  await stageBankSelection();
  T('bank holds 40', (await call('dbg_bank')) === 40, await call('dbg_bank'));
  T('gate panel open (phase 1)', await toGatePanel());
  await key('Space', 300); await sleep(400);                       // pick STORY MODE
  T('buddy entered Banzai World', (await poll('dbg_bwstate', v => v === 1)) === 1, await call('dbg_bwstate'));
  T('bank untouched by the gate pick', (await call('dbg_bank')) === 40, await call('dbg_bank'));
  T('nothing landed on the hat either', (await call('dbg_carried')) === 0, await call('dbg_carried'));
  await key('Escape', 300); await sleep(400);                      // leave BW

  // ---- phase 2: taps on the gate panel --------------------------------
  await stageBankSelection();
  T('gate panel open (phase 2)', await toGatePanel());
  const bankBeforeTap = await call('dbg_bank');
  await tap(320, 132);                                             // the STORY MODE row, centre panel (panel grew to 6 rows, h=210)
  T('a tap on STORY MODE enters Banzai World', (await poll('dbg_bwstate', v => v === 1, 3000)) === 1, await call('dbg_bwstate'));
  T('bank untouched by the tap', (await call('dbg_bank')) === bankBeforeTap, await call('dbg_bank'));
  if ((await call('dbg_bwstate')) === 1) { await key('Escape', 300); await sleep(400); }

  // ---- phase 3: tap FREE PLAY, then tap LEAVE --------------------------
  T('gate panel open (phase 3)', await toGatePanel());
  await tap(320, 152);                                             // the FREE PLAY row
  T('a tap on FREE PLAY enters Banzai World', (await poll('dbg_bwstate', v => v === 1, 3000)) === 1, await call('dbg_bwstate'));
  await key('Escape', 300); await sleep(400);
  T('gate panel open (phase 4)', await toGatePanel());
  await tap(320, 232);                                             // the LEAVE row (row 5 now)
  T('a tap on LEAVE closes the panel', (await poll('dbg_gate', v => v === 0, 3000)) === 0, await call('dbg_gate'));
  T('and does not enter Banzai World', (await call('dbg_bwstate')) === 0, await call('dbg_bwstate'));

  console.log(`taptest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
