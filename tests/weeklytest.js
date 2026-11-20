// weeklytest.js — v1.25 WEEKLY CHALLENGE (feature #6): one goal a
// week, a pure function of the week id, read off counters the game
// already keeps, presented on the Mall's CHALLENGE BOARD (station 6)
// and paid in buddy XP through the shared buddy_award_xp door.
// The suite pins the perceived day with dbg_wksetday (day numbers;
// week = (daynum+3)/7, blocks start Monday):
//   day 20997 = week 3000, 21004 = week 3001, 21011 = week 3002.
// Phase 1 (fresh, week 3000): the slot joins the current week, the
//   derivation matches an independent JS replica of save.c's hash2
//   tables, the board shows the week's card at 0/<target>, and a
//   save/reload changes nothing.
// Phase 2 (crafted, week 3000): a decorated save whose wk= line
//   carries zero baselines completes on the poll whatever the type
//   (every source is maxed) — buddy XP +50 exactly once, persisted,
//   and a reload does not pay again.
// Phase 3 (rollover): setday +7 offers week 3001 with PREVIOUS: DONE;
//   another +7 unplayed shows PREVIOUS: MISSED; the total never moves.
// Phase 4 (stripped): the same blob minus the wk= line loads with
//   records intact, initializes into the current week with baselines
//   at the loaded counters (delta progress 0 — no retro-completion).
// Phase 5 (Zen): a crafted Zen save completes and pays — the First
//   Shift precedent (Zen guides and pays; it only sets no RECORDS,
//   and the weekly board keeps none).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

// an independent replica of the derivation in save.c (wk_challenge):
// hash2 with addition, the same seeds, the same tables. If the game's
// hash or tables ever drift, this suite is supposed to say so.
function hash2(x, y, seed) {
  let h = (seed + Math.imul(x, 374761393) + Math.imul(y, 668265263)) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177) >>> 0;
  return (h ^ (h >>> 16)) >>> 0;
}
const WK_NAMES = ['BUG SQUASHER', 'BIG HAT WEEK', 'BANK HOLIDAY', 'WAR EFFORT', 'MEDAL HUNT',
                  'TANK TROOP', 'STORY TIME', 'MALL RATS', 'CASH IN', 'WALL BUILDERS'];
const WK_NVAR = [3, 3, 3, 2, 2, 2, 1, 2, 2, 2];
const WK_VAR = [[30, 50, 80], [40, 60, 100], [50, 100, 200], [1, 2, 0], [1, 2, 0],
                [2, 3, 0], [1, 0, 0], [1, 3, 0], [100, 250, 0], [4, 6, 0]];
