// shuffletest.js — v1.25 CASTLE SHUFFLE (feature #7): after a first
// clear, the CASTLE cabinet offers SHUFFLE — the same 20 rooms with
// the contents (item rooms/spots, monster rooms/spots, the key and
// the sword) re-dealt from a seed. The suite proves: the panel is
// locked (2 rows) before a clear and grows the SHUFFLE row after;
// the deal is deterministic per seed; a 120-seed sweep is
// structurally completable (door-graph BFS, placement rules, free
// spots, multisets intact); two quiet seeds are completed by
// WALKING the real path sword -> key -> gate; a shuffle escape
// feeds the shared cas_best (Trophy Case row agrees); the cs=
// counter persists and tolerates its own absence; and a classic
// entry still deals the exact shipped tables.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

// the castle's authored geometry, duplicated for structural checks
const CAS_DOORS = [6, 14, 14, 14, 12, 7, 15, 15, 15, 13, 7, 15, 15, 15, 13, 3, 11, 11, 10, 9];
const CAS_PIL = [[0,0,0,0],[296,120,48,48],[0,0,0,0],[0,0,0,0],[120,120,40,120],[0,0,0,0],[150,150,60,24],[286,110,68,68],[0,0,0,0],[480,150,60,24],[120,200,90,30],[286,140,68,40],[0,0,0,0],[460,110,70,40],[140,120,50,50],[0,0,0,0],[296,150,48,48],[0,0,0,0],[150,200,80,26],[470,210,60,30]];
const CAS_ITEM_ROOM = [0, 1, 2, 2, 4, 6, 8, 11, 13, 15, 16, 18, 19, 5, 10, 13, 15];
const CAS_MON = [[1,3],[2,3],[3,0],[5,1],[7,2],[7,2],[9,0],[10,2],[11,0],[11,0],[12,1],[12,1],[13,1],[14,2],[19,0]];
const CAS_SPOT = [[170,140],[470,140],[320,150],[170,262],[470,262],[320,268]];
const SWORD = 13, KEY = 14;

