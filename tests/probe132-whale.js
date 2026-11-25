// probe132-whale.js — v1.32 Item 2 reproduction: fresh profile, watch
// the whale masks (global + the local player's) and the PARTY-burst
// witness through: boot -> run start -> bug kills -> autosave ->
// reload/Continue -> Photo Booth snap -> reload/Continue.
// Temporary probe; the durable checks land in whaletest.js.
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
    const g = await call('dbg_whale'), gn = await call('dbg_whaleon');
    const p = await call('dbg_pown'), pn = await call('dbg_pwon');
    const party = await call('dbg_party'), kills = await call('dbg_kills');
    console.log(`[${tag}] G own=${g} on=${gn} | P0 own=${p} on=${pn} | party=${party} kills=${kills}`);
  };
  const killOne = async () => {
    await call('dbg_tp', 0, 58); await sleep(250);
    const px = await call('dbg_px'), py = await call('dbg_py');
    await call('dbg_spawnbug', px + 45, py + 10);
    await sleep(300);
    const k0 = await call('dbg_kills');
    for (let i = 0; i < 12 && (await call('dbg_kills')) === k0; i++) {
      const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = bx - x, dy = by - y;
      const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
      await page.keyboard.down(dir); await sleep(200);
      await key('Space', 110); await key('Space', 110);
      await page.keyboard.up(dir);
    }
    return (await call('dbg_kills')) > k0;
  };
  const dumpSave = async tag => {
    const keys = await page.evaluate(() => { const o = {}; for (let i = 0; i < localStorage.length; i++) { const k = localStorage.key(i); if (k && k.indexOf('tsg_save') === 0) o[k] = localStorage.getItem(k); } return o; });
    for (const [k, v] of Object.entries(keys)) console.log(`[${tag}] ${k}:`, /whale=\d+/.exec(v)?.[0], /whaleon=\d+/.exec(v)?.[0], '|', /pt=[^ ]*/.exec(v)?.[0]);
    if (!Object.keys(keys).length) console.log(`[${tag}] no tsg_save keys`);
  };
  const continueRun = async () => {
    for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(250); const b = await call('dbg_menucount'); if (a === b && a >= 6) break; }
    await key('Space', 400);
    if (await poll('dbg_state', 7, 4000)) { await key('Enter', 500); await poll('dbg_state', 1, 8000); }
    console.log('state after continue:', await call('dbg_state'));
  };

  await masks('title, fresh profile');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  console.log('state after start:', await call('dbg_state'));
  await masks('run started');
  for (let i = 0; i < 3; i++) console.log('kill', i, await killOne());
  await masks('after kills');
  await sleep(6000);
  await dumpSave('autosaved');

  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(800);
  await masks('after reload (records loaded)');
  await continueRun();
  await masks('after Continue');
  for (let i = 0; i < 2; i++) console.log('kill', i, await killOne());
  await masks('after post-load kills');

  // Photo Booth: island disc at (-232,-5980), station 8
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  console.log('in mall:', await call('dbg_mall'));
  await call('dbg_tp', -232, -5980); await sleep(300);
  await key('KeyE', 350);
  console.log('booth stage:', await call('dbg_portstage'), 'station:', await call('dbg_station'));
  if (await poll('dbg_portstage', 5, 3000)) {
    await key('ArrowDown', 130); await key('ArrowDown', 130);
    await key('Space', 400);   // SNAP (row 2)
    await sleep(400);
    console.log('portrait has:', await call('dbg_porthas'));
  }
  await masks('after booth snap');
  await key('Escape', 300);

  await sleep(6000);
  await dumpSave('autosaved2');
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(800);
  await masks('after 2nd reload');
  await continueRun();
  await masks('after 2nd Continue');
  for (let i = 0; i < 2; i++) console.log('kill', i, await killOne());
  await masks('after booth+load kills');
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
