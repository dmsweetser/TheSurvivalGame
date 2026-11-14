// threatprobe2.js — measure, don't guess (Nov 9 THREAT ratio change).
// Capability scenarios are forged into the autosave's pl line (the
// loader recomputes dmg/rate/shield/hp from upg[], so the power the
// game reports is the game's own). For each scenario record:
//   - the THREAT the game computes from that power (dbg_threat)
//   - the bug-weight pressure of 60 spawns at d=3000 (avg / big /
//     max), sampled in one synchronous burst while the player sits
//     in the mall (spawner idle -> zero pollution)
//   - the spawner population trajectory 12s out at d=5000, with
//     threat pinned to the measured value (dbg_setthreat, tenths)
// Run against BEFORE and AFTER builds and compare. PIN=<tenths>
// skips the forge and pins everything (channel-sanity mode).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = process.env.URL || 'http://localhost:8903/the-survival-game.html';
const LABEL = process.env.LABEL || 'BUILD';
const PIN = process.env.PIN ? parseInt(process.env.PIN, 10) : -1;
const sleep = ms => new Promise(r => setTimeout(r, ms));
// upg[0..4] = dmg, rate, shield, hp, legs; own0 = starter copies
const SCEN = [
  ['fresh',   { upg: [0, 0, 0, 0, 0], own0: 1 }],
  ['mid',     { upg: [6, 6, 2, 2, 0], own0: 3 }],
  ['heavy',   { upg: [20, 20, 3, 3, 0], own0: 8 }],
  ['extreme', { upg: [60, 60, 6, 8, 0], own0: 20 }],
];
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const toMenu = async (clear) => {
    await page.goto(URL);
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    if (clear) { await page.evaluate(() => localStorage.clear()); await page.reload(); await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 }); }
  };
  const startFreshRun = async () => {
    await toMenu(true);
    await key('Enter', 300, 500);
    await page.waitForFunction(() => window.__TSG.dbg_state() === 7, null, { timeout: 10000 });
    await key('Enter', 300, 600);
    await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  };
  const continueRun = async () => {
    await toMenu(false);
    await key('Space', 400, 500);
    if ((await call('dbg_state')) !== 1) await key('Enter', 400, 600);
    await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  };
  console.log(`== threatprobe ${LABEL} pin=${PIN} ==`);
  for (const [name, sc] of SCEN) {
    let threat;
    if (PIN >= 0) {
      await startFreshRun();
      await call('dbg_setthreat', PIN);
      await sleep(600);
      threat = PIN / 10;
    } else {
      await startFreshRun();
      // the autosave fires every 5 GAME seconds; at software-GL frame
      // rates that can take a while in wall time — poll generously
      let raw = '';
      for (let i = 0; i < 150 && raw.length < 50; i++) { raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || ''); if (raw.length < 50) await sleep(500); }
      const forged = await page.evaluate((sc) => {
        const raw = localStorage.getItem('tsg_save1') || '';
        const lines = raw.split('\n');
        for (let li = 0; li < lines.length; li++) {
          const t = lines[li].split(' ');
          if ((t[0] === 'pl' || t[0] === 'pl=') && t[1] === '0') {
            t[8] = String(sc.own0);                       // owned[0]
            for (let u = 0; u < 5; u++) t[13 + u] = String(sc.upg[u]);
            lines[li] = t.join(' ');
            localStorage.setItem('tsg_save1', lines.join('\n'));
            return lines[li].slice(0, 120);
          }
        }
        return '';
      }, sc);
      if (!forged) {
        const dump = await page.evaluate(() => (localStorage.getItem('tsg_save1') || 'NONE').slice(0, 400));
        console.log(`${name}: FORGE FAILED save-head: ${dump}`);
        continue;
      }
      await continueRun();
      await sleep(700);
      threat = (await call('dbg_threat')) / 100;
    }
    const s = await page.evaluate(() => {
      const EX = window.__TSG;
      const n0 = EX.dbg_bugs();
      for (let i = 0; i < 60; i++) EX.dbg_spawnbug(3000, 0);
      return { n0, n: EX.dbg_bugs(), avgw: EX.dbg_bugavgw() / 10, big: EX.dbg_bugbig(), max: EX.dbg_maxbug() / 10 };
    });
    console.log(`${name}: threat=x${threat.toFixed(2)} sample(n=${s.n}, pre=${s.n0}) avgw=${s.avgw.toFixed(1)}kg big=${s.big} max=${s.max.toFixed(1)}kg`);
    // population at d=5000, threat pinned to the measured value
    await startFreshRun();
    await call('dbg_setthreat', Math.round(threat * 10));
    await call('dbg_tp', 5000, 0);
    const traj = [];
    for (let i = 0; i < 6; i++) {
      await sleep(2000);
      traj.push(await call('dbg_bugs'));
      if ((await call('dbg_state')) !== 1) { traj.push('OVER'); break; }
    }
    console.log(`${name}: population@5000 [${traj.join(', ')}]`);
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