function replica(week) {
  const t = hash2(week, 7919, 0x9E3779B9) % 10;
  const tg = WK_VAR[t][hash2(week, 104729, 0x85EBCA6B) % WK_NVAR[t]];
  return { type: t, target: tg, name: WK_NAMES[t] };
}

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
  // the day override is runtime state in the wasm instance — a page
  // reload wipes it. pinDay remembers the suite's intended day and
  // loadSlot re-pins it after every reload, BEFORE the slot loads
  // (continue_run's rollover reads the clock at load time).
  let pinDay = -1;
  const setDay = async d => { pinDay = d; return call('dbg_wksetday', d); };
  // deterministic slot loading: the picker opens on the global
  // blob's slot= line (open_slots picks G.slot-1), and that line
  // only updates when an autosave happens to fire — so point it at
  // the target slot BEFORE the reload and press Enter with no
  // navigation at all. (Picker archaeology: the selection cycles
  // mod 4 including a BACK row, and "last played" is whatever the
  // last save wrote, not what the suite last loaded.)
  // crafted blobs are written AT THE TITLE (the shuffletest rule:
  // never rewrite a save under a live game instance — its autosave
  // can clobber the write). Reload first, write, then loadSlot.
  async function writeAtTitle(key2, blobText) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await page.evaluate(([k, b]) => localStorage.setItem(k, b), [key2, blobText]);
  }
  async function loadSlot(n) {
    await page.evaluate(t => {
      const g = localStorage.getItem('tsg_save');
      if (g) localStorage.setItem('tsg_save', g.replace(/^slot=.*$/m, 'slot=' + t));
    }, n);
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    if (pinDay >= 0) await call('dbg_wksetday', pinDay);
    await key('Space', 400);
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
  // the real player path: in the hut door, walk the mall floor to
  // the board's stand point (south wall, west end), INTERACT it open
  async function openBoard() {
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 300);
    if ((await call('dbg_mall')) !== 1) return false;
    if (!await walkTo(-330, -5868, 14)) return false;
    await key('KeyE', 350);
    return (await poll('dbg_station', v => v === 6, 6000)) === 6;
  }
  async function readBoard() {
    const n = await call('dbg_wkrows');
    const rows = [];
    for (let i = 0; i < n; i++) rows.push([await str('dbg_wkrowname', i), await str('dbg_wkrowright', i)]);
    return rows;
  }
  function expectRows(rows, table, tag) {
    ok(rows.length === table.length, `${tag}: row count ${table.length}`, rows.length);
    for (let i = 0; i < table.length && i < rows.length; i++) {
      ok(rows[i][0] === table[i][0], `${tag}: row ${i} name "${table[i][0]}"`, rows[i][0]);
      ok(rows[i][1] === table[i][1], `${tag}: row ${i} (${table[i][0]}) right "${table[i][1]}"`, rows[i][1]);
    }
  }

  // ============ PHASE 1: a fresh slot joins week 3000 =============
  const exp3000 = replica(3000);
  ok((await setDay(20997)) === 3000, '1: day 20997 is week 3000 by construction');
  ok((await call('dbg_state')) === 0, '1: boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, '1: fresh run started');
  ok((await call('dbg_wk')) === 3000, '1: the slot joined week 3000', await call('dbg_wk'));
  ok((await call('dbg_wktype')) === exp3000.type, `1: type matches the replica (${exp3000.name})`, await call('dbg_wktype'));
  ok((await call('dbg_wktarget')) === exp3000.target, `1: target matches the replica (${exp3000.target})`, await call('dbg_wktarget'));
  ok((await str('dbg_wkname')) === exp3000.name, '1: the board names the replica challenge', await str('dbg_wkname'));
  ok((await call('dbg_wkprog')) === 0, '1: progress starts at 0', await call('dbg_wkprog'));
  ok((await call('dbg_wkdone')) === 0, '1: not done');
  ok((await call('dbg_wktotal')) === 0, '1: ledger empty');
  await call('dbg_adopt', 0);
  ok(await openBoard(), '1: walked the mall and opened the CHALLENGE BOARD (station 6)');
  ok((await call('dbg_state')) === 2, '1: the board panel is a store panel (GS_STORE)');
  const rows1 = await readBoard();
  expectRows(rows1, [
    ['THIS WEEK', ''],
    [exp3000.name, `0/${exp3000.target}`],
    ['THE LEDGER', ''],
    ['CHALLENGES DONE', '0'],
  ], '1');
  await page.screenshot({ path: 'tsg-v125-f6-board-early.png' });
  await key('Escape', 300);
  // the week survives a save/reload untouched (same week, same card)
  const blob1 = await pollSave(1, 'wk=3000,');
  ok(/^wk=3000,0,0,0,0,/m.test(blob1), '1: the save carries the wk= line for week 3000', (blob1.match(/^wk=.*$/m) || [''])[0]);
  ok(await loadSlot(1), '1: slot 1 reloads');
  ok((await call('dbg_wk')) === 3000 && (await call('dbg_wktype')) === exp3000.type
     && (await call('dbg_wktarget')) === exp3000.target, '1: same week, same challenge after reload');
  ok((await call('dbg_wkprog')) === 0, '1: progress still 0 after reload');

  // ====== PHASE 2: a maxed save with zero baselines completes =====
  const SQ = 2752682;   // quests 0-5 done (trophycasetest's value)
  const setTok = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);
  const crafted = blob1.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    let L = line;
    L = setTok(L, 'bk', 500);           // banked 500 kg (BANK HOLIDAY state source)
    L = setTok(L, 'bb', 5);             // beans: shop fingerprint up (MALL RATS delta)
    L = setTok(L, 'bx', 240);           // buddy XP 240 -> expect 290 after the payout
    L = setTok(L, 'sq', SQ); L = setTok(L, 'sq2', 170);
    L = setTok(L, 'cb', 1250);          // castle record (Phase 4 intactness)
    L = setTok(L, 'td', 6);             // tank_done (TANK TROOP delta)
    L = setTok(L, 'wr', 8);             // war_prog (WAR EFFORT delta)
    return L + ' mc=230,271,0,0,0,0,0,880 mr=261,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0';   // medals (MEDAL HUNT delta)
  }).join('\n')
    .replace(/^tkills=.*$/m, 'tkills=9999')        // BUG SQUASHER delta
    .replace(/^tearned=.*$/m, 'tearned=99999')     // CASH IN delta
    .replace(/^walls=0$/m, 'walls=6\nwl= 100 100 10 10 1 1\nwl= 200 100 10 10 1 1\nwl= 300 100 10 10 1 1\nwl= 100 300 10 10 1 1\nwl= 200 300 10 10 1 1\nwl= 300 300 10 10 1 1')   // WALL BUILDERS state
    .replace(/^fs=.*$/m, 'fs=2,6,0')
    .replace(/^wk=.*$/m, 'wk=3000,0,0,0,0,0,0,0,0,0,0,0');   // week 3000, ZERO baselines
  ok(crafted.includes('wk=3000,0,0,0,0,') && crafted.includes('tkills=9999') && crafted.includes('walls=6'),
     '2: crafted blob carries maxed counters and zero baselines');
  await writeAtTitle('tsg_save2', crafted);
  ok(await loadSlot(2), '2: crafted slot 2 loads into play');
  ok((await poll('dbg_wkdone', v => v === 1, 10000)) === 1, '2: the challenge completes on the poll');
  ok((await call('dbg_buddyxp')) === 290, '2: buddy XP paid exactly +50 (240 -> 290)', await call('dbg_buddyxp'));
  ok((await call('dbg_wktotal')) === 1, '2: the ledger counts the finish');
  const saved2 = await pollSave(2, 'wk=3000,3000,1');
  ok(/^wk=3000,3000,1,0,0,/m.test(saved2), '2: completion persisted in the wk= line', (saved2.match(/^wk=.*$/m) || [''])[0]);
  ok(await openBoard(), '2: opened the board on the completed week');
  const rows2 = await readBoard();
  expectRows(rows2, [
    ['THIS WEEK', ''],
    [exp3000.name, 'DONE!'],
    ['THE LEDGER', ''],
    ['CHALLENGES DONE', '1'],
  ], '2');
  await page.screenshot({ path: 'tsg-v125-f6-board-done.png' });
  await key('Escape', 300);
  ok(await loadSlot(2), '2: slot 2 reloads');
  await sleep(1500);
  ok((await call('dbg_buddyxp')) === 290, '2: NO second payout after reload', await call('dbg_buddyxp'));
  ok((await call('dbg_wkdone')) === 1 && (await call('dbg_wktotal')) === 1, '2: done state and ledger survive the reload');

  // ============ PHASE 3: the week rolls over, twice ===============
  const exp3001 = replica(3001), exp3002 = replica(3002);
  await setDay(21004);
  ok((await poll('dbg_wk', v => v === 3001, 10000)) === 3001, '3: +7 days rolls the slot into week 3001');
  ok((await call('dbg_wktype')) === exp3001.type && (await call('dbg_wktarget')) === exp3001.target,
     `3: week 3001 offers the replica challenge (${exp3001.name} ${exp3001.target})`);
  ok((await call('dbg_wkdone')) === 0, '3: the new week starts unpaid');
  ok((await call('dbg_wkprog')) === 0, '3: the new week starts at 0 (baselines re-snapshotted)');
  ok((await call('dbg_wktotal')) === 1, '3: the ledger keeps the one finish');
  ok(await openBoard(), '3: opened the board in week 3001');
  const rows3 = await readBoard();
  expectRows(rows3, [
    ['THIS WEEK', ''],
    [exp3001.name, `0/${exp3001.target}`],
    ['THE LEDGER', ''],
    ['CHALLENGES DONE', '1'],
    [`PREVIOUS: ${exp3000.name}`, 'DONE'],
  ], '3');
  await key('Escape', 300);
  await setDay(21011);
  ok((await poll('dbg_wk', v => v === 3002, 10000)) === 3002, '3: another +7 rolls into week 3002');
  ok((await call('dbg_wktype')) === exp3002.type && (await call('dbg_wktarget')) === exp3002.target,
     `3: week 3002 offers the replica challenge (${exp3002.name} ${exp3002.target})`);
  ok(await openBoard(), '3: opened the board in week 3002');
  const rows3b = await readBoard();
  const prevRow = rows3b.find(r => r[0].startsWith('PREVIOUS:'));
  ok(!!prevRow && prevRow[0] === `PREVIOUS: ${exp3001.name}` && prevRow[1] === 'MISSED',
     '3: the unplayed week 3001 reads PREVIOUS: MISSED', prevRow && prevRow.join(' | '));
  ok((await call('dbg_wktotal')) === 1, '3: a miss costs nothing — the ledger still counts 1');
  await key('Escape', 300);

  // ======== PHASE 4: a pre-feature save (no wk= line) loads =======
  // bx=241 (not the crafted 240): proves the picker landed on slot 3
  const stripped = crafted.replace(/^wk=.*\n/m, '').replace(' bx=240', ' bx=241');
  ok(!/^wk=/m.test(stripped) && stripped.includes(' bx=241'), '4: stripped blob has no wk= line');
  await writeAtTitle('tsg_save3', stripped);
  // the perceived week for a stripped save is the CURRENT one (3002)
  ok(await loadSlot(3), '4: pre-feature slot 3 loads into play');
  ok((await call('dbg_casbest')) === 1250, '4: castle record intact', await call('dbg_casbest'));
  ok((await call('dbg_buddyxp')) === 241, '4: buddy XP intact (no payout) — and this IS slot 3', await call('dbg_buddyxp'));
  ok((await call('dbg_wk')) === 3002, '4: the slot initializes into the current week', await call('dbg_wk'));
  ok((await call('dbg_wkprog')) === 0, '4: delta progress 0 — baselines snapshot the loaded counters', await call('dbg_wkprog'));
  ok((await call('dbg_wkdone')) === 0, '4: no retro-completion');
  ok((await call('dbg_wktotal')) === 0, '4: the ledger starts empty');
  ok(await openBoard(), '4: the board renders on the pre-feature save');
  const rows4 = await readBoard();
  ok(rows4.length === 4 && rows4[1][0] === exp3002.name && rows4[1][1] === `0/${exp3002.target}`,
     '4: board shows the current week at 0/target', JSON.stringify(rows4));
  await key('Escape', 300);

  // ============ PHASE 5: Zen advances and pays (FS rule) ============
  await setDay(20997);   // back to week 3000, the week the crafted wk= line belongs to
  // bx=242 (unique to this blob): proves the picker landed on slot 1
  const zenCraft = crafted.replace(/^zrun=.*$/m, 'zrun=1').replace(' bx=240', ' bx=242');
  ok(zenCraft.includes('zrun=1') && zenCraft.includes(' bx=242'), '5: Zen blob crafted');
  await writeAtTitle('tsg_save1', zenCraft);
  ok(await loadSlot(1), '5: Zen slot 1 loads');
  ok((await call('dbg_zen')) === 1, '5: the run is Zen', await call('dbg_zen'));
  ok((await poll('dbg_wkdone', v => v === 1, 10000)) === 1, '5: the weekly completes in Zen (training, not a record)');
  ok((await call('dbg_buddyxp')) === 292, '5: Zen pays the same +50 XP (242 -> 292)', await call('dbg_buddyxp'));
  await setDay(-1);
  const realWeek = await page.evaluate(() => {
    const d = new Date();
    const daynum = Math.floor(Date.UTC(d.getFullYear(), d.getMonth(), d.getDate()) / 86400000);
    return Math.floor((daynum + 3) / 7);
  });
  ok((await call('dbg_wkweek')) === realWeek, '5: clearing the override hands the clock back to the device', await call('dbg_wkweek'));

  console.log(`\nWEEKLYTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
