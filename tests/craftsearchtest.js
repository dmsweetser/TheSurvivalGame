// craftsearchtest.js — BUDDY CRAFT menu search (Nov 14): the craft
// list runs to thirty-odd rows; on a phone that is a long scroll.
// Press "/" (or tap the search row) and type: the list filters flat
// to matching items. Covers: search on, filtering, same-session
// backspace, Escape ending the search AND clearing the filter (the
// soft-keyboard session restarts empty, so a standing filter could
// never be edited again), and the no-match sentinel row.
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);

  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT (mode 4)', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  await key('KeyF', 250); await sleep(300);
  T('the craft menu opens', (await call('dbg_craftmenu')) === 1, await call('dbg_craftmenu'));
  const full = await call('dbg_craftrows');
  T('the full list is long', full >= 20, 'rows=' + full);
  T('search starts off', (await call('dbg_craftsearch')) === 0, await call('dbg_craftsearch'));

  // "/" starts the search (P belongs to the BW menu); AXE matches
  // exactly the two axes, WOODEN AXE first
  await key('Slash', 250); await sleep(200);
  T('Slash turns search on', (await call('dbg_craftsearch')) === 1, await call('dbg_craftsearch'));
  await page.keyboard.type('AXE', { delay: 90 });
  await sleep(400);
  T('typing AXE filters to the two axes', (await poll('dbg_craftrows', v => v === 2, 4000)) === 2, await call('dbg_craftrows'));
  T('...with the WOODEN AXE (id 0) selected', (await call('dbg_craftrowid')) === 0, await call('dbg_craftrowid'));
  await page.screenshot({ path: __dirname + '/craft-search.png' });

  // backspace inside the same session unwinds the filter
  for (let i = 0; i < 3; i++) await key('Backspace', 120);
  await sleep(300);
  T('backspacing the filter restores the full list', (await poll('dbg_craftrows', v => v === full, 4000)) === full, await call('dbg_craftrows'));
  T('...still searching', (await call('dbg_craftsearch')) === 1, await call('dbg_craftsearch'));

  // Escape ends the search and clears the filter; the menu stays
  await page.keyboard.type('WOODEN', { delay: 90 });
  await sleep(300);
  T('WOODEN narrows to axe + wall', (await call('dbg_craftrows')) === 2, await call('dbg_craftrows'));
  await key('Escape', 250); await sleep(300);
  T('Escape leaves search mode', (await call('dbg_craftsearch')) === 0, await call('dbg_craftsearch'));
  T('...the menu stays open', (await call('dbg_craftmenu')) === 1, await call('dbg_craftmenu'));
  T('...and the filter is cleared', (await call('dbg_craftrows')) === full, await call('dbg_craftrows'));

  // a nonsense filter yields the sentinel row, never a crash
  await key('Slash', 250); await sleep(200);
  await page.keyboard.type('ZZZ', { delay: 90 });
  await sleep(400);
  T('a hopeless filter still shows one (sentinel) row', (await call('dbg_craftrows')) === 1, await call('dbg_craftrows'));
  T('the sentinel is not a recipe', (await call('dbg_craftrowid')) === -99, await call('dbg_craftrowid'));
  await key('Escape', 250); await sleep(200);
  await key('Escape', 250); await sleep(300);
  T('Escape closes the menu', (await call('dbg_craftmenu')) === 0, await call('dbg_craftmenu'));

  await key('Escape', 300); await sleep(400);
  T('Escape returns to the mall', (await call('dbg_bwmode')) === -1, await call('dbg_bwmode'));

  console.log('craftsearchtest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
