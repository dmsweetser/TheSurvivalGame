// Music ladder + Bug Bowl: scoring exactness via roll hooks, then a
// real thrown game through the mall lanes view.
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
  const state = () => call('dbg_state');
  async function poll(fn, want, timeout = 10000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await fn(); if (want(v)) return v; await sleep(step); }
    return fn();
  }
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
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const roll = k => call('dbg_bowl_roll', k);
  async function game(rolls) { await call('dbg_bowl_new'); for (const r of rolls) await roll(r); }

  // ---- boot + start -------------------------------------------------------
  ok((await state()) === 0, 'boots to title');
  ok((await call('dbg_track')) === 0, 'menu music is the serene Moonlight');
  await key('Space', 400);
  if ((await state()) !== 1) { await key('Enter', 400); }
  ok((await state()) === 1, 'run started', await state());

  // ---- the Beethoven ladder ------------------------------------------------
  await call('dbg_setthreat', 10);
  ok((await call('dbg_track')) === 0, 'threat 1.0: Moonlight I (serene)');
  await call('dbg_setthreat', 25);
  ok((await call('dbg_track')) === 1, 'threat 2.5: Fur Elise');
  await call('dbg_setthreat', 45);
  ok((await call('dbg_track')) === 2, 'threat 4.5: Symphony No. 5');
  await call('dbg_setthreat', 80);
  ok((await call('dbg_track')) === 3, 'threat 8.0: Moonlight III (the storm)');
  await call('dbg_setthreat', -1);
  // mus_piece reads G.threat, which compute_threat refreshes once per
  // frame — give it a few frames to notice the cleared override
  // (reading in the same tick sees the stale 8.0 and reports the storm)
  let trk = -1;
  for (let i = 0; i < 20; i++) { trk = await call('dbg_track'); if (trk === 0) break; await sleep(120); }
  ok(trk === 0, 'override cleared: back to calm at run start', trk);

  // ---- bowling scoring, exactly -------------------------------------------
  await call('dbg_bowl_new');
  await roll(10);
  ok((await call('dbg_bowl_score')) === -1, 'a lone strike is not yet scoreable');
  await roll(7); await roll(2);
  ok((await call('dbg_bowl_score')) === 28, 'running total after frame 2 = 19 + 9 = 28', await call('dbg_bowl_score'));
  for (const r of [9, 0, 10, 10, 10, 5, 5, 3, 4, 10, 10, 10, 10]) await roll(r);
  ok((await call('dbg_bowl_over')) === 1, 'game A ends after the tenth');
  ok((await call('dbg_bowl_score')) === 192, 'game A totals 192', await call('dbg_bowl_score'));
  await game(Array(21).fill(5));
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 150,
     'all spares = 150', await call('dbg_bowl_score'));
  await game(Array(12).fill(10));
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 300,
     'perfect game = 300', await call('dbg_bowl_score'));
  await call('dbg_bowl_new');
  for (let i = 0; i < 20; i++) await roll(0);
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 0,
     'all gutters = 0 in 20 rolls');
  await game([...Array(18).fill(0), 10, 7]);
  ok((await call('dbg_bowl_pins')) === 3, 'tenth: X,7 leaves 3 pins', await call('dbg_bowl_pins'));
  await roll(2);
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 19,
     'tenth X 7 2 = 19', await call('dbg_bowl_score'));
  await game([...Array(18).fill(0), 3, 7]);
  ok((await call('dbg_bowl_pins')) === 10, 'tenth: spare earns a fresh rack', await call('dbg_bowl_pins'));
  await roll(10);
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 20,
     'tenth 3 / X = 20', await call('dbg_bowl_score'));

  // ---- a real thrown game at the lanes -------------------------------------
  await call('dbg_bowl_new');
  await call('dbg_feed'); await call('dbg_feed');
  const carried0 = await call('dbg_carried');
  await call('dbg_tp', 0, 80);
  await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  ok(await walkTo(-170, -5908, 14), 'walked to the Bug Bowl counter');
  await key('KeyE', 300);
  ok((await state()) === 2 && (await call('dbg_station')) === 4, 'bowling view opens', `${await state()}/${await call('dbg_station')}`);
  await key('Space', 250);
  await key('Space', 300);
  await key('Space', 500);
  await key('Space', 200);
  const carried1 = await poll(() => call('dbg_carried'), v => v < carried0, 5000);
  ok(carried1 === carried0 - 20, 'one carcass off the head per ball', `${carried0} -> ${carried1}`);
  const pins1 = await poll(() => call('dbg_bowl_pins'), v => v < 10, 7000);
  ok(pins1 < 10, 'a thrown carcass knocks pins down', pins1);
  await sleep(1600);
  await key('Space', 300); await key('Space', 500); await key('Space', 200);
  const fr = await poll(() => call('dbg_bowl_frame'), v => v >= 10, 9000);
  ok(fr >= 10, 'frame 1 completes after two balls', fr);
  await key('Escape', 300);
  ok((await state()) === 1, 'bowling view closes');

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
