// livedatatest.js — LIVE TUNING (v1.26 Item 3): the tunable registry,
// range validation, boot reapply from livedata.txt, the terminal's
// TUNE view (edit + literal substitution), and RESET STOCK clearing
// overrides. dbg_tune/dbg_tuneset drive the same apply path the
// terminal uses (values cross the seam in milli-units).
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
  // name a tunable through the scratch buffer, then read/set it
  const setName = async (name) => {
    const ptr = await call('dbg_scratch');
    await page.evaluate(([p, s]) => {
      const mem = new Uint8Array(window.__TSG.memory.buffer);
      for (let i = 0; i < s.length; i++) mem[p + i] = s.charCodeAt(i);
      mem[p + s.length] = 0;
    }, [ptr, name]);
  };
  const tune = async (name) => { await setName(name); return call('dbg_tune'); };
  const tuneset = async (name, milli) => { await setName(name); return call('dbg_tuneset', milli); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);

  // stock defaults
  T('SRC_DOOR_TROPHIES defaults to 3', (await tune('SRC_DOOR_TROPHIES')) === 3000, await tune('SRC_DOOR_TROPHIES'));
  T('WEAPON_PRICE_4 defaults to 200', (await tune('WEAPON_PRICE_4')) === 200000, await tune('WEAPON_PRICE_4'));
  T('THREAT_RATIO defaults to 0.75', (await tune('THREAT_RATIO')) === 750, await tune('THREAT_RATIO'));
  T('unknown tunable reads -1', (await tune('NO_SUCH_TUNABLE')) === -1, await tune('NO_SUCH_TUNABLE'));
  T('no overrides at stock', (await call('dbg_tunecount')) === 0, await call('dbg_tunecount'));

  // a live set applies, persists, and the gate follows it
  T('set SRC_DOOR_TROPHIES=5 accepted', (await tuneset('SRC_DOOR_TROPHIES', 5000)) === 0);
  T('value reads back 5', (await tune('SRC_DOOR_TROPHIES')) === 5000, await tune('SRC_DOOR_TROPHIES'));
  T('one override counted', (await call('dbg_tunecount')) === 1, await call('dbg_tunecount'));
  const blob = await page.evaluate(() => localStorage.getItem('tsgmod:livedata.txt'));
  T('livedata.txt persisted the override', !!blob && blob.includes('SRC_DOOR_TROPHIES=5'), blob);
  T('gate still sealed at 3 pinned trophies', (await call('dbg_givetrophy', 3)) >= 3 && (await call('dbg_srcdoor')) === 0, await call('dbg_srcdoor'));
  T('gate opens at 5 pinned trophies', (await call('dbg_givetrophy', 5)) >= 5 && (await call('dbg_srcdoor')) === 1, await call('dbg_srcdoor'));

  // range validation rejects and changes nothing
  T('out-of-range set rejected (25 > 20)', (await tuneset('SRC_DOOR_TROPHIES', 25000)) === -1);
  T('value unchanged after rejection', (await tune('SRC_DOOR_TROPHIES')) === 5000, await tune('SRC_DOOR_TROPHIES'));

  // boot reapply: reload the page, the override comes back
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  T('override reapplies at boot', (await tune('SRC_DOOR_TROPHIES')) === 5000, await tune('SRC_DOOR_TROPHIES'));

  // the real terminal TUNE view: open data/design.c, retune the gate
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  T('re-pin 5 trophies after the reload', (await call('dbg_givetrophy', 5)) >= 5, await call('dbg_givetrophy', 0));
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -210, -6180); await sleep(200);
  await key('KeyE', 300);
  T('in the Source Room (gate at 5, have 5)', (await poll('dbg_py', v => v < -17000)) < -17000, await call('dbg_py'));
  await call('dbg_tp', 0, -18100); await sleep(200);
  await key('KeyE', 300);
  T('terminal open', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));
  // browser order: data/ files come after core/ and coop/ ... walk the
  // selection down to data/design.c by watching the editor open
  const fs = require('fs'); const path = require('path');
  const root = path.join(__dirname, '..', 'src');
  const files = [];
  (function walk(d) { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) walk(p); else if (e.name.endsWith('.c')) files.push(path.relative(root, p).split(path.sep).join('/')); } })(root);
  files.sort();
  const di = files.indexOf('data/design.c');
  for (let i = 0; i < di; i++) await key('ArrowDown', 30);
  await key('Space', 300);
  let opened = await poll('dbg_srcfile', v => v === di, 4000);
  if (opened !== di) {
    await key('Escape', 300);
    const diff = di - opened, dir = diff > 0 ? 'ArrowDown' : 'ArrowUp';
    for (let i = 0; i < Math.abs(diff); i++) await key(dir, 30);
    await key('Space', 300);
    opened = await poll('dbg_srcfile', v => v === di, 4000);
  }
  T('editor opens data/design.c', opened === di, opened);
  await page.keyboard.press('Control+t');
  T('Ctrl+T opens the TUNE view', (await poll('dbg_srcterm', v => v === 5)) === 5, await call('dbg_srcterm'));
  // first tune row is SRC_DOOR_TROPHIES: clear the pending "5", type 7
  await key('Backspace', 100);
  await page.keyboard.type('7', { delay: 40 });
  await sleep(150);
  // rows: 35 tunables, then APPLY, then BACK — walk to APPLY
  for (let i = 0; i < 35; i++) await key('ArrowDown', 25);
  await key('Space', 400);
  T('APPLY writes the live value 7', (await tune('SRC_DOOR_TROPHIES')) === 7000, await tune('SRC_DOOR_TROPHIES'));
  const edited = await page.evaluate(() => localStorage.getItem('tsgmod:data/design.c'));
  T('the literal was substituted into the overlay text', !!edited && edited.includes('SRC_DOOR_TROPHIES = 7'), edited ? edited.slice(0, 80) : edited);
  T('back in the editor after APPLY', (await call('dbg_srcterm')) === 2, await call('dbg_srcterm'));

  // RESET STOCK from the browser clears overrides and restores defaults
  await key('Escape', 300);
  T('back at the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));
  await key('KeyR', 300);
  await key('ArrowUp', 200);
  await key('Space', 400);
  T('RESET STOCK restores the default 3', (await tune('SRC_DOOR_TROPHIES')) === 3000, await tune('SRC_DOOR_TROPHIES'));
  T('overrides cleared', (await call('dbg_tunecount')) === 0, await call('dbg_tunecount'));
  const blobAfter = await page.evaluate(() => localStorage.getItem('tsgmod:livedata.txt'));
  T('livedata.txt cleared', blobAfter === null, blobAfter);

  console.log('livedatatest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
