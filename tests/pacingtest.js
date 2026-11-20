// pacingtest.js — THREAT spawn retune (Daniel, Nov 2): pacing was
// too aggressive and too many BIG bugs showed up, especially near
// home at high THREAT. Three fresh runs: (1) baseline sizes far out
// at THREAT x1, (2) sizes near home at high THREAT — the old code
// rolled doorstep bugs like far-out giants — (3) spawn fill rate far
// out at high THREAT. Run 2's numbers were recalibrated Nov 18, 2026
// (Daniel's call) for his Nov 5 directive that size scales with
// THREAT indefinitely, and again the same evening for his /3 size
// stretch of that scaling; the derivation is written out at run 2.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function freshRun() {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  return { browser, page, call, key };
}

(async () => {
  // run 1: THREAT x1, bugs spawned far out (2500 from the epicenter)
  {
    const { browser, call } = await freshRun();
    ok((await call('dbg_state')) === 1, 'run 1 started');
    // stand near the spawn site: bugs >1750 px from the player are culled
    await call('dbg_tp', 2500, 420); await sleep(200);
    for (let i = 0; i < 30; i++) await call('dbg_spawnbug', 2500, (i % 7) * 30 - 90);
    const avg = await call('dbg_bugavgw'), big = await call('dbg_bugbig'), n = await call('dbg_bugs');
    ok(n >= 30, 'run 1: sample spawned', n);
    ok(avg >= 80 && avg <= 280, 'run 1: far-out bugs at x1 average ~17 kg (small-leaning)', avg);
    ok(big <= 6, 'run 1: few giants far out at x1', big);
    await browser.close();
  }

  // run 2: high THREAT, bugs spawned near home (700 from the epicenter).
  // RECALIBRATED Nov 18, 2026 at Daniel's direction, twice over: the
  // original bounds (avg <= 400, giants <= 9) encoded his Nov 2
  // doorstep calibration, but his Nov 5 directive — SIZE scales with
  // THREAT, indefinitely — superseded it, and on Nov 18 Daniel
  // confirmed the scaling stands and this test moves. Later that
  // evening he stretched ONLY the size slopes /3: still linear, still
  // no cap, but the giants take the scenic route (the Nov 5 slope
  // 0.30 -> 0.10, the ceiling lift 0.35 -> 0.35/3; every other scaling
  // term untouched). New bounds derived from the code, not fitted to
  // a run (spawn_bug / bug_max_weight in buddies.c, THREAT 4.4,
  // d = 700, EASY so d_w() = 1):
  //   de   = 700 + (4.4-1)*650 = 2910
  //   maxW = bug_max_weight(2910)
  //        = 2 + 133 * 0.97^2 * (0.35 + 0.65*0.97) ~= 124.7 kg
  //          (the stretched threat cap, 150*(1+3.4*(0.35/3)) ~= 209.5,
  //          still does not bind at the doorstep sample)
  //   w    = (0.6 + r^4 * maxW) * (1 + (4.4-1)*0.10),  E[r^4] = 1/5
  //   pre-scaling mean = 0.6 + 124.7/5 ~= 25.5 kg — exactly the Nov 2
  //   doorstep calibration; the stretched multiplier is 1 + 3.4*0.10
  //   = 1.34, so the expected mean is 25.5 * 1.34 ~= 34.2 kg (342 in
  //   dbg_bugavgw's x10 units). The original average bound kept x1.6
  //   headroom over its expectation (25 -> <=40); same philosophy:
  //   342 * 1.6 ~= 547.
  //   Giants: P(w > 60) = P(r^4 > (60/1.34 - 0.6)/124.7 = 0.354)
  //   = 1 - 0.354^(1/4) ~= 0.229, so ~7 of 30 are giants now. The
  //   Nov 18 giant bound kept ~x1.6 over its ~9.2 expectation
  //   (<= 15); same philosophy over the new ~6.9 expectation: <= 11.
  {
    const { browser, call } = await freshRun();
    ok((await call('dbg_state')) === 1, 'run 2 started');
    await call('dbg_setthreat', 44);   // THREAT x4.4 via the test hook
    const th = await call('dbg_threat');
    ok(th >= 350, 'run 2: THREAT is high', th);
    await call('dbg_tp', 0, 300); await sleep(200);
    for (let i = 0; i < 30; i++) await call('dbg_spawnbug', 700, (i % 7) * 30 - 90);
    const avg = await call('dbg_bugavgw'), big = await call('dbg_bugbig');
    ok(avg <= 547, 'run 2: doorstep bugs at high THREAT average ~34 kg (Nov 18 size stretch of the ~25 kg doorstep), not the unstretched ~52', avg);
    ok(big <= 11, 'run 2: giants (>60 kg) near home at high THREAT run ~7 of 30 by design (Nov 18 size stretch)', big);
    await browser.close();
  }

  // run 3: high THREAT fill rate, player standing far out
  {
    const { browser, call } = await freshRun();
    ok((await call('dbg_state')) === 1, 'run 3 started');
    await call('dbg_setthreat', 44);
    await call('dbg_tp', 2500, 0);
    const t0 = Date.now();
    while (Date.now() - t0 < 6500) await sleep(250);
    const alive = await call('dbg_bugs');
    ok(alive >= 10, 'run 3: the spawner is feeding the swarm', alive);
    ok(alive <= 37, 'run 3: ~30 bugs after 6.5s at high THREAT (old pacing hit ~45)', alive);
    ok((await call('dbg_state')) === 1, 'run 3: player still standing', await call('dbg_state'));
    await browser.close();
  }

  console.log(`${pass} passed, ${fail} failed`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
