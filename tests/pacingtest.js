// pacingtest.js — THREAT spawn retune (Daniel, Nov 2): pacing was
// too aggressive and too many BIG bugs showed up, especially near
// home at high THREAT. Three fresh runs: (1) baseline sizes far out
// at THREAT x1, (2) sizes near home at high THREAT — the old code
// rolled doorstep bugs like far-out giants — (3) spawn fill rate far
// out at high THREAT.
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

  // run 2: high THREAT, bugs spawned near home (700 from the epicenter)
  {
    const { browser, call } = await freshRun();
    ok((await call('dbg_state')) === 1, 'run 2 started');
    await call('dbg_setthreat', 44);   // THREAT x4.4 via the test hook
    const th = await call('dbg_threat');
    ok(th >= 350, 'run 2: THREAT is high', th);
    await call('dbg_tp', 0, 300); await sleep(200);
    for (let i = 0; i < 30; i++) await call('dbg_spawnbug', 700, (i % 7) * 30 - 90);
    const avg = await call('dbg_bugavgw'), big = await call('dbg_bugbig');
    ok(avg <= 400, 'run 2: doorstep bugs at high THREAT average ~25 kg, not ~68', avg);
    ok(big <= 9, 'run 2: giants (>60 kg) are the exception near home, not a third of the swarm', big);
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
