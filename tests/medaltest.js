// medaltest.js — v1.25 MEDAL TIMES (feature #3): per-track Cart bests
// and per-level Run bests, medals DERIVED from the stored bests.
// v1.27: Cart medals are now EARNED at the finish, capped by
// placement (podium only — medal_tier_finish), and banked per track
// (md= token). Phase A still asserts the exact tier boundaries via a
// crafted save: a save with mc= but no md= is pre-v1.27, so its
// medals MIGRATE from the times at load — the same pure derivation,
// asserted exactly through dbg_crtmedal/dbg_platmedal. Phase F is
// the new podium boundary on live finishes. (Run medals are a solo
// time trial and still derive from the stored bests.)
// Phase B: a real skip-driven Cart finish banks a per-track best via
// the shared race clock; a slower re-run does not replace it; the blob
// carries mc=.
// Phase C: a real flag-touch Run finish banks a per-level best; with
// plat_done >= 1 the entry LEVEL SELECT appears, picks replay level 1,
// and a slower replay does not replace the best.
// Phase D: a pre-feature save (mc=/mr= stripped by hand) loads with no
// medals and bank/XP/kills/legacy records/plat_done intact.
// Phase E: a Zen run (the save's zen=1) finishes a race and a level
// and banks NOTHING.
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
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const hold = async (k, ms) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); };
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
  async function loadSlot(n, lastPlayed) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await key('Space', 400);
    const downs = ((n - lastPlayed) % 4 + 4) % 4;
    for (let i = 0; i < downs; i++) await key('ArrowDown', 200);
    await key('Enter', 400);
    return (await poll('dbg_state', v => v === 1, 8000)) === 1;
  }
  // enter the Cart cabinet and start a solo race from the waiting room
  async function startRace() {
    await call('dbg_bwentm', 7);
    await poll('dbg_crtstate', v => v === 3, 6000);
    await sleep(700);
    for (let tries = 0; tries < 3; tries++) {
      await key('Space', 300);
      const st = await poll('dbg_crtstate', v => v === 0 || v === 1, 4000);
      if (st === 0 || st === 1) break;
    }
    return (await poll('dbg_crtstate', v => v === 1, 9000)) === 1;
  }
  // land a race finish at about dsTarget: skip to the last straight,
  // set the race clock, drive in, wait for the standings (record banked)
  async function finishRaceAt(dsTarget) {
    await call('dbg_crtskip');
    await call('dbg_crtsettime', dsTarget);
    await hold('Space', 1600);
    return (await poll('dbg_crtstate', v => v === 4 || v === 5, 12000)) >= 4;
  }
  // touch the Run flag at about dsTarget on the current level
  async function finishLevelAt(dsTarget) {
    await call('dbg_platsettime', dsTarget);
    const fx = await call('dbg_platflagx');
    await call('dbg_bwtp', fx, 150);
    await sleep(400);
  }

  // ================= PHASE A: exact boundaries via crafted save ====
  ok((await call('dbg_state')) === 0, 'A: boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'A: fresh run started');
  await call('dbg_adopt', 0);
  const blobA = await pollSave(1, 'bk=');
  ok(blobA.includes('pl= 0 '), 'A: slot 1 save carries the P1 line', blobA.slice(0, 60));
  const MC = '251,295,350,459,0,0,0,0';       // t0 gold-exact, t1 silver (gold+1), t2 bronze (silver+1), t3 none (bronze+1) — v1.33: boundaries moved with the re-derived medal table (AI pace 158 -> 145)
  const MR = '261,294,379,509,0,0,0,0,0,0,0,0,0,0,0,0'; // L1 gold-exact, L2 silver, L3 bronze, L4 none
  const crafted = blobA.split('\n').map(line =>
    line.startsWith('pl= 0 ') ? line + ' mc=' + MC + ' mr=' + MR : line).join('\n');
  ok(crafted.includes('mc=' + MC), 'A: crafted blob carries the mc= token');
  await page.evaluate(s => localStorage.setItem('tsg_save2', s), crafted);
  ok(await loadSlot(2, 1), 'A: crafted slot 2 loads into play');
  ok((await call('dbg_crttbest', 0)) === 251, 'A: cart t0 best = 251ds (gold boundary)', await call('dbg_crttbest', 0));
  ok((await call('dbg_crtmedal', 0)) === 3, 'A: t0 at gold boundary -> GOLD', await call('dbg_crtmedal', 0));
  ok((await call('dbg_crtmedal', 1)) === 2, 'A: t1 at gold+1 -> SILVER', await call('dbg_crtmedal', 1));
  ok((await call('dbg_crtmedal', 2)) === 1, 'A: t2 at silver+1 -> BRONZE', await call('dbg_crtmedal', 2));
  ok((await call('dbg_crtmedal', 3)) === 0, 'A: t3 at bronze+1 -> no medal', await call('dbg_crtmedal', 3));
  ok((await call('dbg_crtmedal', 4)) === 0, 'A: t4 unset -> no medal');
  ok((await call('dbg_plattbest', 1)) === 261, 'A: run L1 best = 261ds (gold boundary)', await call('dbg_plattbest', 1));
  ok((await call('dbg_platmedal', 1)) === 3, 'A: L1 at gold boundary -> GOLD', await call('dbg_platmedal', 1));
  ok((await call('dbg_platmedal', 2)) === 2, 'A: L2 at gold+1 -> SILVER', await call('dbg_platmedal', 2));
  ok((await call('dbg_platmedal', 3)) === 1, 'A: L3 at silver+1 -> BRONZE', await call('dbg_platmedal', 3));
  ok((await call('dbg_platmedal', 4)) === 0, 'A: L4 at bronze+1 -> no medal', await call('dbg_platmedal', 4));
  ok((await call('dbg_platmedal', 16)) === 0, 'A: L16 unset -> no medal');

  // ================= PHASE B: a real Cart finish banks a best ======
  ok(await startRace(), 'B: home cup race 1 running');
  ok((await call('dbg_crttrack')) === 0, 'B: race 1 is track 0');
  ok(await finishRaceAt(200), 'B: finish banked, standings up');
  const tb0 = await call('dbg_crttbest', 0);
  ok(tb0 > 200 && tb0 <= 251, 'B: t0 best replaced by the faster real finish', tb0);
  ok((await call('dbg_crtmedal', 0)) === 3, 'B: t0 medal still GOLD');
  const blobB = await pollSave(2, 'mc=');
  ok(blobB.includes('mc='), 'B: slot 2 blob persists the mc= token');
  // a slower run at the same track must NOT replace the best
  await call('dbg_bwexit');
  await sleep(400);
  ok(await startRace(), 'B: re-entered, race 1 running again');
  ok(await finishRaceAt(900), 'B: slow finish banked');
  ok((await call('dbg_crttbest', 0)) === tb0, 'B: slow run did not replace the t0 best', await call('dbg_crttbest', 0));
  await call('dbg_bwexit');
  await sleep(400);

  // ================= PHASE C: Run finish + the level select ========
  await call('dbg_bwentm', 3);
  ok((await poll('dbg_bwmode', v => v === 3, 6000)) === 3, 'C: entered Super Buddy Run');
  ok((await call('dbg_platsel')) === 0, 'C: no select for a first-time runner (plat_done = 0)');
  ok((await call('dbg_platlvl')) === 1, 'C: level 1 loaded');
  await finishLevelAt(250);
  ok((await poll('dbg_platdone', v => v === 1, 12000)) === 1, 'C: level 1 finished, plat_done = 1');
  const tl1 = await call('dbg_plattbest', 1);
  ok(tl1 >= 250 && tl1 <= 261, 'C: L1 best banked at the flag touch', tl1);
  ok((await call('dbg_platmedal', 1)) === 3, 'C: L1 medal GOLD');
  await call('dbg_bwexit');
  await sleep(400);
  await call('dbg_bwentm', 3);
  ok((await poll('dbg_platsel', v => v === 1, 6000)) === 1, 'C: select appears for a returning runner');
  ok((await call('dbg_platpick')) === 2, 'C: frontier level preselected', await call('dbg_platpick'));
  await key('ArrowLeft', 250);
  ok((await call('dbg_platpick')) === 1, 'C: LEFT picks level 1');
  await key('ArrowLeft', 250);
  ok((await call('dbg_platpick')) === 1, 'C: pick clamps at level 1');
  await sleep(500);
  await key('Space', 300);
  ok((await poll('dbg_platsel', v => v === 0, 6000)) === 0, 'C: FIRE starts the picked level');
  ok((await call('dbg_platlvl')) === 1, 'C: replaying level 1');
  await finishLevelAt(400);
  ok((await poll('dbg_platlvl', v => v === 2, 12000)) === 2, 'C: replay finished, advanced');
  ok((await call('dbg_plattbest', 1)) === tl1, 'C: slower replay did not replace the L1 best', await call('dbg_plattbest', 1));
  await call('dbg_bwexit');
  await sleep(400);

  // ================= PHASE D: a pre-feature save loads clean =======
  const bankD = await call('dbg_bank'), xpD = await call('dbg_buddyxp'),
        killsD = await call('dbg_kills'), legacyD = await call('dbg_crtbesttime'),
        pdD = await call('dbg_platdone');
  const blobD = (await saveSlot(2)) || '';
  ok(blobD.includes('mc=') && blobD.includes('mr='), 'D: slot 2 blob carries both medal tokens');
  const stripped = blobD.replace(/ mc=[0-9,]+/g, '').replace(/ mr=[0-9,]+/g, '').replace(/ md=[0-9,]+/g, '');
  ok(!stripped.includes('mc=') && !stripped.includes('mr=') && !stripped.includes(' md='), 'D: stripped blob is medal-free');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), stripped);
  ok(await loadSlot(3, 2), 'D: pre-feature slot 3 loads into play');
  ok((await call('dbg_crttbest', 0)) === 0 && (await call('dbg_crtmedal', 0)) === 0, 'D: no cart medals on the old save');
  ok((await call('dbg_plattbest', 1)) === 0 && (await call('dbg_platmedal', 1)) === 0, 'D: no run medals on the old save');
  ok((await call('dbg_bank')) === bankD, 'D: bank intact', await call('dbg_bank'));
  ok((await call('dbg_buddyxp')) === xpD, 'D: buddy XP intact', await call('dbg_buddyxp'));
  ok((await call('dbg_kills')) === killsD, 'D: kills intact', await call('dbg_kills'));
  ok((await call('dbg_crtbesttime')) === legacyD, 'D: legacy cart best intact', await call('dbg_crtbesttime'));
  ok((await call('dbg_platdone')) === pdD, 'D: plat_done intact', await call('dbg_platdone'));

  // ================= PHASE E: a Zen run earns nothing ==============
  // Zen is a TITLE-menu toggle copied into the run at a FRESH start
  // (a loaded slot restores its own saved zrun=), so: reload to the
  // title, toggle ZEN on like a player, then START a new run in
  // slot 3 — the picker preselects it (last played), and an occupied
  // slot in START mode takes two Enters (overwrite confirm).
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  ok((await call('dbg_mzen')) === 0, 'E: Zen preference starts off');
  // boot-settle (campaign resume wave; the Nov 19 trophycasetest
  // triage's standing rule): the wasm exports answer before
  // game_init's first frame, so a bare menucount read can catch
  // the zeroed-state transient (a NO-save 6-row title) even with
  // saves in every slot — this suite's battery-only 71/1 face.
  // Poll until two reads 300ms apart agree, then assert the same
  // >= 7 below. Assertion strength unchanged.
  let mcount = await call('dbg_menucount');
  { const t0 = Date.now();
    while (Date.now() - t0 < 15000) { await sleep(300); const v = await call('dbg_menucount'); if (v === mcount) break; mcount = v; } }
  const msel0 = await call('dbg_menusel');
  const zenRow = 5;   // has_save title rows: CONTINUE, START, HOW TO, SETTINGS, DIFFICULTY, ZEN, EDDIE (a co-op row, when shown, appends after EDDIE)
  ok(mcount >= 7, 'E: title shows the full row set', mcount);
  for (let i = 0; i < ((zenRow - msel0) % mcount + mcount) % mcount; i++) await key('ArrowDown', 180);
  ok((await call('dbg_menusel')) === zenRow, 'E: selection on the ZEN row', await call('dbg_menusel'));
  await key('Space', 350);
  ok((await call('dbg_mzen')) === 1, 'E: Zen toggled on from the title', await call('dbg_mzen'));
  for (let i = 0; i < zenRow - 1; i++) await key('ArrowUp', 150);
  ok((await call('dbg_menusel')) === 1, 'E: selection on START');
  await key('Space', 400);   // the slot picker, preselecting slot 3
  ok((await call('dbg_state')) === 7, 'E: slot picker open', await call('dbg_state'));
  await key('Enter', 350);   // arm the overwrite confirm
  ok((await call('dbg_state')) === 7, 'E: overwrite confirm armed', await call('dbg_state'));
  await key('Enter', 400);   // confirmed: a fresh run in slot 3
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'E: fresh Zen run started in slot 3');
  await call('dbg_adopt', 0);
  ok((await call('dbg_zen')) === 1, 'E: Zen run is live', await call('dbg_zen'));
  ok(await startRace(), 'E: Zen race running');
  ok(await finishRaceAt(200), 'E: Zen finish banked');
  ok((await call('dbg_crttbest', 0)) === 0, 'E: Zen race banked NO track best', await call('dbg_crttbest', 0));
  ok((await call('dbg_crtmedal', 0)) === 0, 'E: Zen race earned NO medal');
  await call('dbg_bwexit');
  await sleep(400);
  await call('dbg_bwentm', 3);
  ok((await poll('dbg_bwmode', v => v === 3, 6000)) === 3, 'E: entered Run in Zen');
  ok((await call('dbg_platsel')) === 0, 'E: no select on a fresh run (plat_done = 0)');
  await finishLevelAt(250);
  ok((await poll('dbg_platdone', v => v === 1, 12000)) === 1, 'E: Zen clear still advances progression');
  ok((await call('dbg_plattbest', 1)) === 0, 'E: Zen clear banked NO level best', await call('dbg_plattbest', 1));
  ok((await call('dbg_platmedal', 1)) === 0, 'E: Zen clear earned NO medal');
  await call('dbg_bwexit');
  await sleep(400);
  await call('dbg_bwentm', 3);
  ok((await poll('dbg_platsel', v => v === 1, 6000)) === 1, 'E: select offered on the Zen re-entry');
  await key('ArrowLeft', 250);
  await sleep(500);
  await key('Space', 300);
  ok((await poll('dbg_platsel', v => v === 0, 6000)) === 0, 'E: replaying level 1 in Zen');
  await finishLevelAt(250);
  ok((await poll('dbg_platlvl', v => v === 2, 12000)) === 2, 'E: Zen replay finished');
  ok((await call('dbg_plattbest', 1)) === 0, 'E: Zen replay banked NO level best', await call('dbg_plattbest', 1));
  await call('dbg_bwexit');

  // ================= PHASE F: the v1.27 podium boundary ============
  // A Cart finish earns its time tier CAPPED by placement: 3rd with
  // a gold-tier clock earns BRONZE; 4th with an even faster clock
  // earns NOTHING (the time still banks as a time). The fresh run
  // overwrites slot 3 (Phase E's Zen run, no longer needed) so the
  // medal record starts empty and every step is observable.
  // Rival finishes are staged with dbg_crtfin (a rival parked past
  // the line registers its place on the next frame).
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  // Title navigation is verify-and-retry throughout this phase: a
  // bare key press can fall between frames right after a reload
  // (the hazard Phases D/E document — the first version of this
  // phase lost exactly one ArrowDown that way), so the selection is
  // driven to its row by polling, never by counting presses.
  { let mc2 = await call('dbg_menucount');
    const t0 = Date.now();
    while (Date.now() - t0 < 15000) { await sleep(300); const v = await call('dbg_menucount'); if (v === mc2) break; mc2 = v; } }
  async function menuGoto(row) {
    for (let t = 0; t < 14; t++) {
      const sel = await call('dbg_menusel'), cnt = await call('dbg_menucount');
      if (sel === row) return true;
      const down = ((row - sel) % cnt + cnt) % cnt, up = (cnt - down) % cnt;
      await key(down <= up ? 'ArrowDown' : 'ArrowUp', 160);
    }
    return (await call('dbg_menusel')) === row;
  }
  if ((await call('dbg_mzen')) === 1) {
    // Phase E left the Zen preference on; a fresh start would copy
    // it in and bank nothing — turn it back off like a player.
    ok(await menuGoto(5), 'F: selection reached the ZEN row');
    await key('Space', 350);
    ok((await poll('dbg_mzen', v => v === 0, 4000)) === 0, 'F: Zen toggled back off');
  }
  ok((await call('dbg_mzen')) === 0, 'F: Zen preference off for the fresh run');
  ok(await menuGoto(1), 'F: selection on START', await call('dbg_menusel'));
  // The game has THREE slots; the picker preselects the current
  // one (slot 3, Phase E's Zen run) and an occupied slot in START
  // mode takes two Enters — the first arms the overwrite confirm,
  // the second starts fresh. (An earlier draft of this phase
  // aimed at a "slot 4" that does not exist: picker row 3 is the
  // BACK row.) Enters are fed one at a time against the live
  // state, so a press eaten between frames just means one more
  // Enter, never a wrong action: in the picker an Enter can only
  // arm or confirm THIS slot's overwrite. Freshness is then
  // proven by the seams (no medal, no best finish, not Zen).
  await key('Space', 400);   // the slot picker, preselecting slot 3
  ok((await poll('dbg_state', v => v === 7, 4000)) === 7, 'F: slot picker open', await call('dbg_state'));
  let startedF = false;
  for (let e = 0; e < 4 && !startedF; e++) {
    await key('Enter', 400);
    startedF = (await poll('dbg_state', v => v === 1, 3000)) === 1;
  }
  ok(startedF, 'F: fresh run started (slot 3 overwritten)');
  ok((await call('dbg_zen')) === 0, 'F: the fresh run is not Zen', await call('dbg_zen'));
  ok((await call('dbg_crtbest')) === 0, 'F: no best finish on the fresh buddy', await call('dbg_crtbest'));
  await call('dbg_adopt', 0);
  ok((await call('dbg_crtmedal', 0)) === 0, 'F: no medal on the fresh buddy');
  // P3 with a gold-tier clock: two rivals staged home first
  ok(await startRace(), 'F: race 1 running (track 0)');
  await call('dbg_crtskip');
  await call('dbg_crtfin', 1); await call('dbg_crtfin', 2);
  await sleep(300);
  await call('dbg_crtsettime', 150);
  await hold('Space', 1600);
  ok((await poll('dbg_crtstate', v => v === 4 || v === 5, 12000)) >= 4, 'F: P3 finish banked, standings up');
  ok((await call('dbg_crtbest')) === 3, 'F: the finish really was P3', await call('dbg_crtbest'));
  const tbF = await call('dbg_crttbest', 0);
  ok(tbF > 150 && tbF <= 251, 'F: the gold-tier time banked', tbF);
  ok((await call('dbg_crtmedal', 0)) === 1, 'F: P3 caps the gold time at BRONZE', await call('dbg_crtmedal', 0));
  await call('dbg_bwexit'); await sleep(400);
  // P4 with an even faster clock: the time improves, the medal cannot
  ok(await startRace(), 'F: race 2 running');
  await call('dbg_crtskip');
  await call('dbg_crtfin', 1); await call('dbg_crtfin', 2); await call('dbg_crtfin', 3);
  await sleep(300);
  await call('dbg_crtsettime', 140);
  await hold('Space', 1600);
  ok((await poll('dbg_crtstate', v => v === 4 || v === 5, 12000)) >= 4, 'F: P4 finish banked, standings up');
  ok((await call('dbg_crttbest', 0)) < tbF, 'F: the faster P4 time replaced the track best', await call('dbg_crttbest', 0));
  ok((await call('dbg_crtmedal', 0)) === 1, 'F: P4 earned NO medal — still the P3 bronze', await call('dbg_crtmedal', 0));
  await call('dbg_bwexit'); await sleep(400);

  console.log(`\nMEDALTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
