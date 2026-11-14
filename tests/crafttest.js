// crafttest.js — BUDDY CRAFT (Nov 5): the top-down endless crafting
// wild inside Banzai World (mode 4). Covers: entering the mode, finding
// a tree in the procedural world, gathering wood from it with E,
// crafting the WOODEN AXE from the F menu, and the gear surviving a
// leave-and-return cycle.
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
  const wood = async () => (await call('dbg_craftmat')) % 1000;

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);

  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT (mode 4)', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  T('pack starts empty', (await call('dbg_craftmat')) === 0 && (await call('dbg_craftcar')) === 0);

  // find a tree in the endless wild and stand west of it
  let tree = null;
  for (let r = 3; r <= 16 && !tree; r++)
    for (let tx = -r; tx <= r && !tree; tx++)
      for (let ty = -r; ty <= r && !tree; ty++)
        if ((await call('dbg_craftnode', tx, ty)) === 1) tree = [tx, ty];
  T('the wild grows trees', !!tree, tree);
  if (tree) {
    await call('dbg_bwtp', tree[0] * 48 + 24 - 62, tree[1] * 48 + 24);
    await sleep(200);
    await page.keyboard.down('ArrowRight'); await sleep(450); await page.keyboard.up('ArrowRight');
    await sleep(150);
    for (let i = 0; i < 5 && (await wood()) === 0; i++) { await key('KeyE', 200); await sleep(420); }
    T('gathered wood from the tree', (await wood()) >= 3, await wood());
    await page.screenshot({ path: __dirname + '/craft-wild.png' });

    // craft the axe: F opens the menu, row 0 is the axe, Space crafts
    await key('KeyF', 250); await sleep(300);
    T('the craft menu opens', (await call('dbg_craftmenu')) === 1, await call('dbg_craftmenu'));
    const w0 = await wood();
    await key('Space', 250); await sleep(300);
    T('the WOODEN AXE is crafted', ((await call('dbg_craftgear')) & 1) === 1, await call('dbg_craftgear'));
    T('crafting spent the wood', (await wood()) === w0 - 3, w0 + ' -> ' + (await wood()));
    await key('Escape', 250); await sleep(300);
    T('the menu closes', (await call('dbg_craftmenu')) === 0, await call('dbg_craftmenu'));
  }

  await key('Escape', 300); await sleep(400);
  T('Escape returns to the mall', (await call('dbg_bwmode')) === -1, await call('dbg_bwmode'));
  await call('dbg_bwentm', 4);
  T('the axe survives the round trip', ((await call('dbg_craftgear')) & 1) === 1, await call('dbg_craftgear'));

  console.log('crafttest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