function doorNext(r) {
  const col = r % 5, row = (r / 5) | 0, d = CAS_DOORS[r], ns = [];
  if ((d & 1) && row > 0) ns.push(r - 5);
  if ((d & 2) && col < 4) ns.push(r + 1);
  if ((d & 4) && row < 3) ns.push(r + 5);
  if ((d & 8) && col > 0) ns.push(r - 1);
  return ns;
}
function reachable(from) {
  const seen = new Set([from]); const q = [from];
  while (q.length) { const r = q.shift(); for (const n of doorNext(r)) if (!seen.has(n)) { seen.add(n); q.push(n); } }
  return seen;
}
function doorPath(from, to) {
  const prev = new Map([[from, null]]); const q = [from];
  while (q.length) { const r = q.shift(); if (r === to) break; for (const n of doorNext(r)) if (!prev.has(n)) { prev.set(n, r); q.push(n); } }
  if (!prev.has(to)) return null;
  const path = []; let c = to; while (c !== null) { path.unshift(c); c = prev.get(c); }
  return path;
}
function inPillar(room, x, y, pad) {
  const P = CAS_PIL[room];
  return P[2] > 0 && x >= P[0] - pad && x < P[0] + P[2] + pad && y >= P[1] - pad && y < P[1] + P[3] + pad;
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const walkUntil = async (code, fn, pred, ms = 9000) => {
    await page.keyboard.down(code);
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) break; await sleep(100); }
    await page.keyboard.up(code); await sleep(150);
    return v;
  };
  const gotoRow = async () => {
    const y = await call('dbg_casy');
    if (y < 180) await walkUntil('ArrowDown', 'dbg_casy', v => v >= 184, 5000);
    else if (y > 208) await walkUntil('ArrowUp', 'dbg_casy', v => v <= 204, 5000);
  };
  const gotoCol = async () => {
    const x = await call('dbg_casx');
    if (x >= 300 && x <= 340) return;
    await walkUntil('ArrowDown', 'dbg_casy', v => v >= 292, 6000);
    if (x < 300) await walkUntil('ArrowRight', 'dbg_casx', v => v >= 306, 6000);
    else await walkUntil('ArrowLeft', 'dbg_casx', v => v <= 334, 6000);
  };
  // walk one door from the current room into room n (adjacent).
  // A room's stone block can span the door row/column (room 18's
  // block sits on the west-door row): a straight hold pins the
  // buddy on the block's face forever. So hold the direction and,
  // whenever the position stalls, slide perpendicular for a beat —
  // the diagonal walks the buddy around the block's corner.
  let slideFlip = false;
  const crossDoor = async (n) => {
    const cur = await call('dbg_casroom');
    let code = null;
    if (n === cur - 1) code = 'ArrowLeft';
    else if (n === cur + 1) code = 'ArrowRight';
    else if (n === cur - 5) code = 'ArrowUp';
    else if (n === cur + 5) code = 'ArrowDown';
    if (!code) return false;
    if (code === 'ArrowLeft' || code === 'ArrowRight') await gotoRow(); else await gotoCol();
    const horiz = (code === 'ArrowLeft' || code === 'ArrowRight');
    await page.keyboard.down(code);
    const t0 = Date.now();
    let lastPos = '', stallSince = Date.now(), slideDir = null, slideUntil = 0, done = false;
    while (Date.now() - t0 < 15000) {
      if ((await call('dbg_casroom')) === n) { done = true; break; }
      const pos = (await call('dbg_casx')) + ',' + (await call('dbg_casy'));
      const now = Date.now();
      if (pos !== lastPos) { lastPos = pos; stallSince = now; }
      if (slideDir && now > slideUntil) { await page.keyboard.up(slideDir); slideDir = null; }
      if (!slideDir && now - stallSince > 500) {
        slideDir = horiz ? (slideFlip ? 'ArrowDown' : 'ArrowUp') : (slideFlip ? 'ArrowRight' : 'ArrowLeft');
        slideFlip = !slideFlip;
        await page.keyboard.down(slideDir);
        slideUntil = now + 450;
        stallSince = now;
      }
      await sleep(100);
    }
    await page.keyboard.up(code);
    if (slideDir) await page.keyboard.up(slideDir);
    await sleep(150);
    return done || (await call('dbg_casroom')) === n;
  };
  const walkToRoom = async (target, timeout = 60000) => {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const cur = await call('dbg_casroom');
      if (cur === target) return true;
      const path = doorPath(cur, target);
      if (!path || path.length < 2) return false;
      if (!await crossDoor(path[1])) { /* drifted; recompute next loop */ }
    }
    return (await call('dbg_casroom')) === target;
  };
  const walkToPoint = async (tx, ty, timeout = 15000) => {
    const t0 = Date.now();
    let lastPos = '', stallSince = Date.now(), jiggle = false;
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_casx'), y = await call('dbg_casy');
      if (Math.abs(x - tx) <= 13 && Math.abs(y - ty) <= 13) return true;
      const pos = x + ',' + y, now = Date.now();
      if (pos !== lastPos) { lastPos = pos; stallSince = now; }
      const keys = [];
      if (x < tx - 13) keys.push('ArrowRight'); else if (x > tx + 13) keys.push('ArrowLeft');
      if (y < ty - 13) keys.push('ArrowDown'); else if (y > ty + 13) keys.push('ArrowUp');
      // pinned on a stone block: add a perpendicular beat to slide off it
      if (now - stallSince > 600) { keys.push(jiggle ? 'ArrowDown' : 'ArrowUp'); if (now - stallSince > 1100) { jiggle = !jiggle; stallSince = now; } }
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  };
  const gotoCastlePanel = async () => {
    await call('dbg_tp', 0, -11898); await sleep(200);
    await key('KeyE', 250);
    return (await poll('dbg_arcgate', v => v === 1, 3000)) === 1;
  };
  const readLayout = async () => {
    const L = { items: [], mons: [] };
    for (let id = 0; id < 17; id++) L.items.push({ room: await call('dbg_casitemroom', id), x: await call('dbg_casitemx', id), y: await call('dbg_casitemy', id) });
    for (let i = 0; i < 15; i++) L.mons.push({ room: await call('dbg_casmonroom', i), type: await call('dbg_casmontype', i), x: await call('dbg_casmonx', i), y: await call('dbg_casmony', i) });
    return L;
  };
  const enterShuffleSeed = async (seed) => {
    if ((await call('dbg_bwmode')) === 6) await exitCastle();   // never let a stuck run poison the next entry
    await call('dbg_cassetseed', seed);
    T('panel opened for seed ' + seed, await gotoCastlePanel());
    await key('ArrowDown', 200);
    await key('Space', 300);
    const on = (await poll('dbg_bwmode', v => v === 6, 4000)) === 6;
    T('seed ' + seed + ': shuffle launched', on && (await call('dbg_casshuffle')) === 1 && (await call('dbg_casseed')) === seed,
      'mode=' + (await call('dbg_bwmode')) + ' shuf=' + (await call('dbg_casshuffle')) + ' seed=' + (await call('dbg_casseed')));
    return on;
  };
  const exitCastle = async () => {
    await key('Escape', 300);
    return (await poll('dbg_bwmode', v => v === -1, 4000)) === -1;
  };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // mall -> arcade -> CASTLE cabinet (the castletest road)
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));

  // ============ PHASE A: the panel is locked before a clear ======
  T('castle panel opened (fresh buddy)', await gotoCastlePanel());
  await key('ArrowDown', 200);            // row 1: with two rows this is LEAVE
  await key('Space', 300);
  T('A: Down+Space on a fresh panel LEAVES (no SHUFFLE row yet)',
    (await call('dbg_arcgate')) === 0 && (await call('dbg_bwmode')) !== 6,
    'gate=' + (await call('dbg_arcgate')) + ' mode=' + (await call('dbg_bwmode')));

  // ============ PHASE B: a classic clear unlocks SHUFFLE =========
  T('panel reopened', await gotoCastlePanel());
  await key('Space', 300);
  T('B: classic launched from row 0', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6 && (await call('dbg_casshuffle')) === 0,
    'mode=' + (await call('dbg_bwmode')) + ' shuf=' + (await call('dbg_casshuffle')));
  const xpB0 = await call('dbg_buddyxp');
  await call('dbg_casgive', 2);           // the key, by dbg (the castletest shortcut)
  await gotoCol();
  await walkUntil('ArrowDown', 'dbg_casy', v => v >= 316, 6000);
  T('B: the classic escape lands', (await poll('dbg_bwmode', v => v === -1, 6000)) === -1, await call('dbg_bwmode'));
  T('B: classic best is the bare escape (100)', (await call('dbg_casbest')) === 100, await call('dbg_casbest'));
  T('B: the escape paid +50 XP', (await call('dbg_buddyxp')) === xpB0 + 50, (await call('dbg_buddyxp')) - xpB0);

  // the panel now has the third row: Down+Space launches SHUFFLE,
  // and the buddy's first counter deal is seed 1
  T('panel reopened after the clear', await gotoCastlePanel());
  await page.screenshot({ path: 'tsg-v125-f7-panel.png' });
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('B: Down+Space now launches SHUFFLE', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6 && (await call('dbg_casshuffle')) === 1,
    'mode=' + (await call('dbg_bwmode')) + ' shuf=' + (await call('dbg_casshuffle')));
  T('B: the first counter deal is seed 1', (await call('dbg_casseed')) === 1, await call('dbg_casseed'));
  T('B: Escape leaves a shuffle run without the key (v1.22 holds)', await exitCastle());

  // ============ PHASE C: the deal is deterministic ===============
  T('C: seed 42 entered', await enterShuffleSeed(42));
  const lay42a = await readLayout();
  T('C: seed 42 left', await exitCastle());
  T('C: seed 42 re-entered', await enterShuffleSeed(42));
  const lay42b = await readLayout();
  T('C: seed 42 left again', await exitCastle());
  T('C: the same seed deals the identical castle', JSON.stringify(lay42a) === JSON.stringify(lay42b));
  T('C: seed 43 entered', await enterShuffleSeed(43));
  const lay43 = await readLayout();
  T('C: seed 43 left', await exitCastle());
  T('C: a different seed deals a different castle', JSON.stringify(lay43) !== JSON.stringify(lay42a));

  // ============ PHASE D: the 120-seed sweep ======================
  const layouts = new Map([[42, lay42a], [43, lay43]]);
  let sweepBad = [];
  for (let seed = 0; seed < 120; seed++) {
    if (!await enterShuffleSeed(seed)) { sweepBad.push(seed + ':enter'); await exitCastle(); continue; }
    const L = await readLayout();
    layouts.set(seed, L);
    const reach = reachable(17);
    const problems = [];
    if (reach.size !== 20) problems.push('reach=' + reach.size);
    if (L.items[KEY].room === 17) problems.push('key-in-courtyard');
    if (L.items[SWORD].room === 17) problems.push('sword-in-courtyard');
    if (L.items[KEY].room === L.items[SWORD].room) problems.push('key-sword-same-room');
    if (!reach.has(L.items[KEY].room)) problems.push('key-unreachable');
    if (!reach.has(L.items[SWORD].room)) problems.push('sword-unreachable');
    const occupied = new Set();
    for (let id = 0; id < 17; id++) {
      const it = L.items[id];
      if (it.room === 17) problems.push('item' + id + '-in-courtyard');
      if (inPillar(it.room, it.x, it.y, 6)) problems.push('item' + id + '-in-pillar');
      const k2 = it.room + ':' + it.x + ',' + it.y;
      if (occupied.has(k2)) problems.push('item' + id + '-shares-a-spot');
      occupied.add(k2);
    }
    const typeCount = [0, 0, 0, 0];
    for (let i = 0; i < 15; i++) {
      const m = L.mons[i];
      typeCount[m.type]++;
      if (m.room === 17) problems.push('mon' + i + '-in-courtyard');
      if (inPillar(m.room, m.x, m.y, 6)) problems.push('mon' + i + '-in-pillar');
      const k2 = m.room + ':' + m.x + ',' + m.y;
      if (occupied.has(k2)) problems.push('mon' + i + '-shares-a-spot');
      occupied.add(k2);
    }
    if (typeCount.join(',') !== '5,4,4,2') problems.push('types=' + typeCount.join(','));
    if (problems.length) sweepBad.push(seed + ':' + problems.join('|'));
    if (!await exitCastle()) sweepBad.push(seed + ':exit');
  }
  T('D: all 120 seeds deal a completable castle', sweepBad.length === 0, sweepBad.slice(0, 4).join(' ; '));

  // ============ PHASE E: walk two quiet seeds end to end =========
  // choose the two seeds whose sword->key->gate route crosses the
  // fewest monsters and still offers 2+ treasures on the way
  const seedCost = (L) => {
    const route = [...doorPath(17, L.items[SWORD].room), ...doorPath(L.items[SWORD].room, L.items[KEY].room), ...doorPath(L.items[KEY].room, 17)];
    const roomsOn = new Set(route);
    let mons = 0, monsHot = 0, treas = 0;
    for (const m of L.mons) { if (roomsOn.has(m.room)) mons++; if (m.room === L.items[SWORD].room || m.room === L.items[KEY].room) monsHot++; }
    for (let id = 0; id < 13; id++) if (roomsOn.has(L.items[id].room)) treas++;
    return { cost: mons + 3 * monsHot - 2 * Math.min(treas, 3), treas, mons };
  };
  const ranked = [...layouts.entries()].map(([seed, L]) => ({ seed, L, ...seedCost(L) }))
    .filter(r => r.treas >= 2).sort((a, b) => a.cost - b.cost);
  T('E: at least two walkable seeds found', ranked.length >= 2, 'ranked=' + ranked.length);
  const playSeeds = ranked.slice(0, 2);
  let bestBefore = await call('dbg_casbest');
  for (const pick of playSeeds) {
    const L = pick.L, seed = pick.seed;
    T('E' + seed + ': entered', await enterShuffleSeed(seed));
    const xp0 = await call('dbg_buddyxp');
    // travel door by door, gathering every treasure in each room
    // entered — the same route the seed ranking priced
    const collectRoom = async (room) => {
      for (let id = 0; id < 13; id++) if (L.items[id].room === room) await walkToPoint(L.items[id].x, L.items[id].y, 8000);
    };
    const travelTo = async (target) => {
      const t0 = Date.now();
      while (Date.now() - t0 < 90000) {
        const cur = await call('dbg_casroom');
        await collectRoom(cur);
        if (cur === target) return true;
        const path = doorPath(cur, target);
        if (!path || path.length < 2) return false;
        await crossDoor(path[1]);
      }
      return (await call('dbg_casroom')) === target;
    };
    // sword first, then the key, then home
    T('E' + seed + ': walked to the sword room ' + L.items[SWORD].room, await travelTo(L.items[SWORD].room));
    await walkToPoint(L.items[SWORD].x, L.items[SWORD].y);
    const itS = await poll('dbg_casitems', v => (v & 1) === 1, 6000);
    T('E' + seed + ': the sword is in hand', (itS & 1) === 1, itS);
    T('E' + seed + ': walked to the key room ' + L.items[KEY].room, await travelTo(L.items[KEY].room));
    if (seed === playSeeds[0].seed) await page.screenshot({ path: 'tsg-v125-f7-shuffle-keyroom.png' });
    await walkToPoint(L.items[KEY].x, L.items[KEY].y);
    T('E' + seed + ': the key is in hand', ((await call('dbg_casitems')) & 2) === 2, await call('dbg_casitems'));
    T('E' + seed + ': walked home to the courtyard', await travelTo(17));
    T('E' + seed + ': back in the courtyard', (await call('dbg_casroom')) === 17, await call('dbg_casroom'));
    const treas = await call('dbg_castreasures');
    T('E' + seed + ': gathered 2+ treasures on the way', treas >= 2, 'treasures=' + treas);
    const scoreBefore = await call('dbg_casscore');
    await gotoCol();
    await walkUntil('ArrowDown', 'dbg_casy', v => v >= 316, 6000);
    T('E' + seed + ': the gate opens (escape!)', (await poll('dbg_bwmode', v => v === -1, 8000)) === -1, await call('dbg_bwmode'));
    const bestNow = await call('dbg_casbest');
    T('E' + seed + ': the shared best moved exactly as the score says',
      bestNow === Math.max(bestBefore, scoreBefore + 100),
      'score=' + scoreBefore + ' best ' + bestBefore + ' -> ' + bestNow);
    if (pick === playSeeds[0]) T('E' + seed + ': the first shuffle escape strictly raised the best', bestNow > bestBefore, bestBefore + ' -> ' + bestNow);
    T('E' + seed + ': the escape paid +50 XP', (await call('dbg_buddyxp')) === xp0 + 50, (await call('dbg_buddyxp')) - xp0);
    bestBefore = bestNow;
  }

  // the Trophy Case agrees with the shared best. Leave the arcade
  // by its own exit mat (west wall) — the DEV is genuinely in the
  // arcade after an escape, and the mat is the real way out.
  await call('dbg_tp', -330, -11976); await sleep(250);
  await key('KeyE', 300);
  T('left the arcade by its exit mat', (await call('dbg_inarcade')) === 0, await call('dbg_inarcade'));
  const walkMall = async (tx, ty, tol = 14, timeout = 20000) => {
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
  };
  T('walked to the TROPHY CASE', await walkMall(340, -5868));
  await key('KeyE', 350);
  T('the case is open (station 5)', (await poll('dbg_station', v => v === 5, 6000)) === 5, await call('dbg_station'));
  const readStr = (fn, arg) => page.evaluate(([f, a]) => {
    const ptr = window.__TSG[f](a);
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]);
    return s;
  }, [fn, arg]);
  const nRows = await call('dbg_caserows');
  let castleRight = null;
  for (let i = 0; i < nRows; i++) if ((await readStr('dbg_casename', i)) === 'CASTLE') castleRight = await readStr('dbg_caseright', i);
  T('the case CASTLE row reads the shared best', castleRight === 'BEST ' + (await call('dbg_casbest')), castleRight);
  T('the CROWN topper agrees (earned)', ((await call('dbg_topearned')) & 1) === 1, await call('dbg_topearned'));
  await key('Escape', 300);

  // ============ PHASE F: the cs= counter persists ================
  const slotBlob = async () => page.evaluate(() => localStorage.getItem('tsg_save1'));
  let blob = '';
  { const t0 = Date.now();
    while (Date.now() - t0 < 20000) { blob = (await slotBlob()) || ''; const m = blob.match(/ cb=(\d+)/); if (m && +m[1] >= 100) break; await sleep(400); } }
  T('F: the slot save carries cb=', / cb=\d+/.test(blob), blob.slice(0, 80));
  // Install a crafted blob and load it. The blob MUST be written
  // after the reload, never before: the still-running game autosaves
  // every 5s from its in-memory state, so a blob written under a live
  // instance can be overwritten (cs= restored) before the reload
  // commits — the race behind the intermittent "deals seed 1 first ->
  // seed=2" red (proven by fprobe3: cs= reappeared 3.8s after the
  // write, and the load then dealt seed 2). At the title the fresh
  // instance never writes slot blobs (the autosave runs only in
  // GS_PLAY/GS_STORE), and continue_run reads the slot text fresh at
  // confirm (save.c), so write-then-confirm is deterministic.
  const loadCrafted = async (blobText) => {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await page.evaluate(s => localStorage.setItem('tsg_save1', s), blobText);
    await key('Space', 400);
    await key('Enter', 400);
    return (await poll('dbg_state', v => v === 1, 8000)) === 1;
  };
  // a save written in the arcade reloads at the mall centre still
  // flagged in_arcade (a shipped quirk): leave by the exit mat
  // first, then ride the door back in — the real player path
  const rideToCastlePanel = async (tag) => {
    await call('dbg_tp', -330, -11976); await sleep(250);
    await key('KeyE', 300);
    T(tag + ': unwedged by the exit mat', (await call('dbg_inarcade')) === 0, await call('dbg_inarcade'));
    await call('dbg_tp', 350, -5976); await sleep(200);
    await key('KeyE', 250);
    T(tag + ': in the arcade again', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
    return gotoCastlePanel();
  };
  const stripped = blob.replace(/ cs=\d+/, '');
  T('F: stripped blob has no cs=', !/ cs=\d+/.test(stripped));
  T('F: stripped save reloads', await loadCrafted(stripped));
  T('F: panel opened on the stripped save', await rideToCastlePanel('F1'));
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('F: a cs-less save deals seed 1 first', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6 && (await call('dbg_casseed')) === 1,
    'seed=' + (await call('dbg_casseed')));
  T('F: left the stripped-seed run', await exitCastle());
  const crafted = stripped.replace(/( cb=\d+)/, '$1 cs=41');
  T('F: crafted blob carries cs=41', / cs=41/.test(crafted));
  T('F: crafted save reloads', await loadCrafted(crafted));
  T('F: panel opened on the cs=41 save', await rideToCastlePanel('F2'));
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('F: a cs=41 save deals seed 42 next', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6 && (await call('dbg_casseed')) === 42,
    'seed=' + (await call('dbg_casseed')));
  T('F: left the cs=41 run', await exitCastle());

  // ============ PHASE G: classic is the shipped castle ===========
  T('G: panel opened for classic', await gotoCastlePanel());
  await key('Space', 300);
  T('G: classic launched', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6 && (await call('dbg_casshuffle')) === 0,
    'mode=' + (await call('dbg_bwmode')));
  const layC = await readLayout();
  let classicOk = true, classicWhy = '';
  for (let id = 0; id < 17; id++) {
    const room = CAS_ITEM_ROOM[id];
    let k = 0; for (let i = 0; i < id; i++) if (CAS_ITEM_ROOM[i] === room) k++;
    const spot = CAS_SPOT[(room + k) % 6];
    if (layC.items[id].room !== room || layC.items[id].x !== spot[0] || layC.items[id].y !== spot[1]) { classicOk = false; classicWhy = 'item' + id; }
  }
  for (let i = 0; i < 15; i++) {
    const room = CAS_MON[i][0];
    let k = 0; for (let j = 0; j < i; j++) if (CAS_MON[j][0] === room) k++;
    const spot = CAS_SPOT[(room + 2 + k) % 6];
    if (layC.mons[i].room !== room || layC.mons[i].type !== CAS_MON[i][1] || layC.mons[i].x !== spot[0] || layC.mons[i].y !== spot[1]) { classicOk = false; classicWhy = 'mon' + i; }
  }
  T('G: the classic deal matches the shipped tables exactly', classicOk, classicWhy);
  // the classic dungeon still holds its key and its ogre (room 10)
  T('G: walked to the classic dungeon', await walkToRoom(10));
  await page.screenshot({ path: 'tsg-v125-f7-classic-dungeon.png' });
  T('G: classic run left', await exitCastle());

  console.log('shuffletest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
