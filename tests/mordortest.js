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
  async function steerTo(getXY, timeout = 20000) {
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

  console.log(`MORDORTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', e && e.stack ? e.stack.slice(0, 600) : e); process.exit(2); });
