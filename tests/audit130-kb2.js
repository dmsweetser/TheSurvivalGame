// audit130-kb2.js — v1.30 audit, keyboard phase 2: settings + name
// field (selection tracked via dbg_menusel), every Mall station panel,
// and the Source Room (door, terminal browser, editor, CTRL+T tuning).
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

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const shot = async n => { await page.screenshot({ path: SHOT + n }); console.log('SHOT', n); };
  const str = fn => page.evaluate(f => { const ptr = window.__TSG[f](); const mem = new Uint8Array(window.__TSG.memory.buffer); let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]); return s; }, fn);
  const selTo = async target => { // walk menu_sel to target with Down presses (wraps)
    for (let i = 0; i < 12; i++) { if ((await call('dbg_menusel')) === target) return true; await key('ArrowDown', 110, 130); }
    return (await call('dbg_menusel')) === target;
  };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(500);
  const n = await call('dbg_menucount');   // 6, kb flavour, no save
  console.log('NOTE menucount=', n, 'sel=', await call('dbg_menusel'));

  // SETTINGS is row 2 (kb flavour)
  T('walk selection to SETTINGS row', await selTo(2), await call('dbg_menusel'));
  await key('Space', 200, 500);
  T('settings opens', (await poll('dbg_state', v => v === 6, 4000)) === 6, await call('dbg_state'));
  await shot('settings-kb.png');
  await key('ArrowDown'); await key('ArrowDown'); await key('ArrowDown');
  await shot('settings-kb-nav.png');
  await key('Escape', 200, 400);
  T('ESC leaves settings', (await poll('dbg_state', v => v === 0, 4000)) === 0, await call('dbg_state'));

  // NAME field is row n
  T('walk selection to NAME field', await selTo(n), await call('dbg_menusel'));
  await page.keyboard.type('AUDIT', { delay: 70 }); await sleep(300);
  const nm = await str('dbg_myname');
  T('typed letters land in the name', /AUDIT/.test(nm), nm);
  await shot('title-name-kb.png');
  // backspace edits
  await key('Backspace', 150, 200); await key('Backspace', 150, 200);
  const nm2 = await str('dbg_myname');
  T('backspace edits the name', nm2.length === nm.length - 2, nm + ' -> ' + nm2);

  // start the run
  T('walk selection back to START', await selTo(0), await call('dbg_menusel'));
  await key('Space', 200, 500);
  if ((await call('dbg_state')) === 7) await key('Space', 200, 600);
  T('run started', (await poll('dbg_state', v => v === 1, 6000)) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  // ---------- MALL stations ----------
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1, 4000)) === 1, await call('dbg_mall'));
  const ST = [
    [0, 'THE COUNTER', -315, -6148], [1, 'WHALES ONLY', -105, -6148], [2, 'BUG BANK', 105, -6148],
    [3, 'BANZAI BUDDIES', 315, -6148], [4, 'BUG BOWL', -180, -5868], [5, 'TROPHY CASE', 340, -5868],
    [6, 'CHALLENGE BOARD', -330, -5868], [7, 'BOUNTY BOARD', -366, -5992], [8, 'PHOTO BOOTH', -232, -5980],
  ];
  for (const [sid, nm2, sx, sy] of ST) {
    await call('dbg_tp', sx, sy); await sleep(250);
    await key('KeyE', 300);
    const opened = await poll('dbg_state', v => v === 2, 3000);
    const stNow = await call('dbg_station');
    if (opened === 2 && stNow === sid) {
      OK(nm2 + ' panel opens (station ' + sid + ')');
      if (sid === 0) await shot('store-kb.png');
      if (sid === 1) await shot('whales-kb.png');
      if (sid === 4) await shot('bowl-kb.png');
      if (sid === 5) await shot('trophycase-kb.png');
      await key('ArrowDown'); await key('ArrowUp');   // navigation exists
      await key('Escape', 200, 450);
      T(nm2 + ' panel closes with ESC', (await poll('dbg_state', v => v === 1, 3000)) === 1, await call('dbg_state'));
    } else {
      F('DEFECT?', nm2 + ' panel did not open with E', 'state=' + opened + ' station=' + stNow);
      if ((await call('dbg_state')) === 2) await key('Escape', 200, 400);
    }
  }
  // Banzai gate panel (STORY / FREE PLAY / LEAVE)
  await call('dbg_tp', 215, -5868); await sleep(250);
  await key('KeyE', 300);
  await shot('gate-panel-kb.png');
  await key('Escape', 200, 400);
  console.log('NOTE gate panel closed, state=', await call('dbg_state'), 'bwmode=', await call('dbg_bwmode'));

  // ---------- SOURCE ROOM ----------
  T('givetrophy(3) unlocks the door', (await call('dbg_givetrophy', 3)) >= 3 && (await poll('dbg_srcdoor', v => v === 1, 3000)) === 1,
    await call('dbg_srcdoor'));
  await call('dbg_tp', -210, -6180); await sleep(250);
  await key('KeyE', 300);
  T('entered the Source Room', (await poll('dbg_py', v => v < -17000, 4000)) < -17000, await call('dbg_py'));
  await call('dbg_tp', 0, -18100); await sleep(250);
  await key('KeyE', 300);
  T('terminal browser opens', (await poll('dbg_srcterm', v => v === 1, 4000)) === 1, await call('dbg_srcterm'));
  await shot('src-browser-kb.png');
  // open the first file with tunables: walk down a few and open; try design.c via many downs is slow —
  // open file index 2 first (browser order is the blob order); check the editor footer for CTRL+T.
  await key('ArrowDown', 60, 90); await key('ArrowDown', 60, 90);
  await key('Space', 300);
  T('editor opens', (await poll('dbg_srcterm', v => v === 2, 4000)) === 2, await call('dbg_srcterm'));
  await shot('src-editor-kb.png');
  // CTRL+T tuning (only on files with tunables; try, observe)
  await page.keyboard.down('Control'); await key('KeyT', 120, 300); await page.keyboard.up('Control'); await sleep(300);
  const tv = await call('dbg_srcterm');
  console.log('NOTE srcterm view after CTRL+T =', tv, '(5=tune)');
  if (tv === 5) { await shot('src-tune-kb.png'); await key('Escape', 200, 400); }
  await key('Escape', 200, 400);
  if ((await call('dbg_srcterm')) === 2) await key('Escape', 200, 400);
  T('terminal closes back to the room', (await poll('dbg_srcterm', v => v === 0, 3000)) === 0, await call('dbg_srcterm'));
  // leave the Source Room by its exit (walk south / E at the door)
  await call('dbg_tp', 0, -17815); await sleep(250);
  await key('KeyE', 300);
  console.log('NOTE after src exit attempt: py=', await call('dbg_py'), 'mall=', await call('dbg_mall'));

  console.log('AUDIT-KB2 DONE: ok=' + ok + ' findings=' + find);
  await browser.close();
  process.exit(0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
