// srctermtest.js — THE SOURCE ROOM (v1.26 Item 1): the trophy-gated
// Mall door, the source terminal (browse / edit / persist), the web
// BUILD message, and RESET STOCK. Drives the real player path (walk-in
// via Interact, keyboard through the terminal); dbg seams observe.
const { chromium } = require('playwright-core');
const fs = require('fs');
const path = require('path');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

// the blob's file list, exactly as build.py sorts it (POSIX relative
// paths, code-unit order — the same order both sorters produce)
function srcFiles() {
  const root = path.join(__dirname, '..', 'src');
  const out = [];
  (function walk(d) {
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p);
      else if (e.name.endsWith('.c')) out.push(path.relative(root, p).split(path.sep).join('/'));
    }
  })(root);
  return out.sort();
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120) => { await page.keyboard.down(code); await sleep(140); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  // the gate: sealed at 0 trophies
  T('source door sealed at 0 trophies', (await call('dbg_srcdoor')) === 0, await call('dbg_srcdoor'));
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -210, -6180); await sleep(200);
  await key('KeyE', 300);
  T('sealed door refuses entry', (await call('dbg_py')) > -7000, await call('dbg_py'));

  // grant 3 trophies -> the door opens
  T('dbg_givetrophy(3) earns at least 3', (await call('dbg_givetrophy', 3)) >= 3, await call('dbg_givetrophy', 0));
  T('source door unlocked at 3 trophies', (await poll('dbg_srcdoor', v => v === 1)) === 1, await call('dbg_srcdoor'));
  await key('KeyE', 300);
  T('entered the Source Room', (await poll('dbg_py', v => v < -17000)) < -17000, await call('dbg_py'));

  // open the terminal
  await call('dbg_tp', 0, -18100); await sleep(200);
  await key('KeyE', 300);
  T('terminal opens to the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));

  // browse to game/update.c and open it
  const files = srcFiles();
  const target = files.indexOf('game/update.c');
  T('target file is in the blob list', target > 0, target);
  for (let i = 0; i < target; i++) await key('ArrowDown', 30);
  await key('Space', 300);
  let opened = await poll('dbg_srcfile', v => v === target, 4000);
  if (opened !== target) {   // selection drifted: correct from the row we landed on
    await key('Escape', 300);
    const diff = target - opened;
    const dir = diff > 0 ? 'ArrowDown' : 'ArrowUp';
    for (let i = 0; i < Math.abs(diff); i++) await key(dir, 30);
    await key('Space', 300);
    opened = await poll('dbg_srcfile', v => v === target, 4000);
  }
  T('editor opens game/update.c', opened === target, opened);
  T('terminal is in the editor view', (await call('dbg_srcterm')) === 2, await call('dbg_srcterm'));

  // type an edit and SAVE with Ctrl+S
  await key('Enter', 100);
  await page.keyboard.type('// SRCTERMTEST MARKER', { delay: 30 });
  await sleep(200);
  await page.keyboard.press('Control+s');
  await sleep(400);
  T('save registers one edited file', (await poll('dbg_srcmod', v => v === 1)) === 1, await call('dbg_srcmod'));
  const saved = await page.evaluate(() => localStorage.getItem('tsgmod:game/update.c'));
  T('the edit persisted to localStorage', !!saved && saved.includes('SRCTERMTEST MARKER'), saved ? saved.slice(0, 60) : saved);

  // reload the page: the overlay survives (web persistence)
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  T('edit still there after a page reload', (await poll('dbg_srcmod', v => v === 1, 4000)) === 1, await call('dbg_srcmod'));

  // back in through the real door; BUILD on web explains the host app
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  await call('dbg_tp', -210, -6180); await sleep(200);
  await key('KeyE', 300);
  T('re-entered the Source Room', (await poll('dbg_py', v => v < -17000)) < -17000, await call('dbg_py'));
  await call('dbg_tp', 0, -18100); await sleep(200);
  await key('KeyE', 300);
  T('terminal reopens', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));
  await key('KeyB', 300);   // the browser's BUILD shortcut
  T('BUILD view opens (host-app message on web)', (await poll('dbg_srcterm', v => v === 4)) === 4, await call('dbg_srcterm'));
  await key('Escape', 300);
  T('Esc returns to the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));

  // RESET STOCK via its confirm prompt
  await key('KeyR', 300);
  T('reset asks first (prompt view)', (await poll('dbg_srcterm', v => v === 3)) === 3, await call('dbg_srcterm'));
  await key('ArrowUp', 200);   // KEEP is the default; move to RESET EVERYTHING
  await key('Space', 400);
  T('RESET STOCK clears the edited count', (await poll('dbg_srcmod', v => v === 0)) === 0, await call('dbg_srcmod'));
  const after = await page.evaluate(() => localStorage.getItem('tsgmod:game/update.c'));
  T('the localStorage overlay is gone', after === null, after);
  T('back at the browser after reset', (await call('dbg_srcterm')) === 1, await call('dbg_srcterm'));

  // close the panel and leave by the exit mat
  await key('Escape', 300);
  T('panel closed', (await poll('dbg_srcterm', v => v === 0)) === 0, await call('dbg_srcterm'));
  await call('dbg_tp', 0, -17815); await sleep(200);
  await key('KeyE', 300);
  T('exit mat returns to the Mall', (await poll('dbg_py', v => v > -7000)) > -7000, await call('dbg_py'));

  console.log('srctermtest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
