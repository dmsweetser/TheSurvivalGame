// elitetest.js — v1.25 NAMED ELITES + BOUNTY BOARD (feature #8):
// one posted bounty at a time, naming one elite — a bug elevated at
// the end of spawn_bug (kind set to the bounty's, weight x3 under a
// 500 kg cap, standard hp/dmg formulas) whose identity is a pure
// function of the bounty serial (unsigned hash2, never the gameplay
// RNG). Killing it on the real path pays automatically: its weight
// again into the KILLER's bank, 25 XP to every buddy, the ledger
// +1, the next name chalked up, a 150 s cooldown. The board is Mall
// station 7, hanging on the west wall.
// Phase 1 (fresh): serial 0 posts replica(0)'s name; the board,
//   walked to on the real Mall path, reads AT LARGE / NOWHERE YET /
//   CLAIMED 0, and its blurb names the replica title + kind.
// Phase 2 (summon): the control's weight (read off the bug itself
//   via dbg_bugidx/dbg_bugw in the same evaluate — no frame can
//   interleave, no field-mate can pollute it) vs the forced elite's.
//   The elite is a 3x scaling of the SAME spawn, but a ratio of two
//   independent quartic draws is a lottery — the control can draw
//   the 0.6 floor while the elite draws near the ceiling — so the
//   suite proves elevation by the ENVELOPE instead: at this spot
//   (d≈100, maxW≈2.05) an un-elevated bug caps at ~2.7 kg and a
//   3x-scaled one at ~8 kg. The elite's weight then feeds the
//   claim math in Phase 4, which is where the weight is proven
//   exactly. The name index and the nameplate are the identity
//   proof (screenshot).
// Phase 3 (board, loose): with the elite waiting outside (bugs
//   freeze while every player is in the Mall — the cull only hunts
//   non-Mall targets), the board reads LOOSE! and LAST SEEN names
//   the home region (screenshot).
// Phase 4 (kill + claim): the kill runs on the real path (walk and
//   swing, aimed by the elite's position). Bank +elite weight (+-1
//   float rounding), buddy XP exactly +25, done 1, serial 1, the
//   cooldown standing — and a reload pays nothing twice.
// Phase 5 (cooldown gate): force during the cooldown does NOT
//   elevate; force after dbg_bnsetcool(0) does — serial 1's elite.
// Phase 6 (escape): teleporting beyond the 1750 cull leaves the
//   contract standing: serial unmoved, LAST SEEN keeps the region.
// Phase 7 (save/load mid-bounty, realistic state): hat ON (a feed),
//   money banked (the phase-4 claim), elite live — save, reload:
//   hat, bank, serial and ledger intact; the elite itself is AT
//   LARGE again and last-seen honestly forgotten.
// Phase 8 (crafted bn=5,2,0): the board posts replica(5), ledger 2.
// Phase 9 (stripped): the same blob minus bn= posts serial 0 with
//   records and bank intact — a pre-feature save.
// Phase 10 (Zen): a crafted Zen run hunts and claims like any run
//   (the First Shift / Weekly reading: training pays, no record).
// All state sampled right after boot/reload is polled (the Nov 19
// rule: wasm exports answer before the first frame).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

// an independent replica of the derivation in buddies.c:
// bn_name_idx = hash2(serial, 31337, 0xB0B1E5) % 12 and
// bn_kind_of  = hash2(serial, 271828, 0x5EED5) % 4, hash2 being the
// game's unsigned helper. If the tables or seeds drift, this says so.
function hash2(x, y, seed) {
  let h = (seed + Math.imul(x, 374761393) + Math.imul(y, 668265263)) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177) >>> 0;
  return (h ^ (h >>> 16)) >>> 0;
}
const ELITE_NAMES = ['NULLPOINTER', 'SEGFAULT', 'HEISENBUG', 'OFFBYONE', 'DEADLOCK', 'MEMORY LEAK',
                     'RACE CONDITION', 'INFINITE LOOP', 'SPAGHETTI CODE', 'BLUE SCREEN', 'KERNEL PANIC', 'HOTFIX'];
