// pagetest.js — the Elven name picker has FIVE pages (60 names / 12).
// Bug (fixed Nov 3): MORE NAMES flipped the page with ^= 1, so it only
// ever bounced between pages 1 and 2. Pages must advance 1->2->3->4->5
// and wrap back to 1. Verified through the free rename flow.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 130) => { await page.keyboard.press(k); await sleep(ms); };
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }
  // open the rename picker from the buddies panel (panel must be open, stage 0)
  async function openRename() {
    for (let i = 0; i < 8; i++) await key('ArrowDown');   // row 8: NAME
    await key('Space', 300);
    return (await call('dbg_budstage')) === 3;
  }
  // press MORE NAMES (last row) n times
  async function more(n) {
    for (let m = 0; m < n; m++) {
      for (let i = 0; i < 12; i++) await key('ArrowDown', 90);
      await key('Space', 250);
    }
  }
  // pick the first name on the current page (selection resets to row 0)
  async function pickFirst() { await key('Space', 300); }

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  await call('dbg_adopt', 0);
  ok((await call('dbg_budname')) === 0, 'buddy starts with name index 0');
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  ok(await walkTo(315, -6148, 14), 'walked to the buddies counter');
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open');

  ok(await openRename(), 'rename picker opens (page 1)');
  await more(1); await pickFirst();
  ok((await call('dbg_budname')) === 12, 'page 2 first name is index 12', await call('dbg_budname'));

  ok(await openRename(), 'rename picker reopens (starts on page 2)');
  await more(3); await pickFirst();
  ok((await call('dbg_budname')) === 48, 'page 5 first name is index 48', await call('dbg_budname'));

  ok(await openRename(), 'rename picker reopens (starts on page 5)');
  await more(1); await pickFirst();
  ok((await call('dbg_budname')) === 0, 'MORE on page 5 wraps to page 1 (index 0)', await call('dbg_budname'));

  console.log(`\npagetest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
