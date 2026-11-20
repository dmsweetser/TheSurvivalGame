// firstshift.js — v1.25 FIRST SHIFT: the guided first-session chain.
// Phase A walks the real player path on a FRESH slot through all six
// objectives (dbg_fs = state*10+step: active steps 10..15, done 26,
// skipped 30, off 0), asserting the prompt state advances per
// objective, the XP rewards land (pooled until adoption, then
// flushed), and the chain retires permanently across a save/reload.
// Phase B: a second fresh slot resumes mid-chain after a reload,
// then retires the chain through the pause menu's SKIP row.
// Phase C: a v1.24-format save (the fs= line and fp= tokens
// stripped by hand) shows NO chain and loses no state.
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
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  // no evaluateHandle: loadSlot reloads the page, and a handle dies
  // with its context — call through window.__TSG directly instead
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 15000) => {
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(200); }
    return v;
  };
  const saveSlot = n => page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n);
  const pollSave = async (n, needle, ms = 15000) => {
    const t0 = Date.now(); let s = '';
    while (Date.now() - t0 < ms) { s = (await saveSlot(n)) || ''; if (s.includes(needle)) return s; await sleep(300); }
    return s;
  };
  async function killOneBug() {
    const px = await call('dbg_px'), py = await call('dbg_py');
    await call('dbg_spawnbug', px + 45, py + 10);
    await sleep(300);
    const k0 = await call('dbg_kills');
    for (let i = 0; i < 24; i++) {
      if ((await call('dbg_kills')) > k0) return true;
      const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = bx - x, dy = by - y;
      const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
      await page.keyboard.down(dir); await sleep(200);
      await key('Space', 110); await key('Space', 110);
      await page.keyboard.up(dir);
    }
    return (await call('dbg_kills')) > k0;
  }
  async function openStation(x, y, station) {
    await call('dbg_tp', x, y); await sleep(250);
    for (let i = 0; i < 5 && (await call('dbg_state')) !== 2; i++) await key('KeyE', 250);
    return (await call('dbg_state')) === 2 && (await call('dbg_station')) === station;
  }
  // title -> LOAD -> pick slot n. The picker PRESELECTS the last
  // played slot (open_slots: G.slot-1, persisted in the global
  // blob), so the Downs needed are counted from that slot, not
  // from the top of the list.
  async function loadSlot(n, lastPlayed) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await key('Space', 400);   // CONTINUE row -> load picker
    const downs = ((n - lastPlayed) % 4 + 4) % 4;
    for (let i = 0; i < downs; i++) await key('ArrowDown', 200);
    await key('Enter', 400);
    return (await poll('dbg_state', v => v === 1, 8000)) === 1;
  }

  // ================= PHASE A: the full walk on fresh slot 1 =====
  ok((await call('dbg_state')) === 0, 'A: boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'A: fresh run started');
  ok((await call('dbg_fs')) === 10, 'A: chain armed at step 1 (squash a bug)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 0, 'A: tuition pool starts empty');

  ok(await killOneBug(), 'A: first bug squashed by hand');
  ok((await poll('dbg_fs', v => v === 11)) === 11, 'A: prompt advanced to step 2 (hat)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 5, 'A: step 1 XP pooled (no buddy yet)', await call('dbg_fspool'));

  await call('dbg_feed');   // one 10 kg carcass on the head
  ok((await poll('dbg_fs', v => v === 12)) === 12, 'A: prompt advanced to step 3 (wall)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 10, 'A: pool holds 10 XP', await call('dbg_fspool'));

  await call('dbg_feed'); await call('dbg_feed');   // stack of 3 carcasses
  await key('KeyF', 250);                            // build the wall
  ok((await poll('dbg_walls', v => v >= 1)) >= 1, 'A: carcass wall built');
  ok((await poll('dbg_fs', v => v === 13)) === 13, 'A: prompt advanced to step 4 (buy)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 20, 'A: pool holds 20 XP', await call('dbg_fspool'));

  // the marker shot: out on the field, the prompt names the shop
  // and the arrow points back toward the mall hut
  await call('dbg_tp', 300, 300); await sleep(500);
  { const cv = await page.$('canvas');
    await cv.screenshot({ path: __dirname + '/tsg-v125-after-marker.png' }); }

  await call('dbg_feed'); await call('dbg_feed'); await call('dbg_feed');   // 30 kg to spend
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', v => v === 1)) === 1, 'A: entered the mall');
  ok(await openStation(-315, -6148, 0), 'A: Counter panel open');   // MALL_SX[0]=-315, disc row MALL_CY-148 (v1.25 MALL ENLARGEMENT)
  ok((await call('dbg_storesel')) === 0, 'A: cursor on the first row');
  await key('Space', 250);   // buy a second keyboard copy (20 kg) —
  // the till counting itself is tilltest's beat; here the purchase
  // is proven by the chain noticing it (fingerprint) and paying out
  ok((await poll('dbg_fs', v => v === 14, 30000)) === 14, 'A: prompt advanced to step 5 (bank)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 30, 'A: pool holds 30 XP', await call('dbg_fspool'));
  await key('Escape', 300);

  ok(await openStation(105, -6148, 2), 'A: Bug Bank panel open');   // MALL_SX[2]=105
  await key('Space', 400);   // DEPOSIT ALL (10 kg left on the hat)
  ok((await poll('dbg_bank', v => v > 0)) > 0, 'A: carcasses banked', await call('dbg_bank'));
  ok((await poll('dbg_fs', v => v === 15)) === 15, 'A: prompt advanced to step 6 (cabinet)', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 45, 'A: pool holds 45 XP', await call('dbg_fspool'));
  await key('Escape', 300);
  const bankA = await call('dbg_bank');

  await call('dbg_adopt', 0);   // setup: a buddy, so a cabinet will open
  ok((await poll('dbg_fspool', v => v === 0)) === 0, 'A: pool flushed on adoption');
  ok((await call('dbg_buddyxp')) === 45, 'A: buddy received the pooled 45 XP', await call('dbg_buddyxp'));

  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  ok((await poll('dbg_inarcade', v => v === 1)) === 1, 'A: entered the arcade');
  await call('dbg_tp', 0, -12108); await sleep(250);   // SUPER BUDDY RUN cabinet
  ok((await call('dbg_cabinet')) === 1, 'A: standing at the RUN cabinet', await call('dbg_cabinet'));
  await key('KeyE', 250);
  ok((await poll('dbg_arcgate', v => v === 1, 4000)) === 1, 'A: cabinet panel open');
  await key('Space', 300);
  ok((await poll('dbg_fs', v => v === 26, 20000)) === 26, 'A: chain COMPLETE after cabinet entry', await call('dbg_fs'));
  ok((await call('dbg_buddyxp')) === 65, 'A: finale XP paid straight to the buddy (65 total)', await call('dbg_buddyxp'));
  await call('dbg_bwexit'); await sleep(300);
  const killsA = await call('dbg_kills');

  const s1 = await pollSave(1, 'fs=2,6,');
  ok(s1.includes('fs=2,6,'), 'A: the retired chain is what the save file holds', s1.slice(0, 60));
  ok(await loadSlot(1, 1), 'A: slot 1 reloads into play');
  ok((await call('dbg_fs')) === 26, 'A: chain stays retired after reload', await call('dbg_fs'));
  ok((await call('dbg_bank')) === bankA, 'A: bank survived the reload', await call('dbg_bank'));
  ok((await call('dbg_buddyxp')) === 65, 'A: buddy XP survived the reload', await call('dbg_buddyxp'));

  // ================= PHASE B: resume mid-chain, then SKIP =======
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('ArrowDown', 200);   // START row (CONTINUE sits above it now)
  await key('Space', 400);       // -> new-game slot picker
  await key('ArrowDown', 200);   // slot 2
  await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'B: fresh slot 2 started');
  ok((await call('dbg_fs')) === 10, 'B: chain armed on slot 2 as well', await call('dbg_fs'));
  ok(await killOneBug(), 'B: first bug squashed on slot 2');
  ok((await poll('dbg_fs', v => v === 11)) === 11, 'B: step 2 reached', await call('dbg_fs'));
  const s2 = await pollSave(2, 'fs=1,1,');
  ok(s2.includes('fs=1,1,'), 'B: mid-chain state reached the save file');
  ok(await loadSlot(2, 2), 'B: slot 2 reloads into play');
  ok((await call('dbg_fs')) === 11, 'B: chain resumed mid-chain at step 2', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 5, 'B: pooled XP resumed too', await call('dbg_fspool'));

  await key('Escape', 350);
  ok((await call('dbg_state')) === 5, 'B: pause menu open');
  await key('ArrowDown', 150); await key('ArrowDown', 150); await key('ArrowDown', 150);
  ok((await call('dbg_pausesel')) === 3, 'B: fourth pause row is reachable (SKIP FIRST SHIFT)', await call('dbg_pausesel'));
  await key('Space', 400);
  ok((await call('dbg_state')) === 1, 'B: skip returns to play');
  // dbg_fs = state*10+step: state 3 = skipped (the step digit is
  // wherever the chain happened to stop — here 1, so 31)
  ok(Math.floor((await call('dbg_fs')) / 10) === 3, 'B: chain SKIPPED via the pause row', await call('dbg_fs'));
  // a Continue resumes in the mall, and bugs don't fight there —
  // step out the exit mat before the post-skip squash
  await call('dbg_tp', 0, -5810); await sleep(250);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', v => v === 0)) === 0, 'B: stepped out of the mall');
  ok(await killOneBug(), 'B: another bug squashed after the skip');
  await sleep(600);
  ok(Math.floor((await call('dbg_fs')) / 10) === 3, 'B: skipped chain never comes back', await call('dbg_fs'));
  ok((await call('dbg_fspool')) === 5, 'B: no further XP after the skip', await call('dbg_fspool'));

  // ================= PHASE C: a v1.24-format save ===============
  const stripped = ((await saveSlot(1)) || '')
    .replace(/^fs=.*$/m, '')
    .replace(/ fp=\d+/g, '');
  ok(stripped.length > 0 && !/^fs=/m.test(stripped), 'C: stripped save carries no fs= line');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), stripped);
  ok(await loadSlot(3, 2), 'C: old-format slot 3 loads into play');
  ok((await call('dbg_fs')) === 0, 'C: NO chain on a v1.24 save', await call('dbg_fs'));
  ok((await call('dbg_bank')) === bankA, 'C: bank intact from the old save', await call('dbg_bank'));
  ok((await call('dbg_buddyxp')) === 65, 'C: buddy XP intact from the old save', await call('dbg_buddyxp'));
  ok((await call('dbg_kills')) === killsA, 'C: kill total intact from the old save', await call('dbg_kills'));
  await key('Escape', 350);
  ok((await call('dbg_state')) === 5, 'C: pause menu open on the old save');
  await key('ArrowDown', 150); await key('ArrowDown', 150); await key('ArrowDown', 150);
  ok((await call('dbg_pausesel')) === 0, 'C: pause menu still has exactly 3 rows (selection wrapped)', await call('dbg_pausesel'));
  await key('Escape', 300);
  ok((await call('dbg_state')) === 1, 'C: back in play');

  console.log(`firstshift: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
