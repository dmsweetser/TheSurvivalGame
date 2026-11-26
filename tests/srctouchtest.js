// srctouchtest.js — THE SOURCE ROOM on touch (v1.27 Item 2): a
// touch session can swipe-scroll the file list and the editor, tap
// to open and to position the cursor, raise the soft keyboard
// (text_mode 5), type through the shell's keyboard bridge (spaces
// included), and save via the editor's touch footer and the prompt.
// Fingers are driven through the shell's own touch_point export —
// the exact channel its touch listeners forward to — and typing
// goes through the real #kb field the bridge reconciles from.
const { chromium } = require('playwright-core');
const fs = require('fs');
const path = require('path');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

// the blob's file list, exactly as build.py sorts it (see srctermtest)
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
// the browser's row table, replicated from srct_build_rows: a folder
// header (-1) whenever the folder changes, then the file index;
// BUILD / RESET / CLOSE trail as -2 / -3 / -4
function browserRows(files) {
  const rows = []; let last = '';
  for (let i = 0; i < files.length; i++) {
    const sl = files[i].lastIndexOf('/');
    const folder = sl >= 0 ? files[i].slice(0, sl) : '';
    if (folder !== last) { last = folder; rows.push(-1); }
    rows.push(i);
  }
  rows.push(-2, -3, -4);
  return rows;
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 }, hasTouch: true, isMobile: true });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120) => { await page.keyboard.down(code); await sleep(140); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  // one finger through the shell's touch channel (fb coords)
  const tpoint = (id, x, y, ph) => page.evaluate(([i, a, b, p]) => window.__TSG.touch_point(i, a, b, p), [id, x, y, ph]);
  const ftap = async (x, y) => { await tpoint(7, x, y, 0); await sleep(90); await tpoint(7, x, y, 2); await sleep(200); };
  const fdrag = async (x0, y0, x1, y1) => {
    await tpoint(7, x0, y0, 0); await sleep(80);
    for (let k = 1; k <= 6; k++) { await tpoint(7, Math.round(x0 + (x1 - x0) * k / 6), Math.round(y0 + (y1 - y0) * k / 6), 1); await sleep(60); }
    await tpoint(7, x1, y1, 2); await sleep(200);
  };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('dbg_givetrophy(3) earns at least 3', (await call('dbg_givetrophy', 3)) >= 3, await call('dbg_givetrophy', 0));
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', -210, -6180); await sleep(200);
  await key('KeyE', 300);
  T('entered the Source Room', (await poll('dbg_py', v => v < -17000)) < -17000, await call('dbg_py'));
  await call('dbg_tp', 0, -18100); await sleep(200);
  await key('KeyE', 300);
  T('terminal opens to the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));

  // swipe up: the file list scrolls (selection/scroll packed seam)
  const browse0 = await call('dbg_srcbrowse');
  await fdrag(320, 200, 320, 60);
  const browse1 = await call('dbg_srcbrowse');
  T('drag scrolls the file list', browse1 > browse0, browse0 + ' -> ' + browse1);

  // tap the selected row (already selected = activates): editor opens
  const files = srcFiles();
  const rows = browserRows(files);
  const bsel = Math.floor(browse1 / 10000), bscroll = browse1 % 10000;
  const fileIdx = rows[bsel];
  T('the dragged-to row is a file', fileIdx >= 0, 'bsel=' + bsel + ' row=' + fileIdx);
  const ry = 98 + (bsel - bscroll) * 34;   // v1.36 metrics: rows at panel y 4 + body 78 + vi*34, tapped mid-glyph
  await ftap(320, ry);
  T('tap opens the editor', (await poll('dbg_srcterm', v => v === 2)) === 2, await call('dbg_srcterm'));
  T('the tapped file is the open file', (await call('dbg_srcfile')) === fileIdx, await call('dbg_srcfile'));
  T('a touch session may edit', (await call('dbg_srccanedit')) === 1, await call('dbg_srccanedit'));
  T('soft keyboard bridge is live (text_mode 5)', (await page.evaluate(() => window.__TSG.text_mode())) === 5, await page.evaluate(() => window.__TSG.text_mode()));
  T('the bridge field holds focus', (await page.evaluate(() => document.activeElement && document.activeElement.id)) === 'kb', await page.evaluate(() => document.activeElement && document.activeElement.id));

  // tap-to-position: fb (120,130) is display row 1, display col 2
  // of the text at panel (70,4) + text origin (10,78) — v1.36's
  // scale-2 cells are 16 wide, 32 tall; the expected cursor is
  // derived from the file's own bytes
  const fbuf = fs.readFileSync(path.join(__dirname, '..', 'src', files[fileIdx]));
  const flines = [[]];
  for (const b of fbuf) { if (b === 10) flines.push([]); else flines[flines.length - 1].push(b); }
  let expLine = 0, expCol = 0;
  { let d = 0;
    for (let l = 0; l < flines.length; l++) {
      const segs = flines[l].length === 0 ? 1 : Math.ceil(flines[l].length / 30);
      if (1 < d + segs) { expLine = l; expCol = Math.min((1 - d) * 30 + 2, flines[l].length); break; }
      d += segs;
    } }
  await ftap(120, 130);
  T('tap positions the cursor through the wrap mapping', (await call('dbg_srccur')) === expLine * 100000 + expCol, (await call('dbg_srccur')) + ' vs ' + (expLine * 100000 + expCol));

  // type through the bridge — the space must survive (mode 5)
  await page.keyboard.type('ZZ TOP', { delay: 50 });
  await sleep(400);
  // the footer's NEW LINE zone, then its SAVE zone
  await ftap(257, 331);   // [ NEW LINE ] — panel quarters at (70,310): zone 1 centre (v1.36: the footer band grew with the font)
  await ftap(132, 331);   // [ SAVE ]
  T('footer SAVE registers one edited file', (await poll('dbg_srcmod', v => v === 1)) === 1, await call('dbg_srcmod'));
  const saved = await page.evaluate(k => localStorage.getItem(k), 'tsgmod:' + files[fileIdx]);
  T('the typed edit persisted, space included', !!saved && saved.includes('ZZ TOP'), saved ? saved.slice(0, 80) : saved);
  T('the NEW LINE tap landed in the text', !!saved && saved.includes('ZZ TOP\n'), saved ? JSON.stringify(saved.slice(0, 80)) : saved);

  // drag-scroll the editor (display rows), then BACK to the browser
  const top0 = await call('dbg_srctop');
  await fdrag(320, 200, 320, 80);
  T('drag scrolls the editor', (await call('dbg_srctop')) > top0, top0 + ' -> ' + (await call('dbg_srctop')));
  await ftap(507, 331);   // [ BACK ]
  T('footer BACK returns to the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));

  // the prompt is tappable too: dirty the file, BACK into the prompt,
  // tap SAVE AND CLOSE (the prompt box is centred: row 0 at y 145+)
  const browse2 = await call('dbg_srcbrowse');
  const bsel2 = Math.floor(browse2 / 10000), bscroll2 = browse2 % 10000;
  await ftap(320, 98 + (bsel2 - bscroll2) * 34);
  T('editor reopens for the prompt check', (await poll('dbg_srcterm', v => v === 2)) === 2, await call('dbg_srcterm'));
  await page.keyboard.type('Q', { delay: 50 });
  await sleep(300);
  await ftap(507, 331);   // [ BACK ] with unsaved edits -> the prompt
  T('BACK on a dirty file asks first (prompt view)', (await poll('dbg_srcterm', v => v === 3)) === 3, await call('dbg_srcterm'));
  await ftap(320, 161);   // SAVE AND CLOSE FILE (preselected row)
  T('prompt tap saves and closes to the browser', (await poll('dbg_srcterm', v => v === 1)) === 1, await call('dbg_srcterm'));
  const saved2 = await page.evaluate(k => localStorage.getItem(k), 'tsgmod:' + files[fileIdx]);
  // the Q was typed at offset 0 of the reopened editor, so the new
  // overlay is exactly 'Q' + the previous one — no weaker check
  T('the prompt-saved edit persisted too', !!saved2 && saved2.length === saved.length + 1 && saved2[0] === 'Q' && saved2.slice(1) === saved, saved2 ? saved2.slice(0, 80) : saved2);

  console.log('srctouchtest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
