// scaletest.js — Nov 5 scaling batch (Daniel):
//  1. bug size scales indefinitely with THREAT (+ camera steps back)
//  2. buddy training costs: cheap early, steep late (10, 17, 27, ...)
//  3. the buddy climbs wall runs the player can climb
//  4. Banzai World foe level tracks buddy capability, not distance
//  5. game over shows you wearing the hat you died in (+ share card)
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function ok(cond, label, extra) {
  if (cond) { pass++; console.log('  ok -', label); }
  else { fail++; console.log('  FAIL -', label, extra !== undefined ? ' :: ' + extra : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const poll = async (n, a, pred, ms = 6000) => {
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(n, ...a); if (pred(v)) return v; await sleep(120); }
    return v;
  };
  await page.evaluate(() => { localStorage.clear(); });
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  for (let i = 0; i < 5 && (await call('dbg_state')) !== 1; i++) await key('Enter', 450);
  ok((await call('dbg_state')) === 1, 'run started', await call('dbg_state'));
  const cv = await page.$('canvas');
  const exitMall = async () => {
    await call('dbg_tp', 0, -5850); await sleep(300);
    await key('KeyE', 300);
    return poll('dbg_mall', [], v => v === 0, 5000);
  };

  // ---------- mall + buddy ----------
  await call('dbg_tp', 0, 80); await sleep(300);
  await key('KeyE', 350);
  ok((await poll('dbg_mall', [], v => v === 1)) === 1, 'in the mall');
  await call('dbg_adopt', 0);                       // a puppy, beside us in the mall

  // ---------- 2. training cost curve (10, 17, 27) ----------
  await call('dbg_tp', 225, -6060); await sleep(300);   // the Banzai Buddies counter
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open', await call('dbg_station'));
  const trainOnce = async (xp) => {
    await call('dbg_setbudxp', xp);
    await key('Space', 350);
    return [await call('dbg_buddyxp'), await call('dbg_budups')];
  };
  let [xp, ups] = await trainOnce(10);
  ok(xp === 0 && ups >= 1000, 'level 1 training costs 10 XP', `xp=${xp} ups=${ups}`);
  [xp, ups] = await trainOnce(16);
  ok(xp === 16 && ups < 2000, 'level 2 refuses 16 XP (costs 17)', `xp=${xp} ups=${ups}`);
  [xp, ups] = await trainOnce(17);
  ok(xp === 0 && ups >= 2000, 'level 2 training costs 17 XP', `xp=${xp} ups=${ups}`);
  [xp, ups] = await trainOnce(25);
  ok(xp === 25 && ups < 3000, 'level 3 refuses 25 XP (costs 26)', `xp=${xp} ups=${ups}`);
  [xp, ups] = await trainOnce(26);
  ok(xp === 0 && ups >= 3000, 'level 3 training costs 26 XP', `xp=${xp} ups=${ups}`);
  await key('Escape', 300);
  ok((await exitMall()) === 0, 'left the mall');
  await poll('dbg_budy', [], v => v > -1000, 6000);     // buddy followed out

  // ---------- 3. buddy climbs the run ----------
  for (let i = 0; i < 10; i++) await call('dbg_mkwall', -198 + i * 44, 300, 1);
  ok((await poll('dbg_grpmax', [], v => v === 10)) === 10, 'run of 10 interlocked walls');
  await call('dbg_tp', 0, 368);                      // player south of the run
  const yS = await poll('dbg_budy', [], v => v > 330, 8000);
  ok(yS > 330, 'buddy settled south of the run', yS);
  await call('dbg_tp', 0, 232);                      // player steps north of it
  let sawUp = -1;
  { const t0 = Date.now();
    while (Date.now() - t0 < 10000) { const w = await call('dbg_budwall'); if (w >= 0) { sawUp = w; break; } await sleep(40); } }
  ok(sawUp >= 0, 'buddy climbed ONTO the run to follow', sawUp);
  if (sawUp >= 0) await cv.screenshot({ path: __dirname + '/scale-budwall.png' });
  const yN = await poll('dbg_budy', [], v => v < 285, 9000);
  ok(yN < 285, 'buddy crossed to the north side', yN);

  // ---------- 4. BW foe level tracks capability ----------
  await call('dbg_tp', 0, 80); await sleep(300);
  await key('KeyE', 350);                            // back into the mall for the gate
  await poll('dbg_mall', [], v => v === 1, 5000);
  await call('dbg_bwenter'); await sleep(600);
  await call('dbg_bwtp', 600, 600); await sleep(800);
  const lvlFresh = await poll('dbg_roamlvl', [], v => v >= 1, 6000);
  ok(lvlFresh >= 1 && lvlFresh <= 4, 'fresh puppy meets fresh foes (lvl <= 4)', lvlFresh);
  await call('dbg_budup', 15);                       // a seriously trained buddy
  await call('dbg_bwtp', 3200, 3200); await sleep(900);   // fresh cells, fresh foes
  const lvlTrained = await poll('dbg_roamlvl', [], v => v >= 10, 8000);
  ok(lvlTrained >= 10, 'trained buddy meets trained foes (lvl >= 10)', lvlTrained);
  await key('Escape', 400); await sleep(500);        // out of BW, back in the mall
  ok((await exitMall()) === 0, 'out of the mall again');

  // ---------- 1. threat size + zoom ----------
  await call('dbg_tp', 3000, 3000); await sleep(300);
  await call('dbg_setthreat', 10);                    // threat x1.0
  await poll('dbg_bugs', [], v => v >= 6, 9000);
  const avg1 = await call('dbg_bugavgw'), big1 = await call('dbg_bugbig');
  await call('dbg_setthreat', 150);                   // threat x15: weights x5.2
  await call('dbg_tp', 3600, 3600); await sleep(4000);
  const avg2 = await call('dbg_bugavgw'), big2 = await call('dbg_bugbig');
  ok(avg2 > avg1 * 2.2, 'average bug weight leaps with threat', `${avg1} -> ${avg2}`);
  ok(big2 >= 1, 'giant bugs (w>60) exist at high threat', `${big1} -> ${big2}`);
  await cv.screenshot({ path: __dirname + '/scale-threat.png' });
  await call('dbg_setthreat', -1);

  // ---------- 5. game over portrait + share card ----------
  await call('dbg_overhat', 60);
  ok((await call('dbg_state')) === 3, 'game over screen up');
  await sleep(1200);
  await cv.screenshot({ path: __dirname + '/scale-gameover.png' });
  const dl = page.waitForEvent('download', { timeout: 8000 }).catch(() => null);
  await page.click('#savecard').catch(() => null);
  const d = await dl;
  if (d) { await d.saveAs(__dirname + '/scale-sharecard.png'); ok(true, 'share card downloaded'); }
  else ok(false, 'share card downloaded');

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
