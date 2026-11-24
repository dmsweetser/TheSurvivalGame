// audit130-pad.js — v1.30 audit, gamepad phase. Two fake standard
// pads (the e2e-pad harness). Everything driven on the pads except
// position staging (dbg_tp) and inventory staging (dbg_givebean).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const SHOT = __dirname + '/v130-shots/';
require('fs').mkdirSync(SHOT, { recursive: true });
const sleep = ms => new Promise(r => setTimeout(r, ms));
let ok = 0, find = 0;
function OK(name) { ok++; console.log('OK', name); }
function F(sev, name, extra) { find++; console.log('FINDING', sev, name, extra !== undefined ? '-> ' + String(extra).slice(0, 200) : ''); }
function T(name, cond, extra) { if (cond) OK(name); else F('DEFECT?', name, extra); }
const PADSCRIPT = () => {
  window.__pads = [0, 1].map(i => {
    const buttons = [];
    for (let k = 0; k < 18; k++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + i, index: i, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0], timestamp: 0 };
  });
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  window.__axes = (pi, x, y) => { window.__pads[pi].axes[0] = x; window.__pads[pi].axes[1] = y; };
};
const A = 0, B = 1, X = 2, Y = 3, LB = 4, RB = 5, LT = 6, BACK = 8, START = 9, DUP = 12, DDOWN = 13;

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT);
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const btn = async (pi, bi, hold = 200, wait = 260) => { await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]); await sleep(hold); await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]); await sleep(wait); };
  const axes = (pi, x, y) => page.evaluate(([p, a, b]) => window.__axes(p, a, b), [pi, x, y]);
  const shot = async n => { await page.screenshot({ path: SHOT + n }); console.log('SHOT', n); };
  const lbl = (fn, act) => page.evaluate(([f, a]) => { const ptr = window.__TSG[f](a); const mem = new Uint8Array(window.__TSG.memory.buffer); let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]); return s; }, [fn, act]);

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);

  // ---------- TITLE on pad ----------
  await btn(0, DDOWN);   // first pad input: device should flip to PAD
  T('device flips to gamepad after a pad press', (await poll('dbg_dev', v => v === 1, 3000)) === 1, await call('dbg_dev'));
  const nPad = await call('dbg_menucount');
  console.log('NOTE menucount(2 pads connected, pad flavour)=', nPad, 'sel=', await call('dbg_menusel'));
  await shot('title-pad.png');
  console.log('NOTE pad labels: fire=', await lbl('dbg_lbl', 0), 'int=', await lbl('dbg_lbl', 1), 'wall=', await lbl('dbg_lbl', 2), 'bean=', await lbl('dbg_lbl', 3));
  // dpad navigation moves the selection
  const s0 = await call('dbg_menusel');
  await btn(0, DDOWN);
  T('dpad Down moves the menu selection', (await call('dbg_menusel')) !== s0, s0 + '->' + (await call('dbg_menusel')));
  await btn(0, DUP);
  // HOW TO PLAY is row 1 in every flavour
  await btn(0, DDOWN);
  await btn(0, A, 200, 500);
  T('A opens HOW TO PLAY', (await poll('dbg_state', v => v === 4, 4000)) === 4, await call('dbg_state'));
  await shot('help-pad.png');
  await btn(0, BACK, 200, 500);
  let st = await call('dbg_state');
  if (st !== 0) { await btn(0, B, 200, 500); st = await call('dbg_state'); }
  T('help exits on pad (Back or B)', st === 0, st);
  // 2P CO-OP row: with 2 pads it is the last row (index = base count)
  const nAll = await call('dbg_menucount');
  for (let i = 0; i < 12 && (await call('dbg_menusel')) !== nAll - 1; i++) await btn(0, DDOWN, 150, 180);
  T('selection reaches the 2P CO-OP row', (await call('dbg_menusel')) === nAll - 1, await call('dbg_menusel') + ' of ' + nAll);
  await shot('title-pad-cooprow.png');
  await btn(0, A, 200, 400);
  T('A toggles 2P CO-OP on', (await call('dbg_coop')) === 1, await call('dbg_coop'));
  // back to START (row 0) and launch
  for (let i = 0; i < 12 && (await call('dbg_menusel')) !== 0; i++) await btn(0, DUP, 150, 180);
  await btn(0, A, 200, 600);
  st = await call('dbg_state');
  if (st === 7) { await btn(0, A, 200, 600); }
  T('A on START + slot starts the run', (await poll('dbg_state', v => v === 1, 6000)) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  // P2 drop-in: any P2 pad button should bring the second buddy in
  await sleep(600);
  await btn(1, A, 250, 600);
  const p2 = await call('dbg_p2');
  console.log('NOTE p2 slot after P2 A press =', p2, 'devp2=', await call('dbg_devp2'));
  T('P2 joins on pad 2', p2 >= 0, p2);

  // ---------- OVERWORLD on pad ----------
  const x0 = await call('dbg_px');
  await axes(0, 1, 0); await sleep(900); await axes(0, 0, 0); await sleep(200);
  T('left stick walks P1 east', (await call('dbg_px')) > x0 + 15, x0 + '->' + (await call('dbg_px')));
  // Y = bean
  await call('dbg_givebean'); await call('dbg_tp', 0, 900); await sleep(250);
  await btn(0, Y, 200, 300);
  T('Y fires the Backfire Bean', (await poll('dbg_beant', v => v > 0, 4000)) > 0, await call('dbg_beant'));
  await sleep(2600);
  // B = interact at the mall door
  await call('dbg_tp', 0, 80); await sleep(250);
  await btn(0, B, 250, 500);
  T('B opens the mall door', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  // B at THE COUNTER opens the shop panel; B again (interact) closes it
  await call('dbg_tp', -315, -6148); await sleep(250);
  await btn(0, B, 250, 500);
  T('B opens the Counter panel', (await poll('dbg_state', v => v === 2, 4000)) === 2 && (await call('dbg_station')) === 0,
    'state=' + (await call('dbg_state')) + ' st=' + (await call('dbg_station')));
  await shot('store-pad.png');
  await btn(0, B, 250, 500);
  T('B closes the Counter panel', (await poll('dbg_state', v => v === 1, 4000)) === 1, await call('dbg_state'));
  // START pause / START resume
  await btn(0, START, 200, 500);
  T('START opens pause', (await poll('dbg_state', v => v === 5, 4000)) === 5, await call('dbg_state'));
  await shot('pause-pad.png');
  await btn(0, START, 200, 500);
  T('START resumes', (await poll('dbg_state', v => v === 1, 4000)) === 1, await call('dbg_state'));

  // ---------- WAR! on pad: panel, launch, pause ----------
  await call('dbg_tp', 350, -5976); await sleep(250);
  await btn(0, B, 250, 500);
  T('B opens the arcade door', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(250);
  await btn(0, B, 250, 500);
  const gate = await poll('dbg_arcgate', v => v === 1, 3000);
  T('B opens the WAR! cabinet panel', gate === 1, gate);
  await shot('war-panel-pad.png');
  await btn(0, A, 250, 600);
  T('A launches WAR!', (await poll('dbg_bwmode', v => v === 8, 5000)) === 8, await call('dbg_bwmode'));
  await sleep(800);
  await btn(0, START, 200, 600);
  const wp = await call('dbg_warpause');
  console.log('NOTE war_pause after START =', wp);
  T('START opens the WAR! pause menu', wp === 1, wp);
  await shot('war-pause-pad.png');
  await btn(0, B, 250, 600);
  T('B resumes from the WAR! pause', (await call('dbg_warpause')) === 0, await call('dbg_warpause'));
  await btn(0, BACK, 200, 600);
  const leftWar = (await call('dbg_bwmode')) === -1;
  console.log('NOTE bwmode after Back in WAR! =', await call('dbg_bwmode'), 'inarcade=', await call('dbg_inarcade'));
  if (!leftWar) { await call('dbg_bwexit'); await sleep(400); }

  // ---------- MORDOR on pad: panel rows + resume ----------
  await call('dbg_tp', -256, -11758); await sleep(250);
  await btn(0, B, 250, 500);
  const mgate = await poll('dbg_arcgate', v => v === 1, 3000);
  T('B opens the Mordor cabinet panel', mgate === 1, mgate);
  await btn(0, A, 250, 600);
  T('A launches MORDOR FUN RUN', (await poll('dbg_bwmode', v => v === 9, 5000)) === 9, await call('dbg_bwmode'));
  // run north a little on the stick so the run has distance, then leave via Back
  await axes(0, 0, -1); await sleep(1500); await axes(0, 0, 0); await sleep(200);
  await btn(0, BACK, 200, 700);
  const leftMor = (await call('dbg_bwmode')) === -1;
  console.log('NOTE bwmode after Back in Mordor =', await call('dbg_bwmode'));
  if (!leftMor) { await call('dbg_bwexit'); await sleep(400); }
  // re-enter: the entry panel should offer RESUME; A on row 0 resumes
  await call('dbg_tp', -256, -11758); await sleep(250);
  await btn(0, B, 250, 600);
  await sleep(400);
  await shot('mordor-panel-pad.png');
  await btn(0, A, 250, 700);
  const resumed = await poll('dbg_bwmode', v => v === 9, 5000);
  T('Mordor entry panel resumes the parked run with A', resumed === 9, await call('dbg_bwmode'));
  await btn(0, BACK, 200, 600);
  if ((await call('dbg_bwmode')) !== -1) { await call('dbg_bwexit'); await sleep(300); }

  console.log('AUDIT-PAD DONE: ok=' + ok + ' findings=' + find);
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
