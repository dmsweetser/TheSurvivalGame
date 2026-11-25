// probe132-killdbg.js — why do kills fail after Continue?
const { chromium } = require('playwright-core');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  const poll = async (fn, want, timeout = 6000, ...args) => {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { if (await call(fn, ...args) === want) return true; await sleep(150); }
    return false;
  };
  // fresh run + autosave so Continue exists
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(6000);
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(800);
  for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(250); const b = await call('dbg_menucount'); if (a === b && a >= 6) break; }
  await key('Space', 400);
  if (await poll('dbg_state', 7, 4000)) { await key('Enter', 500); await poll('dbg_state', 1, 8000); }
  console.log('state', await call('dbg_state'), 'mall', await call('dbg_mall'), 'pos', await call('dbg_px'), await call('dbg_py'));
  await call('dbg_tp', 0, 58); await sleep(300);
  console.log('after tp pos', await call('dbg_px'), await call('dbg_py'), 'mall', await call('dbg_mall'));
  const px = await call('dbg_px'), py = await call('dbg_py');
  await call('dbg_spawnbug', px + 45, py + 10);
  await sleep(300);
  for (let i = 0; i < 6; i++) {
    const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i), hp = await call('dbg_bughp', i);
    if (isFinite(bx)) console.log('bug', i, bx, by, 'hp', hp);
  }
  const k0 = await call('dbg_kills');
  for (let it = 0; it < 8; it++) {
    // track nearest bug to the player
    let bi = -1, best = 1e9;
    const x = await call('dbg_px'), y = await call('dbg_py');
    for (let i = 0; i < 64; i++) {
      const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i);
      if (!isFinite(bx) || !isFinite(by)) continue;
      const d = Math.abs(bx - x) + Math.abs(by - y);
      if (d < best) { best = d; bi = i; }
    }
    if (bi < 0) { console.log('no bugs found'); break; }
    const bx = await call('dbg_bugx', bi), by = await call('dbg_bugy', bi);
    const dx = bx - x, dy = by - y;
    const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
    await page.keyboard.down(dir); await sleep(200);
    await key('Space', 110); await key('Space', 110);
    await page.keyboard.up(dir);
    console.log('iter', it, 'bug', bi, bx, by, 'hp', await call('dbg_bughp', bi), 'dist', Math.abs(bx - x), Math.abs(by - y), 'kills', await call('dbg_kills'), 'projs', await call('dbg_projs'), 'state', await call('dbg_state'));
    if ((await call('dbg_kills')) > k0) break;
  }
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
