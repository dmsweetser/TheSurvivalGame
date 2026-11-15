// platformtest.js — SUPER BUDDY RUN (Nov 5): the side-scrolling
// platformer inside Banzai World (mode 3). Covers: entering the mode,
// running right under gravity, jumping and landing back on the ground,
// hearts intact, and leaving back to the mall.
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
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(100); } return v; };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);

  await call('dbg_bwentm', 3);
  T('entered SUPER BUDDY RUN (mode 3)', (await poll('dbg_bwmode', v => v === 3)) === 3, await call('dbg_bwmode'));
  T('starts at level 1', (await call('dbg_platlvl')) === 1, await call('dbg_platlvl'));
  T('three hearts', (await call('dbg_plath')) === 3, await call('dbg_plath'));
  T('no levels done yet', (await call('dbg_platdone')) === 0, await call('dbg_platdone'));

  const x0 = await call('dbg_bwx'), y0 = await call('dbg_bwy');
  T('spawned on the ground', y0 >= 300 && y0 <= 320, y0);
  await page.keyboard.down('ArrowRight');
  await sleep(1200);
  await page.keyboard.up('ArrowRight');
  const x1 = await call('dbg_bwx');
  T('running right advances', x1 > x0 + 130, x0 + ' -> ' + x1);
  const y1 = await call('dbg_bwy');
  T('still on solid footing after the run', y1 >= 200 && y1 <= 340, y1);

  // jump: leave the ground, come back — from the swept spawn flat,
  // so the check is about jumping, not about where the run ended up
  await call('dbg_bwtp', 60, 312);
  await sleep(400);
  await key('Space', 120);
  const yAir = await poll('dbg_bwy', v => v < y0 - 46, 2500);
  T('jump leaves the ground', yAir < y0 - 46, yAir);
  // landing may be on the ground OR on top of a block (the momentum
  // rework's higher jump reaches block tops the old jump couldn't) —
  // what matters is that he comes down and stays down
  let yLand = 0, yPrev = -1, stable = 0;
  { const t0 = Date.now();
    while (Date.now() - t0 < 4000) { yLand = await call('dbg_bwy');
      if (yLand === yPrev && yLand <= y0 + 1) { if (++stable >= 3) break; } else stable = 0;
      yPrev = yLand; await sleep(120); } }
  T('and lands again', stable >= 3 && yLand <= y0 + 1, yLand);
  await page.screenshot({ path: __dirname + '/plat-run.png' });

  // REGIONS (Nov 14): level L wears region (L-1)%8 — the same eight
  // countries as the main game, in the same order, every time
  for (let L = 1; L <= 8; L++) {
    await call('dbg_platwarp', L);
    T('level ' + L + ' is region ' + (L - 1), (await call('dbg_platregion')) === L - 1, await call('dbg_platregion'));
    if (L === 5) await page.screenshot({ path: __dirname + '/plat-cache.png' });
    if (L === 6) await page.screenshot({ path: __dirname + '/plat-ram.png' });
  }

  // the BONUS VAULT: find a level whose tallest pipe is gold-banded,
  // stand on it, press DOWN, take the heart, leave by the far pipe
  let vaultLvl = 0;
  for (let L = 1; L <= 12 && !vaultLvl; L++) {
    await call('dbg_platwarp', L);
    if ((await call('dbg_platvaultx')) >= 0) vaultLvl = L;
  }
  T('some level hides a vault pipe', vaultLvl > 0, 'lvl=' + vaultLvl);
  if (vaultLvl) {
    const vx = await call('dbg_platvaultx');
    await call('dbg_bwtp', vx, 60);
    await sleep(900);   // fall onto the pipe top and settle
    await key('ArrowDown', 200);
    T('DOWN on the gold pipe drops into the vault', (await poll('dbg_platsub', v => v === 1, 4000)) === 1, await call('dbg_platsub'));
    await page.screenshot({ path: __dirname + '/plat-vault.png' });
    const h0 = await call('dbg_plath');
    await call('dbg_bwtp', 17 * 24, 312);
    await sleep(500);
    T('the vault heart is real (+1)', (await call('dbg_plath')) === Math.min(5, h0 + 1), h0 + ' -> ' + (await call('dbg_plath')));
    await call('dbg_bwtp', 33 * 24, 60);
    await sleep(900);
    await key('ArrowDown', 200);
    T('the far pipe leads back to the run', (await poll('dbg_platsub', v => v === 0, 4000)) === 0, await call('dbg_platsub'));
    T('...on the same level as before', (await call('dbg_platlvl')) === vaultLvl, await call('dbg_platlvl'));
  }

  await key('Escape', 300); await sleep(400);
  T('Escape returns to the mall', (await call('dbg_bwmode')) === -1, await call('dbg_bwmode'));

  console.log('platformtest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
