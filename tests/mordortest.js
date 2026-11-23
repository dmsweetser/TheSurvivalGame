// mordortest.js — MORDOR FUN RUN (v1.28, Daniel): the Arcade's
// eighth cabinet. Top-down, your buddy runs the Shire -> Mordor
// road (1000 MOOSE at full length), fights bad guys in real time,
// banks distance at moose-marker milestones, and — the core
// requirement — SAVES AND RESUMES an in-progress run.
//
// The suite shortens the road through the dbg_mortotal seam (10
// moose = 4600 units, milestones every 230 units) BEFORE entering,
// so the whole arc fits in a test run; the player path itself
// (cabinet panel, arrow-key running, Space swings, the RESUME
// panel) is walked for real throughout. Save staging follows the
// house rules: the mid-run save is written with realistic state
// (run underway, a milestone banked, HP not full), and state is
// only ever sampled while the game is settled — after a reload we
// read at the cabinet panel, where the mode's per-frame writes are
// parked by design.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 10000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const saveSlot = n => page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n);
  const pollSave = async (n, needle, ms = 15000) => { const t0 = Date.now(); let s = ''; while (Date.now() - t0 < ms) { s = (await saveSlot(n)) || ''; if (s.includes(needle)) return s; await sleep(300); } return s; };
  async function loadSlot(n, lastPlayed) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await call('dbg_mortotal', 10);   // the shortened road is a wasm-side static: a reload resets it, so every phase re-applies it (test-only seam)
    await key('Space', 400);
    const downs = ((n - lastPlayed) % 4 + 4) % 4;
    for (let i = 0; i < downs; i++) await key('ArrowDown', 200);
    await key('Enter', 400);
    return (await poll('dbg_state', v => v === 1, 8000)) === 1;
  }
  async function enterMordor() {
    await call('dbg_arcade'); await sleep(300);
    // cabinet 7 (v1.28): west end of WAR!'s row — interact point
    // (ARC_SX[7], ARC_CY + ARC_ROWY[7] + 58) = (-256, -11758)
    await call('dbg_tp', -256, -11758); await sleep(200);
    if ((await call('dbg_cabinet')) !== 7) return false;
    await key('KeyE', 250);
    if ((await poll('dbg_arcgate', v => v === 1, 3000)) !== 1) return false;
    await key('Space', 300);
    return (await poll('dbg_bwmode', v => v === 9, 4000)) === 9;
  }
  // pulse-steer the runner toward a point, polling as we go (the
  // Nov 6 rule: never assert on a fixed-duration walk)
  async function steerTo(getXY, timeout = 20000) {   // eslint-disable-line no-unused-vars
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const [tx, ty] = await getXY();
      if (tx < 0) return false;
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      const dx = tx - cx, dy = ty - cy;
      if (Math.abs(dx) <= 14 && Math.abs(dy) <= 14) return true;
      const k2 = Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'ArrowLeft' : 'ArrowRight') : (dy < 0 ? 'ArrowUp' : 'ArrowDown');
      await page.keyboard.down(k2); await sleep(120); await page.keyboard.up(k2);
      await sleep(50);
    }
    return false;
  }
  const runNorthUntil = async (fn, pred, ms = 20000) => {
    const t0 = Date.now(); let v;
    await page.keyboard.down('ArrowUp');
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) break; await sleep(150); }
    await page.keyboard.up('ArrowUp');
    return v;
  };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  await call('dbg_mortotal', 10);   // the test road: 10 moose = 4600 units, a marker every 230

  // ---- the cabinet stands where the spec put it, and nobody moved ----
  await call('dbg_arcade'); await sleep(300);
  T('in the arcade', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(200);
  T('WAR! is still cabinet 6 on its raised row', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  await call('dbg_tp', -256, -11758); await sleep(200);
  T('standing at the Mordor cabinet (cabinet 7)', (await call('dbg_cabinet')) === 7, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('mordor cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('MORDOR FUN RUN launched (bw_mode 9)', (await poll('dbg_bwmode', v => v === 9, 4000)) === 9, await call('dbg_bwmode'));

  // ---- a fresh entry starts a fresh run at the Shire ----
  T('no resume panel on a first entry', (await call('dbg_morpanel')) === 0, await call('dbg_morpanel'));
  T('a run is on the road', (await call('dbg_moron')) === 1, await call('dbg_moron'));
  T('distance starts at the Shire (0)', (await call('dbg_mordist')) === 0, await call('dbg_mordist'));
  const hpMax = await call('dbg_morhp');
  T('HP starts full', hpMax > 0, hpMax);
  await page.screenshot({ path: 'mordor-start.png' });

  // ---- v1.29 road powerups, part 1: the first station is health,
  // early, and collecting it at full HP caps instead of overhealing.
  // Stations are a hash of (seed, k) — 50 of them at total/50 spacing
  // (92 units on this road); station 0 is always HEALTH at 0.22 of
  // the first slot, inside the first moose. ----
  const steerPow = async (timeout = 20000) => {   // steer onto the nearest untaken powerup (world seams; wy>0 is the live-target test)
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const tx = await call('dbg_morpowwx'), ty = await call('dbg_morpowy');
      if (ty <= 0) return false;
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      const dx = tx - cx, dy = ty - cy;
      if (Math.abs(dx) <= 14 && Math.abs(dy) <= 14) return true;
      const k2 = Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'ArrowLeft' : 'ArrowRight') : (dy < 0 ? 'ArrowUp' : 'ArrowDown');
      await page.keyboard.down(k2); await sleep(120); await page.keyboard.up(k2);
      await sleep(50);
    }
    return false;
  };
  // station 0, read straight from the derivation: always HEALTH,
  // early in the opening stretch (0.22 of the first 92-unit slot on
  // this road; 2,024 units in on the full one).
  T('v1.29: station 0 is HEALTH — the road’s first gift is always aid', (await call('dbg_morpow0kind')) === 0, await call('dbg_morpow0kind'));
  T('v1.29: station 0 is early — inside the first moose (s < 460)',
    (await call('dbg_morpow0s')) >= 0 && (await call('dbg_morpow0s')) < 460, await call('dbg_morpow0s'));
  // station 0 stands 20 units from the start line on the shortened
  // road — inside the 30-unit collection radius — so the run's first
  // frames collect it. At full HP its +50 caps.
  T('v1.29: the first powerup is collected at the start line', (await poll('dbg_morpowtaken', v => v === 1, 4000)) === 1, await call('dbg_morpowtaken'));
  T('v1.29: health at full HP caps at max (no overheal)', (await call('dbg_morhp')) === hpMax, await call('dbg_morhp'));

  // ---- run: movement banks distance, the first marker banks a milestone ----
  const mile1 = await runNorthUntil('dbg_mormile', v => v >= 1, 25000);
  T('running north banks the first moose marker', mile1 >= 1, 'mile=' + mile1);
  T('distance is past the marker (>= 230 units)', (await call('dbg_mordist')) >= 230, await call('dbg_mordist'));
  const blobMile = await pollSave(1, 'mu=1,');
  T('the milestone save carries the run (mu= token, on=1)', blobMile.includes('mu=1,'), blobMile.slice(0, 80));

  // ---- a real-time fight: steer onto the nearest foe and swing it down ----
  T('foes walk the road', (await poll('dbg_moren', v => v > 0, 8000)) > 0, await call('dbg_moren'));
  const kills0 = await call('dbg_morkills');
  const fight = async () => {
    const t0 = Date.now();
    while (Date.now() - t0 < 30000) {
      if ((await call('dbg_morkills')) > kills0) return true;
      const ex = await call('dbg_morenx'), ey = await call('dbg_moreny');
      if (ex < 0) { await sleep(300); continue; }
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      const dx = ex - cx, dy = ey - cy;
      if (Math.abs(dx) <= 26 && Math.abs(dy) <= 26) {
        // in the swing's inner band: face it and swing, at the weapon's pace
        const face = Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'ArrowLeft' : 'ArrowRight') : (dy < 0 ? 'ArrowUp' : 'ArrowDown');
        await page.keyboard.down(face); await sleep(90); await page.keyboard.up(face);
        await hold('Space', 380);
        await sleep(120);
      } else {
        const k2 = Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'ArrowLeft' : 'ArrowRight') : (dy < 0 ? 'ArrowUp' : 'ArrowDown');
        await page.keyboard.down(k2); await sleep(120); await page.keyboard.up(k2);
        await sleep(40);
      }
    }
    return (await call('dbg_morkills')) > kills0;
  };
  T('a foe dies to real-time swings', await fight(), 'kills=' + (await call('dbg_morkills')));
  await page.screenshot({ path: 'mordor-fight.png' });

  // ---- v1.29 INPUT REGRESSION (Daniel's playtest, Nov 23): "you
  // don't fire via the button — it happens automatically when you
  // move UP." The shared Banzai input fill folded K_UP into sel_held
  // for every mode but Tank (a platformer hold-to-jump translation),
  // and Mordor reads sel_held as its swing trigger — so steering
  // north swung the sword. The fold now belongs to the platformer
  // alone. The swing witness is dbg_morswings, a session counter
  // the swing block alone increments (the buddy's own atk_cd never
  // decays inside Banzai World, so it latches and cannot witness). ----
  const adjFoe = async () => {   // pulse-steer into the swing's inner band (<=20/axis: facing-independent) of the nearest foe, FIRE untouched
    const t0 = Date.now();
    while (Date.now() - t0 < 20000) {
      const ex = await call('dbg_morenx'), ey = await call('dbg_moreny');
      if (ex < 0) { await sleep(300); continue; }
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      const dx = ex - cx, dy = ey - cy;
      if (Math.abs(dx) <= 20 && Math.abs(dy) <= 20) return true;
      const k2 = Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'ArrowLeft' : 'ArrowRight') : (dy < 0 ? 'ArrowUp' : 'ArrowDown');
      await page.keyboard.down(k2); await sleep(110); await page.keyboard.up(k2);
      await sleep(40);
    }
    return false;
  };
  T('a foe is in the swing band for the input regression', await adjFoe());
  { // (a1) UP held with the runner pinned (Up+Down cancel, so the
    // runner stands in the foe's face): K_UP is down the whole time.
    // Under the bug the swing fired at the weapon's pace and the foe
    // died where it stood.
    const k0 = await call('dbg_morkills'), w0 = await call('dbg_morswings');
    await page.keyboard.down('ArrowUp'); await page.keyboard.down('ArrowDown');
    await sleep(3000);
    await page.keyboard.up('ArrowUp'); await page.keyboard.up('ArrowDown');
    T('v1.29: UP held (pinned) never swings', (await call('dbg_morswings')) === w0, (await call('dbg_morswings')) + ' vs ' + w0);
    T('v1.29: UP held (pinned) banks no kill', (await call('dbg_morkills')) === k0, await call('dbg_morkills'));
  }
  { // (a2) UP held on the move: three seconds running north from
    // beside the foe, the foe chasing through the swing band.
    const k0 = await call('dbg_morkills'), w0 = await call('dbg_morswings');
    await page.keyboard.down('ArrowUp');
    await sleep(3000);
    await page.keyboard.up('ArrowUp');
    T('v1.29: UP held (moving) never swings', (await call('dbg_morswings')) === w0, (await call('dbg_morswings')) + ' vs ' + w0);
    T('v1.29: UP held (moving) banks no kill', (await call('dbg_morkills')) === k0, await call('dbg_morkills'));
  }
  { // (b) the button: one FIRE hold in the band lands the same swing
    // the mode always had — damage or a kill, never neither.
    T('a foe is back in the swing band for the button check', await adjFoe());
    const k0 = await call('dbg_morkills'), h0 = await call('dbg_morenhp'), w0 = await call('dbg_morswings');
    await hold('Space', 380);
    await sleep(150);
    const h1 = await call('dbg_morenhp'), k1 = await call('dbg_morkills');
    T('v1.29: FIRE swings — the swing counter moves', (await call('dbg_morswings')) > w0, (await call('dbg_morswings')) + ' vs ' + w0);
    T('v1.29: FIRE swings — the foe takes damage or dies', k1 > k0 || (h1 >= 0 && h1 < h0), `hp ${h0} -> ${h1}, kills ${k0} -> ${k1}`);
  }

  // ---- v1.29 road powerups, part 2: the collection loop. Each
  // station's kind is read through the seam BEFORE stepping on it,
  // then the effect is asserted at full strength: health restores
  // +50 (staged down first), amplification at least doubles a
  // swing's damage, speed raises a timed north-run's distance by
  // the +40% (and lapses back to baseline when the timer dies).
  // The shortened road is DENSE — stations every 92 units — so
  // steering crosses stations constantly: pickups are asserted as
  // increments (never exact counts), damage comparisons are made
  // on foes too healthy for one swing to kill (so a swing's damage
  // reads exactly), stray buffs are waited out before baselines,
  // and speed runs are measured on runways the seam verifies clear
  // of stations ahead. ----
  const oneSwing = async () => { await page.keyboard.down('Space'); await sleep(170); await page.keyboard.up('Space'); await sleep(200); };   // one swing: the 0.34s cooldown covers the press
  const northDelta = async (ms) => { const d0 = await call('dbg_mordist'); await page.keyboard.down('ArrowUp'); await sleep(ms); await page.keyboard.up('ArrowUp'); await sleep(120); return (await call('dbg_mordist')) - d0; };
  const quietSpot = async () => {   // a warp spot whose nearest foe is out of chase range (>320): foes only move inside 300, so the spot stays quiet
    for (const w of [1500, 2200, 2900, 800, 3600, 1200, 2600]) {
      await call('dbg_morwarp', w); await sleep(600);
      const ex = await call('dbg_morenx'), ey = await call('dbg_moreny');
      if (ex < 0) return true;
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      if ((ex - cx) * (ex - cx) + (ey - cy) * (ey - cy) > 320 * 320) return true;
    }
    return false;
  };
  const clearRunway = async (need) => {   // collect forward until no untaken station sits within `need` units ahead of the runner
    for (let i = 0; i < 8; i++) {
      const s = await call('dbg_morpowx'), d = await call('dbg_mordist');
      if (s < 0 || s - d > need) return true;
      if (!(await collectPow())) return false;
    }
    return false;
  };
  const baselineDelta = async () => {   // an unboosted 700ms north-run: no buff alive, runway clear
    for (let round = 0; round < 3; round++) {
      if ((await call('dbg_morspdb')) > 0) { await quietSpot(); if ((await poll('dbg_morspdb', v => v === 0, 35000)) !== 0) return -1; }
      if (await clearRunway(240)) return await northDelta(700);
    }
    return -1;
  };
  const collectPow = async () => {   // warp-assisted collection of the nearest untaken station: stage 100 units south of it (the warp re-derives foes outside their 150 no-spawn bubble), then a short steer on — long steers at staged-down HP faint-cycle on this dense road
    const s = await call('dbg_morpowx');
    if (s < 0) return false;
    await call('dbg_morwarp', Math.max(0, s - 100));
    await sleep(300);
    return await steerPow(9000);
  };
  const driveCollect = async (takenBefore) => {   // keep collecting until the taken count actually rises (a steer can chase a switching "nearest" while crossing other stations — the take is the success, not the arrival)
    for (let i = 0; i < 4; i++) {
      if ((await call('dbg_morpowtaken')) > takenBefore) return true;
      await collectPow();
    }
    return (await call('dbg_morpowtaken')) > takenBefore;
  };
  const strongFoe = async () => {   // adjFoe, but the foe must have >=15 HP: one swing (max ~14 amplified, ~7 plain) can never kill it, so damage reads exactly
    for (let i = 0; i < 5; i++) {
      if (!(await adjFoe())) return false;
      if ((await call('dbg_morenhp')) >= 15) return true;
      await oneSwing();   // finish the weak one and take the next
    }
    return (await call('dbg_morenhp')) >= 15;
  };
  const swingDmg = async () => {   // one swing on the banded foe, HP read PINNED to the foe's array index (the "nearest foe" identity swaps as foes move — an unpinned delta once read 25 for a 7-damage swing). -1 = unmeasurable (the array moved: a death, a cull, a spawn reshuffle)
    const idx = await call('dbg_morenidx');
    if (idx < 0) return -1;
    const hb = await call('dbg_morenhpidx', idx), kb = await call('dbg_morkills');
    if (hb < 0) return -1;
    await oneSwing();
    const ha = await call('dbg_morenhpidx', idx);
    if (ha < 0 || (await call('dbg_morkills')) !== kb) return -1;   // a death that frame can compact the array under the pinned index — spawns cannot (they append), and culls need the runner to move
    return hb - ha;
  };
  const swingDmgRetry = async () => { for (let i = 0; i < 4; i++) { const d = await swingDmg(); if (d >= 0) return d; await strongFoe(); } return -1; };
  let gotHealth = false, gotAmp = false, gotSpeed = false;
  const baselineSwing = async () => {   // a swing measured with NO amp alive. Steering to the foe can cross a stray amp station and re-buff the runner (it did — a "baseline" read 14, the amplified number), so the timer is checked AFTER the steering, and a tainted approach is waited out at a quiet spot and redone.
    for (let round = 0; round < 4; round++) {
      if (!(await strongFoe())) return -1;
      if ((await call('dbg_moramp')) > 0) {
        await quietSpot();
        if ((await poll('dbg_moramp', v => v === 0, 35000)) !== 0) return -1;
        continue;
      }
      for (let att = 0; att < 4; att++) {
        const idx = await call('dbg_morenidx');
        if (idx < 0 || (await call('dbg_morenhpidx', idx)) < 15) break;   // the banded foe is gone or too weak — re-approach (and re-check the timer)
        const d = await swingDmg();
        if (d >= 0) return d;
      }
    }
    return -1;
  };
  const proveAmp = async (dAmpFirst) => {   // the amp comparison, whichever swing the road let us take first
    let dBase = -1, dAmp = -1;
    if (dAmpFirst) {
      T('v1.29 amp: a strong foe is in the band for the amplified swing', await strongFoe());
      dAmp = await swingDmgRetry();
      dBase = await baselineSwing();   // waits the timer out itself, taint-proofed
    } else {
      dBase = await baselineSwing();
    }
    return [dBase, dAmp];
  };
  const proveSpeed = async (d1First) => {   // the speed comparison, likewise order-proofed; returns [D0, D1]
    let D0 = -1, D1 = -1;
    if (d1First) {
      D1 = await northDelta(700);
      T('v1.29 speed: a foe-free stretch for the lapse wait', await quietSpot());
      T('v1.29: the speed timer lapses on its own', (await poll('dbg_morspdb', v => v === 0, 30000)) === 0, await call('dbg_morspdb'));
      D0 = await baselineDelta();
    } else {
      D0 = await baselineDelta();
    }
    return [D0, D1];
  };
  for (let iter = 0; iter < 30 && !(gotHealth && gotAmp && gotSpeed); iter++) {
    // crossing traffic on this dense road starts buffs uninvited —
    // a live buff is a proof opportunity, not a nuisance
    if (!gotAmp && (await call('dbg_moramp')) > 100) {
      const [dB, dA] = await proveAmp(true);
      T('v1.29: amplified damage is at least double the baseline (crossing pickup)', dB > 0 && dA >= 2 * dB - 2, 'base=' + dB + ' amp=' + dA);
      gotAmp = dB > 0 && dA >= 2 * dB - 2;
      continue;
    }
    if (!gotSpeed && (await call('dbg_morspdb')) > 100) {
      const [D0, D1] = await proveSpeed(true);
      T('v1.29: boosted speed covers >1.28x the baseline distance (crossing pickup)', D0 > 0 && D1 > 1.28 * D0, 'D0=' + D0 + ' D1=' + D1);
      if (D0 > 0 && D1 > 1.28 * D0) {
        const D2 = await baselineDelta();
        T('v1.29: after the lapse the pace is baseline again (<1.15x)', D2 > 0 && D2 < 1.15 * D0, 'D0=' + D0 + ' D2=' + D2);
        gotSpeed = D2 > 0 && D2 < 1.15 * D0;
      }
      continue;
    }
    const kind = await call('dbg_morpowkind');
    const takenBefore = await call('dbg_morpowtaken');
    if (kind < 0) break;
    if (kind === 0 && !gotHealth) {
      // stage HP down so the +50 is observable (pulse-style:
      // a foe hit can re-arm the combat grace and block one offer)
      { const t0 = Date.now();
        while (Date.now() - t0 < 8000 && (await call('dbg_morhp')) > hpMax - 50){ await call('dbg_morhurt', 25); await sleep(350); } }
      const hpBefore = await call('dbg_morhp');
      if (hpBefore > hpMax - 50) { T('v1.29: health staging — HP staged below max-50', false, 'hp=' + hpBefore); break; }
      T('v1.29: collected a health powerup', await driveCollect(takenBefore));
      T('v1.29: health pickup registers', (await poll('dbg_morpowtaken', v => v > takenBefore, 4000)) > takenBefore, await call('dbg_morpowtaken'));
      const hpAfter = await call('dbg_morhp');
      T('v1.29: health restores at least +50 (capped at max)', hpAfter >= Math.min(hpBefore + 50, hpMax) && hpAfter <= hpMax, hpBefore + ' -> ' + hpAfter);
      gotHealth = true;
    } else if (kind === 1 && !gotAmp) {
      // deliberate: baseline first, then collect the amp station and
      // swing amplified. (A stray amp from crossing traffic is
      // proven at the loop top instead, before we ever get here.)
      const [dBase] = await proveAmp(false);
      T('v1.29: collected an amplification powerup', await driveCollect(takenBefore));
      T('v1.29: the amp timer is running (30s)', (await call('dbg_moramp')) > 200, await call('dbg_moramp'));
      T('v1.29 amp: a strong foe is in the band for the amplified swing', await strongFoe());
      const dAmp = await swingDmgRetry();
      T('v1.29: amplified damage is at least double the baseline', dBase > 0 && dAmp >= 2 * dBase - 2, 'base=' + dBase + ' amp=' + dAmp);
      gotAmp = true;
    } else if (kind === 2 && !gotSpeed) {
      for (let h = 0; h < 4 && (await call('dbg_morhp')) < 40; h++) { if (!(await driveCollect(await call('dbg_morpowtaken')))) break; await sleep(200); }   // top up before timed runs if the road offers
      const [D0] = await proveSpeed(false);
      T('v1.29 speed: the baseline north-run covered ground', D0 > 80, 'D0=' + D0);
      T('v1.29: collected a speed powerup', await driveCollect(takenBefore));
      T('v1.29: the speed timer is running (20s)', (await call('dbg_morspdb')) > 150, await call('dbg_morspdb'));
      const D1 = await northDelta(700);
      T('v1.29: boosted speed covers >1.28x the baseline distance', D0 > 0 && D1 > 1.28 * D0, 'D0=' + D0 + ' D1=' + D1);
      // expiry: wait the timer out somewhere the foes cannot reach,
      // then the same north-run must fall back to baseline pace
      T('v1.29 speed: a foe-free stretch for the expiry wait', await quietSpot());
      const distWait = await call('dbg_mordist');
      T('v1.29: the speed timer lapses on its own', (await poll('dbg_morspdb', v => v === 0, 30000)) === 0, await call('dbg_morspdb'));
      T('v1.29: no faint during the expiry wait (distance unmoved)', (await call('dbg_mordist')) === distWait && (await call('dbg_morhp')) > 0,
        'dist ' + distWait + ' -> ' + (await call('dbg_mordist')) + ' hp=' + (await call('dbg_morhp')));
      const D2 = await baselineDelta();
      T('v1.29: after expiry the pace is baseline again (<1.15x)', D0 > 0 && D2 > 0 && D2 < 1.15 * D0, 'D0=' + D0 + ' D2=' + D2);
      gotSpeed = true;
    } else {
      // a kind already proven (or a buff the loop top will prove):
      // just collect it and move on
      await driveCollect(takenBefore);
    }
  }
  T('v1.29: all three powerup kinds were collected and proven', gotHealth && gotAmp && gotSpeed,
    'health=' + gotHealth + ' amp=' + gotAmp + ' speed=' + gotSpeed);

  // ---- faint: back to the banked milestone, distance kept, hale again ----
  const mileAtFaint = await call('dbg_mormile');
  // stage the down through the real mor_hurt path, pulse-style: a foe
  // hit can re-arm the 0.4s combat grace at any moment, so one staged
  // hit can be legitimately blocked — keep offering it until it lands
  { const t0 = Date.now();
    while (Date.now() - t0 < 8000 && (await call('dbg_morhp')) > 0){ await call('dbg_morhurt', 9999); await sleep(300); } }
  T('the staged hit downs the runner (hp 0)', (await call('dbg_morhp')) === 0, await call('dbg_morhp'));
  const homeDist = await poll('dbg_mordist', v => v === mileAtFaint * 230, 8000);
  T('faint returns to the banked milestone exactly', homeDist === mileAtFaint * 230, 'dist=' + homeDist + ' mile=' + mileAtFaint);
  T('HP is restored at the marker', (await call('dbg_morhp')) === hpMax, await call('dbg_morhp'));
  T('the milestone itself is never lost', (await call('dbg_mormile')) === mileAtFaint, await call('dbg_mormile'));

  // ---- SAVE / RESUME: a realistic mid-run state survives a full reload ----
  await sleep(1700);   // the faint-home grace (1.5s) lapses
  await hold('ArrowUp', 650);   // mid-stretch again, past the marker
  { const t0 = Date.now();   // HP not full in the saved state — same pulse discipline (grace can block one hit)
    while (Date.now() - t0 < 5000 && (await call('dbg_morhp')) >= hpMax){ await call('dbg_morhurt', 10); await sleep(300); } }
  await call('dbg_bwexit'); await sleep(500);
  T('left the cabinet (the run stays on the road)', (await call('dbg_bwmode')) === -1 && (await call('dbg_moron')) === 1,
    'bwmode=' + (await call('dbg_bwmode')) + ' on=' + (await call('dbg_moron')));
  const distSave = await call('dbg_mordist'), mileSave = await call('dbg_mormile'),
        hpSave = await call('dbg_morhp'), timeSave = await call('dbg_mortime');
  T('saved state is mid-run and realistic (past a marker, HP not full)',
    distSave > mileSave * 230 && hpSave < hpMax && hpSave > 0,
    `dist=${distSave} mile=${mileSave} hp=${hpSave}/${hpMax}`);
  const blobRun = await pollSave(1, 'mu=1,');
  T('the slot save carries the in-progress run', blobRun.includes('mu=1,'), blobRun.slice(0, 80));
  T('the run reloads from the slot', await loadSlot(1, 1), await call('dbg_state'));
  T('entering Mordor again raises the RESUME panel', await enterMordor() && (await poll('dbg_morpanel', v => v === 1, 4000)) === 1,
    'panel=' + (await call('dbg_morpanel')));
  T('distance preserved across the reload', (await call('dbg_mordist')) === distSave, (await call('dbg_mordist')) + ' vs ' + distSave);
  T('milestone preserved across the reload', (await call('dbg_mormile')) === mileSave, await call('dbg_mormile'));
  T('HP at save preserved across the reload', (await call('dbg_morhp')) === hpSave, (await call('dbg_morhp')) + ' vs ' + hpSave);
  const timeBack = await call('dbg_mortime');
  T('elapsed run time preserved across the reload', timeBack >= timeSave && timeBack <= timeSave + 3, timeBack + ' vs ' + timeSave);
  await page.screenshot({ path: 'mordor-resume.png' });
  // RESUME is the panel's first row
  await key('Space', 300);
  T('RESUME closes the panel and keeps the run', (await poll('dbg_morpanel', v => v === 0, 3000)) === 0 && (await call('dbg_moron')) === 1,
    'panel=' + (await call('dbg_morpanel')));
  T('the runner stands where the save left them', Math.abs((await call('dbg_mordist')) - distSave) <= 1, await call('dbg_mordist'));

  // ---- NEW RUN: the confirm is real, then the road starts over ----
  await call('dbg_bwexit'); await sleep(400);
  T('re-entering raises the panel again (run still on)', await enterMordor() && (await poll('dbg_morpanel', v => v === 1, 4000)) === 1,
    'panel=' + (await call('dbg_morpanel')));
  await key('ArrowDown', 200);
  T('panel selection moves to NEW RUN', (await call('dbg_morpsel')) === 1, await call('dbg_morpsel'));
  await key('Space', 300);
  T('the first NEW RUN pick only arms the confirm (run untouched)',
    (await call('dbg_morpanel')) === 1 && (await call('dbg_mordist')) === distSave,
    'panel=' + (await call('dbg_morpanel')) + ' dist=' + (await call('dbg_mordist')));
  await key('Space', 300);
  T('the second pick scraps it: fresh run at the Shire',
    (await poll('dbg_morpanel', v => v === 0, 3000)) === 0 && (await call('dbg_mordist')) === 0 && (await call('dbg_mormile')) === 0 && (await call('dbg_morhp')) === hpMax,
    'dist=' + (await call('dbg_mordist')) + ' mile=' + (await call('dbg_mormile')) + ' hp=' + (await call('dbg_morhp')));

  // ---- the finish: the whole (shortened) road, records, and out ----
  await sleep(1600);   // let the fresh run's clock pass one second: a recorded best is whole seconds, and no real run finishes inside one
  await call('dbg_morwarp', 4600);
  T('reaching Mordor completes the run', (await poll('dbg_mordone', v => v === 1, 8000)) === 1, await call('dbg_mordone'));
  T('the run is off the road after the finish', (await call('dbg_moron')) === 0, await call('dbg_moron'));
  const best1 = await call('dbg_morbest');
  T('a best time is recorded', best1 > 0, best1);
  T('the cabinet lets you back to the arcade after the celebration', (await poll('dbg_bwmode', v => v === -1, 10000)) === -1, await call('dbg_bwmode'));
  const blobDone = await pollSave(1, 'mo=1');
  T('the completion is in the save (mo= token)', blobDone.includes('mo=1'), blobDone.slice(0, 80));

  // ---- Wave-2 round-trip: the records reload exactly, and a finished
  //      run does not haunt the cabinet ----
  T('the records reload from the slot', await loadSlot(1, 1), await call('dbg_state'));
  T('completions round-trip exactly', (await call('dbg_mordone')) === 1, await call('dbg_mordone'));
  T('best time round-trips exactly', (await call('dbg_morbest')) === best1, (await call('dbg_morbest')) + ' vs ' + best1);
  T('no run is in progress after the round-trip', (await call('dbg_moron')) === 0, await call('dbg_moron'));
  T('a finished run raises no panel — a fresh run starts instead',
    await enterMordor() && (await call('dbg_morpanel')) === 0 && (await call('dbg_moron')) === 1 && (await call('dbg_mordist')) === 0,
    'panel=' + (await call('dbg_morpanel')) + ' on=' + (await call('dbg_moron')));
  await page.screenshot({ path: 'mordor-fresh-after-done.png' });

  // ---- v1.29 touch path (STICK scheme, the touchaudit conventions:
  // touch_point exports are the same calls the shell makes for real
  // fingers). The stick feeds K_UP through the very same fill, so a
  // stick pushed UP must steer without swinging; the FIRE zone is
  // the button, and it must swing. ----
  await call('set_mobile', 1);
  await call('dbg_touchctl', 1);
  T('v1.29 touch: STICK scheme on', (await call('dbg_touchctl', -1)) === 1, await call('dbg_touchctl', -1));
  await call('dbg_morwarp', 900);   // staging seam (as the finish phase uses): foes re-derive around a mid-road spot
  await sleep(600);
  const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
  const stickTo = async () => {   // stick-pulse toward the nearest foe, into the inner band
    const t0 = Date.now();
    while (Date.now() - t0 < 25000) {
      const ex = await call('dbg_morenx'), ey = await call('dbg_moreny');
      if (ex < 0) { await sleep(300); continue; }
      const cx = await call('dbg_morx'), cy = await call('dbg_mory');
      const dx = ex - cx, dy = ey - cy;
      if (Math.abs(dx) <= 20 && Math.abs(dy) <= 20) return true;
      const ddx = Math.abs(dx) > Math.abs(dy) ? Math.sign(dx) * 42 : 0;
      const ddy = Math.abs(dx) > Math.abs(dy) ? 0 : Math.sign(dy) * 42;
      await finger(1, 100, 180, 0); await sleep(60);
      await finger(1, 100 + ddx, 180 + ddy, 1); await sleep(240);
      await finger(1, 100 + ddx, 180 + ddy, 2); await sleep(80);
    }
    return false;
  };
  T('v1.29 touch: a foe is in the swing band', await stickTo());
  { // stick pushed straight UP and held beside the foe: the runner
    // runs north (movement is not the bug) but never swings.
    const k0 = await call('dbg_morkills'), w0 = await call('dbg_morswings');
    await finger(1, 100, 180, 0); await sleep(60);
    await finger(1, 100, 138, 1);
    await sleep(2600);
    await finger(1, 100, 138, 2);
    T('v1.29 touch: stick UP held never swings', (await call('dbg_morswings')) === w0, (await call('dbg_morswings')) + ' vs ' + w0);
    T('v1.29 touch: stick UP held banks no kill', (await call('dbg_morkills')) === k0, await call('dbg_morkills'));
  }
  { // the FIRE zone (582,278) is the button: hold it in the band and
    // the swing lands, same as Space.
    T('v1.29 touch: a foe is in the band for the FIRE check', await stickTo());
    const k0 = await call('dbg_morkills'), h0 = await call('dbg_morenhp'), w0 = await call('dbg_morswings');
    await finger(2, 582, 278, 0);
    const swung = (await poll('dbg_morswings', v => v > w0, 2500)) > w0;
    await finger(2, 582, 278, 2);
    await sleep(150);
    const h1 = await call('dbg_morenhp'), k1 = await call('dbg_morkills');
    T('v1.29 touch: the FIRE zone swings', swung);
    T('v1.29 touch: the FIRE zone — the foe takes damage or dies', k1 > k0 || (h1 >= 0 && h1 < h0), `hp ${h0} -> ${h1}, kills ${k0} -> ${k1}`);
  }

  console.log(`MORDORTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', e && e.stack ? e.stack.slice(0, 600) : e); process.exit(2); });
