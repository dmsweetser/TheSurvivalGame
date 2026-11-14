// batch5shots.js — visual verification for the Nov 3 batch: castle
// walls (tier 3 battlements, tier 4 endpoint turret), the mall's
// Banzai World gate, the BW overworld, and a BW battle.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  async function poll(fn, want, timeout = 15000, args = []) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(250); }
    return call(fn, ...args);
  }
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);

  // --- the mall gate (fresh run, before any fighting)
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  { const t0 = Date.now();
    while (Date.now() - t0 < 15000) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - 185) <= 12 && Math.abs(y - (-6000 + 55)) <= 12) break;
      const keys = [];
      if (x < 173) keys.push('ArrowRight'); else if (x > 197) keys.push('ArrowLeft');
      if (y < -5957) keys.push('ArrowDown'); else if (y > -5933) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    } }
  await sleep(300);
  await page.screenshot({ path: __dirname + '/bw-gate.png' });
  await call('dbg_tp', 0, -6000 + 150); await sleep(150);
  await key('KeyE', 300);   // exit mat: back outside

  // --- castle walls, tier 4: 34 spots x 3 stacked = 102 segments.
  // Feed sparingly: a giant hat scales the spawner up and gets the
  // photographer killed (learned the hard way, three times).
  await call('dbg_tp', -1500, 320); await sleep(200);
  let built = 0;
  for (let i = 0; i < 34; i++) {
    await call('dbg_setthreat', 0);   // keep the photoshoot peaceful
    await call('dbg_feed');
    for (let s = 0; s < 3; s++) {
      await call('dbg_tp', -1500 + i * 44, 320); await sleep(50);
      await key('KeyF', 90);
      const w = await poll('dbg_walls', v => v > built, 2500);
      if (w > built) built = w;
    }
    if ((await call('dbg_state')) !== 1) { console.log('run ended early at spot', i); break; }
  }
  console.log('tier4 run: walls =', built, 'grpmax =', await call('dbg_grpmax'));
  await call('dbg_tp', -1500 + 12, 250); await sleep(300);
  await page.screenshot({ path: __dirname + '/castle-tier4-end.png' });
  await call('dbg_tp', -1500 + 17 * 44, 250); await sleep(300);
  await page.screenshot({ path: __dirname + '/castle-tier4-mid.png' });

  // --- Banzai World: overworld + battle
  await call('dbg_bwenter');
  await sleep(400);
  await page.keyboard.down('ArrowDown'); await sleep(1200); await page.keyboard.up('ArrowDown');
  await page.keyboard.down('ArrowRight'); await sleep(700); await page.keyboard.up('ArrowRight');
  await page.screenshot({ path: __dirname + '/bw-world.png' });
  await call('dbg_teach', 1); await call('dbg_teach', 4);
  for (let i = 0; i < 20 && (await call('dbg_bwbattle')) === 0; i++) {
    await call('dbg_bwtele_hack');
    await sleep(500);
  }
  await poll('dbg_bwbattle', v => v === 1, 8000);
  await sleep(300);
  await page.screenshot({ path: __dirname + '/bw-battle.png' });
  console.log('shots done');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
