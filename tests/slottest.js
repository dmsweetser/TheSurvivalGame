// slottest.js — the slot picker's buddy line must be the run's REAL
// buddy. peek_slots once walked one field short of the sum tail and
// read ZEN as the name index and the name index as the species, so
// the picker listed pets the player does not have (Daniel, Nov 4).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 120) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => {
    localStorage.clear();
    // slot 1: current format, buddy name_idx 17, species 4 (platypus)
    localStorage.setItem('tsg_save1', 'TSG3\nsum= DANIEL 120 40 300 0 17 4\nrun=1\n');
    // slot 2: current format, no buddy at all
    localStorage.setItem('tsg_save2', 'TSG3\nsum= DEV 0 0 12 0 -1 -1\nrun=1\n');
    // slot 3: OLD format (no buddy tail), a zen run — the phantom trap
    localStorage.setItem('tsg_save3', 'TSG3\nsum= OLDDEV 55 9 77 1\nrun=1\n');
  });
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await call('dbg_peek');
  T('slot 1 shows species 4', (await call('dbg_pkbuddy', 1)) === 4, await call('dbg_pkbuddy', 1));
  T('slot 1 shows name index 17', (await call('dbg_pkbname', 1)) === 17, await call('dbg_pkbname', 1));
  T('slot 2 (no buddy) shows none', (await call('dbg_pkbuddy', 2)) === -1, await call('dbg_pkbuddy', 2));
  T('slot 2 (no buddy) shows no name', (await call('dbg_pkbname', 2)) === -1, await call('dbg_pkbname', 2));
  T('slot 3 (old format) shows NO phantom pet', (await call('dbg_pkbuddy', 3)) === -1, await call('dbg_pkbuddy', 3));
  T('slot 3 (old format) shows no phantom name', (await call('dbg_pkbname', 3)) === -1, await call('dbg_pkbname', 3));
  console.log(`slottest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