const ELITE_TITLES = ['DEVOURER OF STACKS', 'THE UNHANDLED EXCEPTION', 'GONE WHEN YOU LOOK AT IT',
                      'ONE MORE THAN YOU COUNTED', 'FIRST TO BITE, LAST TO LET GO',
                      'BIGGER EVERY TIME YOU LOOK AWAY', 'ALREADY BEHIND YOU', 'IT COMES BACK. IT COMES BACK.',
                      'A KNOT WITH LEGS', 'THE ONE THAT ENDS THE DAY', 'EVEN THE KERNEL RAN AWAY',
                      'SHIPPED ON A FRIDAY. STILL HERE.'];
const ELITE_KINDS = ['ROACH', 'BEETLE', 'SPIDER', 'MOTH'];
function replica(s) {
  const ni = hash2(s, 31337, 0xB0B1E5) % 12;
  const ki = hash2(s, 271828, 0x5EED5) % 4;
  return { idx: ni, name: ELITE_NAMES[ni], title: ELITE_TITLES[ni], kindIdx: ki, kind: ELITE_KINDS[ki] };
}
const HOME_REGION = 'CLEAN CODE MEADOWS';

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
  const pollSave = async (n, needle, ms = 20000) => {
    const t0 = Date.now(); let s = '';
    while (Date.now() - t0 < ms) { s = (await saveSlot(n)) || ''; if (s.includes(needle)) return s; await sleep(300); }
    return s;
  };
  // deterministic slot loading (weeklytest's pattern): point the
  // global blob's slot= line at the target BEFORE the reload, then
  // pick the preselected row with no navigation at all
  async function loadSlot(n) {
    await page.evaluate(t => {
      const g = localStorage.getItem('tsg_save');
      if (g) localStorage.setItem('tsg_save', g.replace(/^slot=.*$/m, 'slot=' + t));
    }, n);
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
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
  // the bounty board's stand point (west wall), INTERACT it open
  async function openBountyBoard() {
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 300);
    if ((await call('dbg_mall')) !== 1) return false;
    // two legs around the Photo Booth island (v1.25 MALL
    // ENLARGEMENT): the straight diagonal can pin on the island's
    // east face — the target row (-5992) sits inside the island's
    // collision band, so a walker whose y arrives first pushes west
    // into the face forever. Swing south of the island first, then
    // north up the west wall to the board's stand point.
    if (!await walkTo(-366, -5920, 14)) return false;
    if (!await walkTo(-366, -5992, 14)) return false;
    await key('KeyE', 350);
    return (await poll('dbg_station', v => v === 7, 6000)) === 7;
  }
  async function exitMall() {
    // callers close any open panel first — from plain mall floor,
    // the exit is the mat: stand on it (teleport is setup) and
    // INTERACT out, the firstshift idiom
    await call('dbg_tp', 0, -5810); await sleep(250);
    await key('KeyE', 300);
    return (await poll('dbg_mall', v => v === 0, 6000)) === 0;
  }
  async function readBoard() {
    const n = await call('dbg_bnrows');
    const rows = [];
    for (let i = 0; i < n; i++) rows.push([await str('dbg_bnrowname', i), await str('dbg_bnrowright', i)]);
    return rows;
  }
  function rowOf(rows, name) { return rows.find(r => r[0] === name); }
  // swing at whatever the position readers point at until the
  // done-predicate trips — the firstshift walk-and-fire, aimed by
  // the elite's own coordinates for the hunt phases. Two survival
  // rules: if the player died and respawned in the Mall, step back
  // out before anything else (a corpse-walk into the Mall's west
  // wall burned a whole fight in the first solo run); and after
  // each swing, step AWAY — a traded-blows stance loses to a big
  // elite, a kite does not.
  async function fight(readPos, isDone, timeout = 90000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      if (await isDone()) return true;
      if ((await call('dbg_mall')) === 1) {
        await call('dbg_tp', 0, -5810); await sleep(200);
        await key('KeyE', 300); await sleep(200);
        continue;
      }
      const [tx, ty] = await readPos();
      if (tx === -99999) { await sleep(300); continue; }
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = tx - x, dy = ty - y, d = Math.hypot(dx, dy);
      const toward = [], away = [];
      if (dx > 10) { toward.push('ArrowRight'); away.push('ArrowLeft'); }
      else if (dx < -10) { toward.push('ArrowLeft'); away.push('ArrowRight'); }
      if (dy > 10) { toward.push('ArrowDown'); away.push('ArrowUp'); }
      else if (dy < -10) { toward.push('ArrowUp'); away.push('ArrowDown'); }
      if (d > 85) {
        for (const k of toward) await page.keyboard.down(k);
        await sleep(140);
        for (const k of toward) await page.keyboard.up(k);
      } else if (d < 55) {
        // too close: an elite's bite lands inside this radius —
        // step out WITHOUT swinging (trading blows is how the
        // first Zen attempt died)
        for (const k of away) await page.keyboard.down(k);
        await sleep(120);
        for (const k of away) await page.keyboard.up(k);
      } else {
        for (const k of toward) await page.keyboard.down(k);
        await sleep(50);
        await key('Space', 100);
        if (d >= 65) await key('Space', 100);
        for (const k of toward) await page.keyboard.up(k);
        for (const k of away) await page.keyboard.down(k);
        await sleep(130);
        for (const k of away) await page.keyboard.up(k);
      }
    }
    return isDone();
  }
  // force + spawn + read in ONE evaluate: no frame runs between the
  // arming and the spawn, so no natural spawn can steal the force
  // and no other bug can pollute the read
  const forceSpawn = (x, y) => page.evaluate(([sx, sy]) => {
    window.__TSG.dbg_bnforce();
    window.__TSG.dbg_spawnbug(sx, sy);
    return [window.__TSG.dbg_bnelite(), window.__TSG.dbg_bnelitew()];
  }, [x, y]);

  // ============ PHASE 1: a fresh slot posts serial 0 =============
  const exp0 = replica(0), exp1 = replica(1), exp5 = replica(5);
  ok((await call('dbg_state')) === 0, '1: boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, '1: fresh run started');
  ok((await poll('dbg_bnserial', v => v === 0)) === 0, '1: the first bounty is serial 0');
  ok((await call('dbg_bndone')) === 0, '1: the ledger starts empty');
  ok((await str('dbg_bnname')) === exp0.name, `1: serial 0 posts ${exp0.name} (replica)`, await str('dbg_bnname'));
  ok((await str('dbg_bntitle')) === exp0.title, '1: the title matches the replica', await str('dbg_bntitle'));
  ok((await call('dbg_bnkind')) === exp0.kindIdx, `1: the kind matches the replica (${exp0.kind})`, await call('dbg_bnkind'));
  ok((await call('dbg_bnelite')) === -1, '1: no elite is live yet');
  // NOTE: no buddy is adopted until Phase 4 — on purpose. A buddy
  // hunts on its own and WILL solo a small elite given a minute
  // (it did exactly that to a 2 kg MEMORY LEAK in solo run 2,
  // settling the claim before the board could be read). With no
  // buddy, the claim's +25 XP lands in the First Shift tuition
  // pool instead — a counter nothing else in the game can move —
  // which is what makes Phase 4's exact-payment proof airtight.
  ok(await openBountyBoard(), '1: walked the mall and opened the BOUNTY BOARD (station 7)');
  ok((await call('dbg_state')) === 2, '1: the board panel is a store panel (GS_STORE)');
  const rows1 = await readBoard();
  ok(rows1.length === 5, '1: the board has five rows', rows1.length);
  ok(rowOf(rows1, 'WANTED') !== undefined, '1: a WANTED header row');
  const b1 = rowOf(rows1, exp0.name);
  ok(!!b1 && b1[1] === 'AT LARGE', '1: the bounty row names the elite, AT LARGE', b1 && b1.join(' | '));
  const s1 = rowOf(rows1, 'LAST SEEN');
  ok(!!s1 && s1[1] === 'NOWHERE YET', '1: LAST SEEN starts at NOWHERE YET', s1 && s1.join(' | '));
  const c1 = rowOf(rows1, 'BOUNTIES CLAIMED');
  ok(!!c1 && c1[1] === '0', '1: BOUNTIES CLAIMED starts at 0', c1 && c1.join(' | '));
  const blurb1 = await str('dbg_bnblurb', rows1.findIndex(r => r[0] === exp0.name));
  ok(blurb1.startsWith(exp0.title) && blurb1.includes('A ' + exp0.kind),
     '1: the bounty blurb carries the replica title and kind', blurb1);
  await key('Escape', 300);

  // ====== PHASE 2: summon — the elite is ~3x its plain spawn =====
  ok(await exitMall(), '2: stepped back out of the mall');
  // clear the field: everything left near home is culled once the
  // player stands 5000+ px away (the 1750 rule), so the reads below
  // see only the suite's own spawns
  await call('dbg_tp', 5000, 5000); await sleep(600);
  await call('dbg_tp', 0, 80); await sleep(300);
  const px = await call('dbg_px'), py = await call('dbg_py');
  // the control's OWN weight, read off the bug itself in the same
  // evaluate that spawns it (dbg_bugidx/dbg_bugw) — a field-wide
  // max would be polluted by any natural spawn that wandered in
  const ctl10 = await page.evaluate(([sx, sy]) => {
    window.__TSG.dbg_spawnbug(sx, sy);
    return window.__TSG.dbg_bugw(window.__TSG.dbg_bugidx(sx, sy));
  }, [px + 45, py + 10]);
  ok(ctl10 > 0 && ctl10 <= 30, '2: control spawn weighs in (x10 kg)', ctl10);
  const k0 = await call('dbg_kills');
  ok(await fight(async () => [await call('dbg_bugx', 0), await call('dbg_bugy', 0)],
                 async () => (await call('dbg_kills')) > k0, 60000),
     '2: control bug squashed by hand');
  await call('dbg_feed');   // a hat of feed carcasses: First Shift's hat step, settled before the claim math
  ok((await poll('dbg_fs', v => v === 12)) === 12, '2: First Shift settled at its wall step (no XP surprises later)', await call('dbg_fs'));
  const pool0 = await call('dbg_fspool'), carried0 = await call('dbg_carried'), bank0 = await call('dbg_bank');
  ok(pool0 === 10, '2: the tuition pool holds the two First Shift payments (5 + 5)', pool0);
  const [eliteIdx, eliteW] = await forceSpawn(px + 45, py + 10);
  ok(eliteIdx === exp0.idx, `2: the forced spawn IS ${exp0.name} (name index ${exp0.idx})`, eliteIdx);
  ok(eliteW >= 2 && eliteW <= 8,
     `2: the elite weighs in the 3x elevation envelope for this spot (${eliteW} kg; un-elevated caps ~2.7 kg)`, eliteW);
  ok((await poll('dbg_bnlive', v => v === 1)) === 1, '2: the tracker latches the live elite');
  // the nameplate shot FIRST, while the elite is still beside the
  // player (it closes fast, and a late shot hides the plate behind
  // the player's own hat stack — as the first attempt showed)
  await sleep(250);
  { const cv = await page.$('canvas');
    await cv.screenshot({ path: __dirname + '/tsg-v125-f8-elite-field.png' }); }
  // then step well away from it before the Mall trip: the elite
  // gives chase at bug speed, so distance is time — and nothing in
  // this suite may stand next to it unattended
  await call('dbg_tp', px - 250, py + 200);
  await sleep(400);

  // ====== PHASE 3: the board, with the elite loose outside =======
  ok(await openBountyBoard(), '3: opened the BOUNTY BOARD with the elite loose');
  const rows3 = await readBoard();
  const b3 = rowOf(rows3, exp0.name);
  ok(!!b3 && b3[1] === 'LOOSE!', '3: the bounty row reads LOOSE!', b3 && b3.join(' | '));
  const s3 = rowOf(rows3, 'LAST SEEN');
  ok(!!s3 && s3[1] === HOME_REGION, '3: LAST SEEN names the home region while it hunts there', s3 && s3.join(' | '));
  await page.screenshot({ path: __dirname + '/tsg-v125-f8-board-loose.png' });
  await key('Escape', 300);
  ok(await exitMall(), '3: back out of the mall — the elite waited');

  // ============ PHASE 4: the kill, the claim, once ===============
  ok(await fight(async () => [await call('dbg_bnx'), await call('dbg_bny')],
                 async () => (await call('dbg_bndone')) === 1, 120000),
     '4: the named elite is DOWN (killed on the real path)');
  ok((await call('dbg_bndone')) === 1, '4: the ledger counts the claim');
  ok((await call('dbg_bnserial')) === 1, '4: the board advanced to serial 1');
  ok((await str('dbg_bnname')) === exp1.name, `4: the next name chalked up is ${exp1.name}`, await str('dbg_bnname'));
  const bankDelta = (await call('dbg_bank')) - bank0;
  ok(Math.abs(bankDelta - eliteW) <= 1, `4: the killer banked the elite's weight again (${bankDelta} vs ${eliteW} kg)`, bankDelta);
  // no buddy exists yet, so the claim's XP went to the tuition
  // pool — the one counter with no other writers. Exactly +25.
  ok((await poll('dbg_fspool', v => v === pool0 + 25)) === pool0 + 25,
     '4: the claim paid exactly +25 XP (into the tuition pool — no buddy yet)', await call('dbg_fspool'));
  ok((await poll('dbg_carried', v => v >= carried0 + (eliteW - 2) * 10, 8000)) >= carried0 + (eliteW - 2) * 10,
     '4: the elite carcass rides the killer\'s hat', (await call('dbg_carried')) + ' vs ' + carried0);
  ok((await call('dbg_bncool')) > 100, '4: the cooldown stands after a claim', await call('dbg_bncool'));
  ok((await call('dbg_bnelite')) === -1, '4: no second elite is live');
  const blob4 = await pollSave(1, 'bn=1,1,');
  ok(/^bn=1,1,/m.test(blob4), '4: the claim persisted in the bn= line', (blob4.match(/^bn=.*$/m) || [''])[0]);
  // adoption flushes the pool whole — the buddy's first XP is the
  // First Shift payments plus the claim's 25, to the point
  await call('dbg_adopt', 0);
  ok((await poll('dbg_buddyxp', v => v === pool0 + 25)) === pool0 + 25,
     '4: adoption flushes the pool — the claim XP lands whole', await call('dbg_buddyxp'));
  // freeze the ambient economy before comparing across a reload:
  // in the Mall the buddy cannot hunt, so bank and XP hold still
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', v => v === 1)) === 1, '4: stepped into the mall to freeze the ledger');
  await sleep(6500);   // one full autosave cycle on frozen values
  const bankA = await call('dbg_bank'), xpA = await call('dbg_buddyxp');
  ok(await loadSlot(1), '4: slot 1 reloads');
  await sleep(1200);
  ok((await call('dbg_bndone')) === 1 && (await call('dbg_bnserial')) === 1, '4: no second claim after reload');
  ok((await call('dbg_bank')) === bankA && (await call('dbg_buddyxp')) === xpA, '4: bank and XP unmoved by the reload');

  // ====== PHASE 5: the cooldown gate, then serial 1's elite ======
  // the reload respawned the player at the Mall centre — step out
  // first, or the "elite" would spawn at Mall coordinates and the
  // escape reading below would name the Mall's own far region
  ok(await exitMall(), '5: stepped out of the mall after the reload');
  const px5 = await call('dbg_px'), py5 = await call('dbg_py');
  const [gateIdx] = await forceSpawn(px5 + 45, py5 + 10);
  ok(gateIdx === -1, '5: force during the cooldown does NOT elevate', gateIdx);
  await call('dbg_bnsetcool', 0);
  const [elite1Idx] = await forceSpawn(px5 + 45, py5 + 10);
  ok(elite1Idx === exp1.idx, `5: after the cooldown, the force raises ${exp1.name}`, elite1Idx);
  ok((await poll('dbg_bnlive', v => v === 1)) === 1, '5: the tracker latches serial 1\'s elite');

  // ============ PHASE 6: it slips away; the contract stands ======
  await call('dbg_bnsetcool', 9999);   // keep the spawner out of the escape reading
  await call('dbg_tp', 4000, 0);
  ok((await poll('dbg_bnlive', v => v === 0, 10000)) === 0, '6: beyond the cull, the elite is gone');
  ok((await call('dbg_bnserial')) === 1 && (await call('dbg_bndone')) === 1, '6: an escape moves neither serial nor ledger');
  const rows6 = await readBoard();
  const s6 = rowOf(rows6, 'LAST SEEN');
  ok(!!s6 && s6[1] === HOME_REGION, '6: LAST SEEN keeps the region it slipped away from', s6 && s6.join(' | '));
  await call('dbg_tp', 0, 80); await sleep(300);

  // ====== PHASE 7: save/load mid-bounty, realistic state =========
  const bankB = await call('dbg_bank');
  await call('dbg_feed');   // hat ON for the save
  ok((await call('dbg_carried')) >= 95, '7: a hat is on for the save', await call('dbg_carried'));
  await call('dbg_bnsetcool', 0);
  const px7 = await call('dbg_px'), py7 = await call('dbg_py');
  const [elite7] = await forceSpawn(px7 + 45, py7 + 10);
  ok(elite7 === exp1.idx, '7: the escaped elite crawls out again on a later roll', elite7);
  ok((await poll('dbg_bnlive', v => v === 1)) === 1, '7: it is live at save time');
  // step into the Mall before waiting on the save: the elite
  // freezes outside (it cannot be buddy-killed mid-save), and the
  // autosave still writes the contract
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', v => v === 1)) === 1, '7: stepped into the mall — the elite waits outside');
  const blob7 = await pollSave(1, 'bn=1,1,');
  ok(/^bn=1,1,/m.test(blob7), '7: the mid-bounty save carries the contract', (blob7.match(/^bn=.*$/m) || [''])[0]);
  ok(await loadSlot(1), '7: slot 1 reloads mid-bounty');
  ok((await call('dbg_bank')) === bankB, '7: the bank survived', await call('dbg_bank'));
  ok((await call('dbg_carried')) >= 95, '7: the hat survived', await call('dbg_carried'));
  ok((await call('dbg_bnserial')) === 1 && (await call('dbg_bndone')) === 1, '7: serial and ledger survived');
  ok((await call('dbg_bnelite')) === -1, '7: the elite itself is NOT in the save — it must crawl out again');
  const rows7 = await readBoard();
  const b7 = rowOf(rows7, exp1.name);
  ok(!!b7 && b7[1] === 'AT LARGE', '7: the board reads AT LARGE after the load', b7 && b7.join(' | '));
  const s7 = rowOf(rows7, 'LAST SEEN');
  ok(!!s7 && s7[1] === 'NOWHERE YET', '7: last-seen is runtime memory, honestly forgotten', s7 && s7.join(' | '));

  // ====== PHASE 8: a crafted bn= line posts its serial ============
  const setTok = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);
  const blobNow = await pollSave(1, 'bn=1,1,');
  const crafted = blobNow.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    return setTok(line, 'bx', 777);
  }).join('\n').replace(/^bn=.*$/m, 'bn=5,2,0');
  ok(crafted.includes('bn=5,2,0') && crafted.includes(' bx=777'), '8: crafted blob carries bn=5,2,0');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), crafted);
  ok(await loadSlot(3), '8: crafted slot 3 loads');
  ok((await call('dbg_buddyxp')) === 777, '8: this IS slot 3 (marker XP)', await call('dbg_buddyxp'));
  ok((await poll('dbg_bnserial', v => v === 5)) === 5, '8: the crafted serial posts', await call('dbg_bnserial'));
  ok((await call('dbg_bndone')) === 2, '8: the crafted ledger reads 2');
  ok((await str('dbg_bnname')) === exp5.name, `8: serial 5 posts ${exp5.name} (replica)`, await str('dbg_bnname'));

  // ====== PHASE 9: a pre-feature save (no bn= line) loads =========
  const stripped = blobNow.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    return setTok(line, 'bx', 778);
  }).join('\n').replace(/^bn=.*\n/m, '');
  ok(!/^bn=/m.test(stripped) && stripped.includes(' bx=778'), '9: stripped blob has no bn= line');
  await page.evaluate(s => localStorage.setItem('tsg_save2', s), stripped);
  ok(await loadSlot(2), '9: pre-feature slot 2 loads into play');
  ok((await call('dbg_buddyxp')) === 778, '9: this IS slot 2 (marker XP)', await call('dbg_buddyxp'));
  ok((await call('dbg_bnserial')) === 0 && (await call('dbg_bndone')) === 0, '9: serial 0, clean ledger');
  ok((await str('dbg_bnname')) === exp0.name, '9: the board posts serial 0 on the old save');
  ok((await call('dbg_bank')) === bankB, '9: the bank is intact from the old save', await call('dbg_bank'));

  // ============ PHASE 10: Zen hunts and claims ===================
  const zenCraft = blobNow.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    let L = setTok(line, 'bx', 555);
    L = setTok(L, 'bk', 200);   // the bk token stores DOUBLE kg (save.c writes banked*2) — this is banked 100
    return L;
  }).join('\n')
    .replace(/^zrun=.*$/m, 'zrun=1')
    .replace(/^bn=.*$/m, 'bn=0,0,0');
  ok(zenCraft.includes('zrun=1') && zenCraft.includes('bn=0,0,0') && zenCraft.includes(' bx=555'), '10: Zen blob crafted');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), zenCraft);
  ok(await loadSlot(3), '10: Zen slot 3 loads');
  ok((await call('dbg_zen')) === 1, '10: the run is Zen', await call('dbg_zen'));
  ok((await call('dbg_buddyxp')) === 555, '10: marker XP — this is the Zen slot', await call('dbg_buddyxp'));
  await call('dbg_tp', 5000, 5000); await sleep(600);
  await call('dbg_tp', 0, 80); await sleep(300);
  const pxZ = await call('dbg_px'), pyZ = await call('dbg_py');
  const [eliteZ, eliteZW] = await forceSpawn(pxZ + 45, pyZ + 10);
  ok(eliteZ === exp0.idx, '10: the bounty runs in Zen — the elite rises', eliteZ);
  const bankZ0 = await call('dbg_bank');
  ok(bankZ0 === 100, '10: the Zen bank starts at the crafted 100', bankZ0);
  ok(await fight(async () => [await call('dbg_bnx'), await call('dbg_bny')],
                 async () => (await call('dbg_bndone')) === 1, 240000),
     '10: the Zen elite is DOWN');
  // the +25 claim XP is in there (Phase 4 proved the payment exact
  // via the pool); here the buddy exists and may legitimately add
  // its own kill/assist credit, so the floor is what proves Zen pays
  ok((await poll('dbg_buddyxp', v => v >= 580)) >= 580, '10: Zen pays the +25 XP too (555 -> at least 580)', await call('dbg_buddyxp'));
  // the bank, exactly: nothing in the game takes banked kg on
  // death (checked in the source — a faint costs the hat's risk,
  // never the bank), so the claim's payment shows up whole
  const bankZ1 = await call('dbg_bank');
  ok(Math.abs(bankZ1 - (bankZ0 + eliteZW)) <= 1,
     `10: Zen banks the weight again too (${bankZ1 - bankZ0} vs ${eliteZW} kg)`, bankZ1);

  console.log(`\nELITETEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
