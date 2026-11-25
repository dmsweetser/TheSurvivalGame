// probe132-whale2.js — v1.32 Item 2: crafted-save reproduction.
// A save whose global whaleon= line carries bit 10 (CONFETTI SQUASH)
// while whale= (owned) does NOT: does the loader apply it, does the
// kill gate fire on it, and does it self-perpetuate through autosave?
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
  const masks = async tag => {
    console.log(`[${tag}] G own=${await call('dbg_whale')} on=${await call('dbg_whaleon')} | P0 own=${await call('dbg_pown')} on=${await call('dbg_pwon')} | party=${await call('dbg_party')} kills=${await call('dbg_kills')}`);
  };
  const killOne = async () => {
    await call('dbg_tp', 0, 58); await sleep(250);
    const px = await call('dbg_px'), py = await call('dbg_py');
    await call('dbg_spawnbug', px + 45, py + 10);
    await sleep(300);
    // find the spawned bug's slot (bug 0 may be an ambient bug post-Continue)
    let bi = 0, best = 1e9;
    for (let i = 0; i < 64; i++) {
      const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i);
      if (!isFinite(bx) || !isFinite(by)) continue;
      const d = Math.abs(bx - (px + 45)) + Math.abs(by - (py + 10));
      if (d < best) { best = d; bi = i; }
    }
    const k0 = await call('dbg_kills');
    for (let i = 0; i < 14 && (await call('dbg_kills')) === k0; i++) {
      const bx = await call('dbg_bugx', bi), by = await call('dbg_bugy', bi);
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = bx - x, dy = by - y;
      const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
      await page.keyboard.down(dir); await sleep(200);
      await key('Space', 110); await key('Space', 110);
      await page.keyboard.up(dir);
    }
    return (await call('dbg_kills')) > k0;
  };

  // 1) fresh run -> autosave -> capture the globals blob
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(6000);
  let glob = await page.evaluate(() => localStorage.getItem('tsg_save') || '');
  console.log('blob has:', /whale=\d+/.exec(glob)?.[0], /whaleon=\d+/.exec(glob)?.[0]);
  // 2) patch: confetti ON without ownership
  glob = glob.replace(/whaleon=\d+/, 'whaleon=1024');
  await page.evaluate(s => localStorage.setItem('tsg_save', s), glob);
  console.log('patched whaleon=1024, whale stays 0');
  // 3) reload -> Continue -> kill
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(800);
  await masks('after reload (crafted save)');
  for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(250); const b = await call('dbg_menucount'); if (a === b && a >= 6) break; }
  await key('Space', 400);
  if (await poll('dbg_state', 7, 4000)) { await key('Enter', 500); await poll('dbg_state', 1, 8000); }
  await masks('after Continue');
  // exit the mall via the exit mat (the landing pad at MALL_CY+158)
  await call('dbg_tp', 0, -5810); await sleep(300);
  await key('KeyE', 350);
  console.log('mall after exit attempt:', await call('dbg_mall'), 'pos', await call('dbg_px'), await call('dbg_py'));
  for (let i = 0; i < 3; i++) console.log('kill', i, await killOne());
  await masks('after kills — party>0 means the unowned bit fired');
  await sleep(6000);
  const glob2 = await page.evaluate(() => localStorage.getItem('tsg_save') || '');
  console.log('re-saved blob:', /whale=\d+/.exec(glob2)?.[0], /whaleon=\d+/.exec(glob2)?.[0], '(bit self-perpetuates if whaleon still 1024)');
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
