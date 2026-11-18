// speedtest.js — Espresso Legs rework (Daniel, Nov 2): the speed
// upgrade should matter MORE the heavier the hat is. Bare speed may
// only creep up; the load curve is what flattens. Level 0 must feel
// exactly like the classic game at every weight.
// Two later Daniel-directed changes supersede the Nov 2 numbers
// below the hat: the Endless-levels rework raised the shrug to
// 50%/level (bare bonus capped at +40%), and the Nov 14 floor pins
// any crawl to 40 px/s. Expectations here assert THAT design.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
const near = (v, want, tol) => Math.abs(v - want) <= tol;

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_tp', 3000, 3000); await sleep(200);   // far from the bugs

  // bare speeds: 148 base, +4%/level — deliberately modest
  const bare0 = await call('dbg_speed');
  ok(near(bare0, 1480, 45), 'L0 bare speed is the classic 148', bare0);
  await call('dbg_setupg', 4, 3);
  const bare3 = await call('dbg_speed');
  ok(near(bare3, 1658, 50), 'L3 bare speed only +12% (165.8)', bare3);
  await call('dbg_setupg', 4, 5);
  const bare5 = await call('dbg_speed');
  ok(near(bare5, 1776, 55), 'L5 bare speed only +20% (177.6)', bare5);

  // load up: 24 x 10 kg = 240 on the hat
  await call('dbg_setupg', 4, 0);
  for (let i = 0; i < 24; i++) await call('dbg_feed');
  ok((await call('dbg_carried')) === 2400, 'hat holds 240 kg', await call('dbg_carried'));

  const heavy0 = await call('dbg_speed');
  ok(near(heavy0, 400, 30), 'L0 at 240 kg: curve is still 27.6, pinned to the 40 floor (Nov 14)', heavy0);
  await call('dbg_setupg', 4, 3);
  const heavy3 = await call('dbg_speed');
  ok(near(heavy3, 603, 50), 'L3 at 240 kg: 60.3 with the 50%/level shrug', heavy3);
  await call('dbg_setupg', 4, 5);
  const heavy5 = await call('dbg_speed');
  ok(near(heavy5, 790, 60), 'L5 at 240 kg: 79.0 — about 2x the floored crawl', heavy5);

  const r0 = heavy0 / bare0, r5 = heavy5 / bare5;
  // the floor inflates L0's kept share (27% measured, 19% on the raw
  // curve), so the growth multiplier is asserted against the floored
  // baseline: 1.6 still fails if the shrug drops back to 35% (0.387)
  // or vanishes (0.225), and passes for the designed 50% (0.445).
  ok(r5 > r0 * 1.6, 'the heavier payoff grows with levels: L5 keeps ' +
     Math.round(r5 * 100) + '% of bare speed vs L0 keeping ' + Math.round(r0 * 100) + '%', r0 + ' -> ' + r5);

  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
