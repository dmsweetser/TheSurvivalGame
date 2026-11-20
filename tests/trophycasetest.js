// trophycasetest.js — v1.25 TROPHY CASE (feature #4): a Mall station
// that is a pure VIEW over records the game already keeps. The suite
// asserts the panel's actual words through dbg_casename/dbg_caseright/
// dbg_caseblurb — the same helpers the panel draws with.
// Phase B (fresh): a brand-new slot's case is a full panel of labelled,
//   waiting plaques (the empty state), First Shift at STEP 1 OF 6.
// Phase A (crafted): a save with known records shows exactly those —
//   every row's name and right-hand value asserted verbatim.
// Phase C (stripped): the same blob minus the v1.25 tokens (mc=/mr= and
//   the fs= line) loads with records intact; the case renders, medals
//   read NONE YET, the First Shift row is gone, older plaques remain.
// Phase D (Zen): a fresh Zen run finishing a race and a level at gold
//   pace earns NOTHING in the case — the gated surfaces stay empty.
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
  const str = async (fn, arg) => page.evaluate(([f, a]) => {
    const ptr = window.__TSG[f](a);
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]);
    return s;
  }, [fn, arg]);
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
  const pollGlobal = async (needle, ms = 15000) => {
    const t0 = Date.now(); let s = '';
    while (Date.now() - t0 < ms) { s = (await page.evaluate(() => localStorage.getItem('tsg_save'))) || ''; if (s.includes(needle)) return s; await sleep(300); }
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
  // the real player path: teleport to the hut door, INTERACT in, walk
  // the mall floor to the case's stand point, INTERACT the panel open
  async function openCase() {
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 300);
    if ((await call('dbg_mall')) !== 1) return false;
    if (!await walkTo(340, -5868, 14)) return false;
    await key('KeyE', 350);
    return (await poll('dbg_station', v => v === 5, 6000)) === 5;
  }
  async function readCase() {
    const n = await call('dbg_caserows');
    const rows = [];
    for (let i = 0; i < n; i++) rows.push([await str('dbg_casename', i), await str('dbg_caseright', i)]);
    return rows;
  }
  function expectRows(rows, table, tag) {
    ok(rows.length === table.length, `${tag}: row count ${table.length}`, rows.length);
    for (let i = 0; i < table.length && i < rows.length; i++) {
      ok(rows[i][0] === table[i][0], `${tag}: row ${i} name "${table[i][0]}"`, rows[i][0]);
      ok(rows[i][1] === table[i][1], `${tag}: row ${i} (${table[i][0]}) right "${table[i][1]}"`, rows[i][1]);
    }
  }
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
  async function finishRaceAt(dsTarget) {
    await call('dbg_crtskip');
    await call('dbg_crtsettime', dsTarget);
    await hold('Space', 1600);
    return (await poll('dbg_crtstate', v => v === 4 || v === 5, 12000)) >= 4;
  }
  async function finishLevelAt(dsTarget) {
    await call('dbg_platsettime', dsTarget);
    const fx = await call('dbg_platflagx');
    await call('dbg_bwtp', fx, 150);
    await sleep(400);
  }

  // ================= PHASE B: the fresh, empty case ================
  ok((await call('dbg_state')) === 0, 'B: boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'B: fresh run started');
  ok(await openCase(), 'B: walked the mall and opened the TROPHY CASE (station 5)');
  ok((await call('dbg_state')) === 2, 'B: the case panel is a store panel (GS_STORE)');
  const rowsB = await readCase();
  expectRows(rowsB, [
    ['THE CABINETS', ''],
    ['CASTLE', 'NOT YET'],
    ['STORY MODE', 'NOT STARTED'],
    ['WAR!', 'NOT YET'],
    ['CART CUPS', 'HOME -- - FAR --'],
    ['TANK RESCUE', 'NOT YET'],
    ['ENTRANCES', 'NOT YET'],
    ['THE SUBFLOOR', 'NOT YET'],
    ['MEDALS + WALK OF FAME', ''],
    ['CART MEDALS', 'NONE YET'],
    ['RUN MEDALS', 'NONE YET'],
    ['HEAVIEST HAT', 'NO RECORD'],
    ['MOST BUGS SQUASHED', 'NO RECORD'],
    ['BEST BUG BOWL', 'NO RECORD'],
    ['LONGEST RUN', 'NO RECORD'],
    ['THE CREW', ''],
    ['FIRST SHIFT', 'STEP 1 OF 6'],
    ['YOUR BUDDY', 'NONE YET'],
  ], 'B');
  let blurbsOk = true;
  for (let i = 0; i < rowsB.length; i++) { const b = await str('dbg_caseblurb', i); if (!b || b.length < 10) blurbsOk = false; }
  ok(blurbsOk, 'B: every row carries an honest blurb (no blank panel)');
  await page.screenshot({ path: 'tsg-v125-f4-before-case.png' });
  await key('Escape', 300);
  ok((await call('dbg_station')) !== 5 || (await call('dbg_state')) !== 2, 'B: panel closed');

  // ================= PHASE A: the decorated case ===================
  await call('dbg_adopt', 0);
  const blobA = await pollSave(1, 'bk=');
  ok(blobA.includes('pl= 0 '), 'A: slot 1 save carries the P1 line', blobA.slice(0, 60));
  const SQ = 2752682;   // quests 0-5 done (170 + 2<<18 + 2<<20) + Zed bit 17 (131072)
  const setTok = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);
  const crafted = blobA.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    let L = line;
    L = setTok(L, 'sq', SQ); L = setTok(L, 'sq2', 170);
    L = setTok(L, 'er', 20051); L = setTok(L, 'sb', 14);
    L = setTok(L, 'pd', 9); L = setTok(L, 'td', 6);
    L = setTok(L, 'cb', 1250); L = setTok(L, 'cr', 9); L = setTok(L, 'wr', 8);
    L = setTok(L, 'bx', 240); L = setTok(L, 'b1', 3); L = setTok(L, 'b2', 2); L = setTok(L, 'b3', 1);
    L = setTok(L, 'bs', 15); L = setTok(L, 'bn', 26);
    return L + ' mc=230,271,0,0,0,0,0,880 mr=261,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0';
  }).join('\n').replace(/^fs=.*$/m, 'fs=2,6,0');
  ok(crafted.includes('cb=1250') && crafted.includes('wr=8') && crafted.includes('fs=2,6,0'), 'A: crafted blob carries the record tokens');
  const globA = await pollGlobal('rec_hat=');
  const globPatched = globA
    .replace(/^rec_name=.*$/m, 'rec_name=CHAMP')
    .replace(/^rec_hat=.*$/m, 'rec_hat=152.5')
    .replace(/^rec_kills=.*$/m, 'rec_kills=1234')
    .replace(/^rec_bowl=.*$/m, 'rec_bowl=187')
    .replace(/^rec_time=.*$/m, 'rec_time=754');
  ok(globPatched.includes('rec_name=CHAMP') && globPatched.includes('rec_hat=152.5'), 'A: global blob patched with house records');
  await page.evaluate(s => localStorage.setItem('tsg_save', s), globPatched);
  await page.evaluate(s => localStorage.setItem('tsg_save2', s), crafted);
  ok(await loadSlot(2, 1), 'A: crafted slot 2 loads into play');
  ok((await call('dbg_casbest')) === 1250, 'A: castle record loaded', await call('dbg_casbest'));
  ok(await openCase(), 'A: opened the TROPHY CASE on the crafted save');
  const rowsA = await readCase();
  expectRows(rowsA, [
    ['THE CABINETS', ''],
    ['CASTLE', 'BEST 1250'],
    ['STORY MODE', 'ZED DEFEATED'],
    ['WAR!', 'CONQUERED'],
    ['CART CUPS', 'HOME 1ST - FAR 1ST'],
    ['TANK RESCUE', 'LEVEL 6'],
    ['ENTRANCES', 'OUT x2'],
    ['THE SUBFLOOR', 'B-14'],
    ['MEDALS + WALK OF FAME', ''],
    ['CART MEDALS', '1G - 2S - 0B'],
    ['RUN MEDALS', '1G - 0S - 0B'],
    ['HEAVIEST HAT', '152 KG'],
    ['MOST BUGS SQUASHED', '1234'],
    ['BEST BUG BOWL', '187'],
    ['LONGEST RUN', '12:34'],
    ['THE CREW', ''],
    ['FIRST SHIFT', 'COMPLETE'],
    ['BUDDY: FEANOR', 'XP 240'],
    ['TRAINING', 'HP3 ATK2 DEF1'],
  ], 'A');
  ok((await str('dbg_caseblurb', 11)).includes('CHAMP'), 'A: heaviest-hat blurb names the record holder', await str('dbg_caseblurb', 11));
  ok((await str('dbg_caseblurb', 17)).includes('PUPPY'), 'A: buddy blurb names the species', await str('dbg_caseblurb', 17));
  const trainBlurb = await str('dbg_caseblurb', 18);
  ok(trainBlurb.includes('MOVES TAUGHT 4 OF 4'), 'A: training blurb carries the moves taught', trainBlurb);
  ok((await str('dbg_caseblurb', 9)).includes('1 GOLD, 2 SILVER, 0 BRONZE'), 'A: cart medal blurb spells the counts', await str('dbg_caseblurb', 9));
  await page.screenshot({ path: 'tsg-v125-f4-after-case.png' });
  for (let i = 0; i < 10; i++) await key('ArrowDown', 130);
  await sleep(300);
  await page.screenshot({ path: 'tsg-v125-f4-after-case-crew.png' });
  await key('Escape', 300);

  // ================= PHASE C: a pre-feature save ===================
  const stripped = crafted.replace(/ mc=[0-9,]+/g, '').replace(/ mr=[0-9,]+/g, '').replace(/^fs=.*\n/m, '');
  ok(!stripped.includes('mc=') && !stripped.includes('fs='), 'C: stripped blob is v1.25-free');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), stripped);
  ok(await loadSlot(3, 2), 'C: pre-feature slot 3 loads into play');
  ok((await call('dbg_casbest')) === 1250, 'C: castle record intact', await call('dbg_casbest'));
  ok((await call('dbg_crtcupbest')) === 9, 'C: cup record intact', await call('dbg_crtcupbest'));
  ok((await call('dbg_platdone')) === 9, 'C: plat_done intact', await call('dbg_platdone'));
  ok((await call('dbg_buddyxp')) === 240, 'C: buddy XP intact', await call('dbg_buddyxp'));
  ok(await openCase(), 'C: the case renders on the pre-feature save');
  const rowsC = await readCase();
  ok(rowsC.length === 18, 'C: 18 rows (the First Shift row is absent)', rowsC.length);
  ok(!rowsC.some(r => r[0] === 'FIRST SHIFT'), 'C: no First Shift row on a pre-feature save');
  const byNameC = Object.fromEntries(rowsC.map(r => [r[0], r[1]]));
  ok(byNameC['CASTLE'] === 'BEST 1250', 'C: castle plaque survives (it predates the program)', byNameC['CASTLE']);
  ok(byNameC['WAR!'] === 'CONQUERED', 'C: war plaque survives', byNameC['WAR!']);
  ok(byNameC['CART MEDALS'] === 'NONE YET', 'C: cart medals empty without the mc= token', byNameC['CART MEDALS']);
  ok(byNameC['RUN MEDALS'] === 'NONE YET', 'C: run medals empty without the mr= token', byNameC['RUN MEDALS']);
  await key('Escape', 300);

  // ================= PHASE D: Zen earns nothing ====================
  await page.evaluate(() => localStorage.removeItem('tsg_save'));
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  ok((await call('dbg_mzen')) === 0, 'D: Zen preference starts off');
  // Boot-settle before sampling the menu count. The wasm exports answer
  // before game_init has run its first frame; until probe_slots() scans
  // the slots, the zeroed G reads as a NO-save menu (6 rows) even though
  // all three slots hold run=1 saves. That transient (tens of ms, wider
  // under load) is all a bare read can catch — it bit in the size-stretch
  // and skirmish batteries while every later Phase D check, navigating
  // the settled menu, kept passing. Poll until the count is stable
  // across two reads 300 ms apart, then assert the full set for THIS
  // state: has_save=1 (slots 1-3 all run=1) and keyboard flavour (no pad
  // connected or touched since the reload) -> menu_base_count = 7 - 0 =
  // 7 rows: CONTINUE, START, HOW TO, SETTINGS, DIFFICULTY, ZEN, EDDIE.
  let mcount = await call('dbg_menucount');
  { const t0 = Date.now();
    while (Date.now() - t0 < 15000) { await sleep(300); const v = await call('dbg_menucount'); if (v === mcount) break; mcount = v; } }
  const msel0 = await call('dbg_menusel');
  const zenRow = 5;   // has_save title rows: CONTINUE, START, HOW TO, SETTINGS, DIFFICULTY, ZEN, EDDIE
  ok(mcount >= 7, 'D: title shows the full row set', mcount);
  for (let i = 0; i < ((zenRow - msel0) % mcount + mcount) % mcount; i++) await key('ArrowDown', 180);
  ok((await call('dbg_menusel')) === zenRow, 'D: selection on the ZEN row', await call('dbg_menusel'));
  await key('Space', 350);
  ok((await call('dbg_mzen')) === 1, 'D: Zen toggled on from the title', await call('dbg_mzen'));
  for (let i = 0; i < zenRow - 1; i++) await key('ArrowUp', 150);
  ok((await call('dbg_menusel')) === 1, 'D: selection on START');
  await key('Space', 400);
  ok((await call('dbg_state')) === 7, 'D: slot picker open', await call('dbg_state'));
  await key('Enter', 350);
  ok((await call('dbg_state')) === 7, 'D: overwrite confirm armed', await call('dbg_state'));
  await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'D: fresh Zen run started in slot 3');
  await call('dbg_adopt', 0);
  ok((await call('dbg_zen')) === 1, 'D: Zen run is live', await call('dbg_zen'));
  ok(await startRace(), 'D: Zen race running');
  ok(await finishRaceAt(200), 'D: Zen finish at gold pace banked');
  await call('dbg_bwexit');
  await sleep(400);
  await call('dbg_bwentm', 3);
  ok((await poll('dbg_bwmode', v => v === 3, 6000)) === 3, 'D: entered Run in Zen');
  await finishLevelAt(250);
  ok((await poll('dbg_platdone', v => v === 1, 12000)) === 1, 'D: Zen clear advances progression (as ever)');
  await call('dbg_bwexit');
  await sleep(400);
  ok(await openCase(), 'D: opened the case after Zen-only play');
  const rowsD = await readCase();
  const byNameD = Object.fromEntries(rowsD.map(r => [r[0], r[1]]));
  ok(byNameD['CART MEDALS'] === 'NONE YET', 'D: Zen race earned NO cart medal in the case', byNameD['CART MEDALS']);
  ok(byNameD['RUN MEDALS'] === 'NONE YET', 'D: Zen clear earned NO run medal in the case', byNameD['RUN MEDALS']);
  ok(byNameD['HEAVIEST HAT'] === 'NO RECORD', 'D: Walk of Fame hat untouched by Zen', byNameD['HEAVIEST HAT']);
  ok(byNameD['MOST BUGS SQUASHED'] === 'NO RECORD', 'D: Walk of Fame kills untouched by Zen', byNameD['MOST BUGS SQUASHED']);
  ok(byNameD['BEST BUG BOWL'] === 'NO RECORD', 'D: Walk of Fame bowl untouched by Zen', byNameD['BEST BUG BOWL']);
  ok(byNameD['LONGEST RUN'] === 'NO RECORD', 'D: Walk of Fame time untouched by Zen', byNameD['LONGEST RUN']);
  ok(byNameD['CART CUPS'] === 'HOME -- - FAR --', 'D: no cup rank from a single Zen race', byNameD['CART CUPS']);
  await key('Escape', 300);

  console.log(`\nTROPHYCASETEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
