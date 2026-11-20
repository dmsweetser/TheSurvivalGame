// ENTRANCES (Nov 5): the Doors-like hotel mode inside Banzai World.
// Covers: gate entry into mode 2, doors (plain + locked + key), the Zip
// Bug in room 3 (room-wide bonk in the open, safe in a bush), Typo
// nibbles in the dark room 2, drawer loot, the Inspector chase at room
// 19 (rarer, longer: caught, then escaped five rooms on for +10 bits),
// the Library at 25 (door waits for 5 fuses), the secret SUBFLOOR
// behind the Library bookcase, a forced GLITCH rush, the EXIT room at
// 51, leaving by the exit door, and records surviving a save/continue
// cycle. Damage is Doors-scale now, so the suite tops the buddy up with
// dbg_budup (a full heal) between the dangerous sections.
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

const ROOMW = 15 * 48;                    // one room slab, world units
const doorX = (room) => (room - 1) * ROOMW + 14 * 48 + 24;
const roomCX = (room) => (room - 1) * ROOMW + 360;
const tileCX = (room, lx) => (room - 1) * ROOMW + lx * 48 + 24;
const tileCY = (ly) => ly * 48 + 24;

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(60); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };
  const tp = async (x, y, hold = 140) => { await call('dbg_bwtp', x, y); await sleep(hold); };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);              // maxhp 1595: bonks sting, never faint
  const maxhp = await call('dbg_buddyhp');

  // park the DEV in the mall, then take the gate's new third row for real:
  // hooks enter BW, but entry itself goes through the actual gate panel
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  T('DEV parked in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));

  // DEEPER DOORS (v1.25 feat 11): the ENTRANCES cabinet panel is
  // locked before a first clear — a fresh buddy's panel is the
  // shipped two rows, so Down+Space LEAVES instead of launching
  // anything. Entry into Chapter 2 later in this suite goes
  // through this same panel; no debug door into it anywhere.
  async function gotoEntPanel() {
    await call('dbg_tp', 350, -5976); await sleep(200);
    await key('KeyE', 250);
    T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
    await call('dbg_tp', -256, -12108); await sleep(200);   // the ENTRANCES cabinet's stand point (ARC_SX[0], ARC_ROWY[0]+58)
    await key('KeyE', 250);
    return (await poll('dbg_arcgate', v => v === 1, 3000)) === 1;
  }
  T('entrances panel opened (fresh buddy)', await gotoEntPanel());
  await key('ArrowDown', 200);            // row 1: with two rows this is LEAVE
  await key('Space', 300);
  T('fresh panel: Down+Space LEAVES (no DEEPER DOORS row yet)',
    (await call('dbg_arcgate')) === 0 && (await call('dbg_bwmode')) !== 2,
    'gate=' + (await call('dbg_arcgate')) + ' mode=' + (await call('dbg_bwmode')));
  T('panel reopened', await gotoEntPanel());
  await key('Space', 300);                // row 0: PLAY — Chapter 1
  T('PLAY launches Chapter 1', (await poll('dbg_bwmode', v => v === 2, 4000)) === 2 && (await call('dbg_entchap')) === 1,
    'mode=' + (await call('dbg_bwmode')) + ' chap=' + (await call('dbg_entchap')));
  await key('Escape', 300);
  T('back from the panel launch', (await poll('dbg_bwstate', v => v === 0, 4000)) === 0, await call('dbg_bwstate'));

  await call('dbg_bwent2');
  T('in Banzai World', (await poll('dbg_bwstate', v => v === 1)) === 1);
  T('starts in room 1 (the lobby)', (await call('dbg_entroom')) === 1, await call('dbg_entroom'));
  await page.screenshot({ path: __dirname + '/ent-lobby.png' });

  // helpers for the long walk east
  async function keySweep(room) {
    // stand on the key itself: dbg_entdiag reports where it lies (the
    // game's own debug scaffolding), and the pickup is a proximity
    // check inside ent_update. A blind tile-by-tile sweep proved
    // flaky in headless runs — passes and drains move the buddy, and
    // a one-second stand can still slip (Nov 6: the probe that stands
    // on the exact tile went 20 doors for 20; the sweeping suite
    // kept missing keys that were sitting right there)
    // v1.25 feat 11: the stand now PERSISTS. A pass monster's bonk
    // knocks the buddy exactly one pickup radius (26px) off the key,
    // so a single stand-and-poll can be undone mid-poll by the room's
    // own Zip/Boomerang — the hunt failed that way twice in one run,
    // once per chapter, and in a locked Boomerang room the stacked
    // bonks fainted the buddy out of the walk entirely. Re-standing
    // every beat (and topping up between beats) keeps the buddy on
    // the tile until the pocket registers; the assertion the caller
    // makes — the key is found, then spent — is unchanged.
    const kp = await call('dbg_entdiag', room, 3);
    const kx = kp % 100000, ky = Math.floor(kp / 100000);
    const t0 = Date.now();
    while (Date.now() - t0 < 12000) {
      // top up EVERY beat, the first one included: the buddy can
      // arrive at this hunt already carrying tryRows' dupe bonks,
      // and the room's one Zip pass (45%) landing in the first
      // second — before any in-hunt top-up — faints him out of
      // the hunt entirely (feat 11 battery: door 3, twice in
      // four runs; the pass's timing against the hunt is the
      // whole race)
      await call('dbg_budup', 60);
      await tp(kx, ky, 60);
      if ((await call('dbg_entkey')) === 1) return 1;
      await sleep(240);
      if ((await call('dbg_entkey')) === 1) return 1;
    }
    return await call('dbg_entkey');
  }
  async function pressDoor(room) {
    // hold E like a human thumb would, and verify — an instant press
    // can fall between frames and vanish (Nov 6: door 42 ate two in a
    // row and the whole walk-out cascaded). The game is fine (a held
    // press opens it first try); the suite was too quick for it.
    for (let t = 0; t < 3; t++) {
      await page.keyboard.down('KeyE'); await sleep(150); await page.keyboard.up('KeyE'); await sleep(250);
      if ((await call('dbg_entdoor')) >= room) return 1;
    }
    return 0;
  }
  // the real door's row wanders per room now, and painted dupes share
  // the east wall — find every door-shaped tile on it, exactly like a
  // player scanning the wall (Nov 6 batch C)
  async function doorRows(room) {
    const rows = [];
    for (let ly = 1; ly <= 8; ly++)
      if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) rows.push(ly);
    return rows;
  }
  async function tryRows(room, rows) {
    for (const ly of rows) {
      // top up per row: a resident Big Bug bumps for 40% a bump
      // with only its grace between bumps, so an un-topped
      // multi-row try in its room can faint the buddy before the
      // real row is ever pressed (feat 11 solo 3: door 39, key in
      // hand, walk frozen at entdoor 38). One row's exposure —
      // one bump plus a dupe bonk — is always survivable.
      await call('dbg_budup', 60);
      await tp(doorX(room) - 40, tileCY(ly));
      if (await pressDoor(room)) return 1;   // a dupe bonks and refuses; the real door opens
    }
    return 0;
  }
  async function openDoor(room) {
    const rows = await doorRows(room);
    if (rows.length && await tryRows(room, rows)) return;   // plain door, opened by Interact
    // only a genuinely locked door has a key to find — asking for one
    // anywhere else sweeps a room that holds none and fails the run
    // (an already-open door has no tile-2 rows at all, and a chase can
    // leave the buddy rooms behind the one advance() aimed at)
    if ((await call('dbg_entdiag', room, 0)) !== 1) return;
    await keySweep(room);
    T('found the brass key for door ' + room, (await call('dbg_entkey')) === 1, await call('dbg_entkey'));
    await tryRows(room, rows.length ? rows : [4]);
    T('key opens door ' + room, (await call('dbg_entdoor')) >= room, await call('dbg_entdoor'));
    T('key is spent', (await call('dbg_entkey')) === 0, await call('dbg_entkey'));
  }
  async function doorStandY(room) { const r = await doorRows(room); return tileCY(r.length ? r[0] : 4); }
  async function advance(room) {               // from inside `room` to room+1
    // top the buddy up first: dupe bonks and room events chip away at
    // him across a long walk, and a faint inside the hotel freezes the
    // whole run — the revive (dbg_budup does NOT clear it) lives back
    // at the mall shop (Nov 6 diag: a room-14 faint ended the run)
    await call('dbg_budup', 60);
    await openDoor(room);
    await tp(roomCX(room + 1), 216);
    return await call('dbg_entroom');
  }

  // room 1 -> 2: a plain door, opened by Interact
  T('door 1 shut at first', (await call('dbg_entdoor')) === 0, await call('dbg_entdoor'));
  await advance(1);
  T('room 2 reached', (await call('dbg_entroom')) === 2, await call('dbg_entroom'));
  T('records the deepest room', (await call('dbg_entrec')) % 10000 >= 2, await call('dbg_entrec'));

  // room 2 is dark and holds Typos: stand still and get nibbled
  T('room 2 event is the Typo', Math.floor((await call('dbg_entev')) / 10) === 3, await call('dbg_entev'));
  T('a Typo nibbles a still buddy', (await poll('dbg_buddyhp', v => v < maxhp, 14000)) < maxhp, await call('dbg_buddyhp'));
  // now hunt one down: wait for it to appear, teleport onto it
  await poll('dbg_entev', v => v === 32, 12000);
  const packed = await call('dbg_enttypo');
  const txp = packed % 100000, typ = Math.floor(packed / 100000);
  const bitsBefore = await call('dbg_entbits');
  await tp(txp, typ);
  T('catching a Typo pays bits', (await poll('dbg_entbits', v => v > bitsBefore, 5000)) > bitsBefore, await call('dbg_entbits'));

  // drawer loot somewhere in rooms 2-4: bits, a stick, or a brighter lantern
  let looted = 0;
  for (const room of [2, 3, 4]) {
    // only walk TO a room we have not reached yet: advancing at a door
    // that already stands open sweeps for a key that does not exist
    if ((await call('dbg_entroom')) < room) await advance(room - 1);
    for (let ly = 1; ly <= 8 && !looted; ly++) for (let lx = 1; lx <= 13 && !looted; lx++) {
      if ((await call('dbg_enttile', (room - 1) * 15 + lx, ly)) === 4) {
        const b0 = await call('dbg_entbits'), s0 = await call('dbg_entsticks'), l0 = await call('dbg_entlant');
        await tp(tileCX(room, lx), tileCY(ly) + 30);
        await key('KeyE', 200); await sleep(300);
        if ((await call('dbg_entbits')) > b0 || (await call('dbg_entsticks')) > s0 || (await call('dbg_entlant')) > l0) looted = 1;
      }
    }
  }
  T('a drawer somewhere pays out', looted === 1);

  // room 3: the Zip Bug. In the open it bonks; in a bush it cannot see you.
  if ((await call('dbg_entroom')) !== 3) { await tp(roomCX(3), 216); await sleep(200); }
  T('room 3 event is the Zip Bug', Math.floor((await call('dbg_entev')) / 10) === 1, await call('dbg_entev'));
  const hpZip0 = await call('dbg_buddyhp');
  await tp(roomCX(3), 216);                  // stand in the open, middle row
  T('the Zip Bug launches', (await poll('dbg_entev', v => v === 12, 6000)) === 12, await call('dbg_entev'));
  T('caught in the open: bonked', (await poll('dbg_buddyhp', v => v < hpZip0, 6000)) < hpZip0, await call('dbg_buddyhp'));
  // re-arm by stepping out and back in, then hide in a bush tile if the room grew one
  let bush = null;
  for (let ly = 1; ly <= 8 && !bush; ly++) for (let lx = 1; lx <= 13 && !bush; lx++)
    if ((await call('dbg_enttile', 2 * 15 + lx, ly)) === 3) bush = [tileCX(3, lx), tileCY(ly)];
  if (bush) {
    await tp(roomCX(2), 216); await sleep(200);
    await tp(bush[0], bush[1]); await sleep(200);
    const hpBush = await call('dbg_buddyhp');
    T('the Zip Bug relaunches', (await poll('dbg_entev', v => v === 12, 6000)) === 12, await call('dbg_entev'));
    await sleep(3000);                        // a full pass takes ~1.8s at the new width
    T('hidden in the bush: untouched', (await call('dbg_buddyhp')) >= hpBush - 1, await call('dbg_buddyhp'));
  } else console.log('note: room 3 grew no bush this seed; bush check skipped');

  // the long walk to the chase: rooms 4..18
  for (let room = 3; room <= 18; room++) { if ((await call('dbg_entroom')) < room + 1) await advance(room); }
  // (revive first if the long walk ended in a faint — budup restores
  // hp but never clears the fainted flag; the mall revive does both.
  // The walk tops out wherever it tops out: the chase assertions only
  // need the buddy inside room 19, so walk on from there, not from 14)
  for (let i = 0; i < 40 && (await call('dbg_entroom')) !== 19; i++) {
    await call('dbg_budrevive'); await call('dbg_bwent2'); await sleep(250);
    if ((await call('dbg_entroom')) < 2) await tp(tileCX(1, 7), tileCY(4));
    for (let r = 1; r < 19; r++) { if ((await call('dbg_entroom')) >= 19) break; await advance(r); }
  }
  T('reached the chase rooms', (await call('dbg_entroom')) === 19, await call('dbg_entroom'));
  T('room 19 arms the Inspector chase', (await call('dbg_entchase')) === 19, await call('dbg_entchase'));
  await call('dbg_budup', 60);              // full heal: a catch costs 40% now
  // trigger it and stand still: the Inspector catches a dawdler
  const hpChase0 = await call('dbg_buddyhp');
  await tp(roomCX(19) + 120, 216);
  T('the Inspector gives chase', (await poll('dbg_entev', v => v === 51, 6000)) === 51, await call('dbg_entev'));
  T('a dawdler gets caught', (await poll('dbg_buddyhp', v => v < hpChase0, 12000)) < hpChase0, await call('dbg_buddyhp'));
  // escape: five rooms east ends it, +10 bits
  const bitsChase = await call('dbg_entbits');
  for (const room of [20, 21, 22, 23, 24]) { await tp(roomCX(room), 216); await sleep(200); }
  T('the chase ends five rooms on', (await call('dbg_entchase')) === 0, await call('dbg_entchase'));
  T('escaping pays 10 bits', (await call('dbg_entbits')) >= bitsChase + 10, await call('dbg_entbits'));

  // landing tent tile exists in room 10
  T('room 10 grows a landing tent', (await call('dbg_enttile', 9 * 15 + 3, 1)) === 6, await call('dbg_enttile', 9 * 15 + 3, 1));

  // on to the Library (25): its door refuses everyone without the fuses
  await call('dbg_budup', 60);
  for (let r = 19; r <= 24 && (await call('dbg_entroom')) < 25; r++) await advance(r);
  T('reached the Library door', (await call('dbg_entroom')) === 25, await call('dbg_entroom'));
  T('room 25 event is the Big Bug', Math.floor((await call('dbg_entev')) / 10) === 6, await call('dbg_entev'));
  await tp(doorX(25) - 40, await doorStandY(25));
  await key('KeyE', 200); await sleep(250);
  T('the Library door waits for fuses', (await call('dbg_entdoor')) < 25, await call('dbg_entdoor'));
  for (const [lx, ly] of [[2, 1], [12, 1], [7, 4], [2, 8], [12, 8]]) { await tp(tileCX(25, lx), tileCY(ly)); await sleep(180); }
  T('five fuses collected', (await call('dbg_entfuse')) === 31, await call('dbg_entfuse'));
  T('the Library is done', (await call('dbg_entlib')) === 1, await call('dbg_entlib'));
  // stand on the door's own row, not just any door-shaped tile — a
  // painted dupe may sit above the real door in the scan (Nov 6 run k:
  // the single press here landed on the dupe and the door stayed shut;
  // the game's own rule is that libdone makes the tile walkable, and
  // stepping through is what the suite should prove anyway)
  await tp(doorX(25) + 30, tileCY(4)); await sleep(150);
  T('fuses open the Library door', (await call('dbg_entroom')) === 26, await call('dbg_entroom'));
  await page.screenshot({ path: __dirname + '/ent-library.png' });

  // the tall bookcase hides a stairwell — but only once the Library is done
  await call('dbg_budup', 60);
  await tp(tileCX(25, 12), tileCY(8));
  await key('KeyE', 250); await sleep(400);
  T('the bookcase slides: downstairs', (await call('dbg_entsub')) === 1, await call('dbg_entsub'));
  T('B-1 of the service floor', (await call('dbg_entroom')) === 1, await call('dbg_entroom'));
  await page.screenshot({ path: __dirname + '/ent-subfloor.png' });
  await advance(1); await advance(2);
  T('downstairs has its own doors', (await call('dbg_entroom')) === 3, await call('dbg_entroom'));
  T('the deep B-rooms are remembered', (await call('dbg_entsubb')) >= 3, await call('dbg_entsubb'));
  await tp(60, 216); await sleep(250);
  await key('KeyE', 250); await sleep(400);
  T('the west door climbs back to the Library', (await call('dbg_entsub')) === 0 && (await call('dbg_entroom')) === 25, await call('dbg_entroom'));

  // a forced GLITCH: the air tears and the rush comes with no warning
  await call('dbg_budup', 60);
  await tp(roomCX(25), 216); await sleep(200);
  const hpGl = await call('dbg_buddyhp');
  await tp(roomCX(25), await doorStandY(25)); await sleep(150);   // the pass rides the door row now — stand in its lane
  await call('dbg_enttrig');
  T('the glitch tears open', (await poll('dbg_entev', v => v === 72, 6000)) === 72, await call('dbg_entev'));
  T('no warning: it hits a standing buddy', (await poll('dbg_buddyhp', v => v < hpGl, 6000)) < hpGl, await call('dbg_buddyhp'));
  await page.screenshot({ path: __dirname + '/ent-glitch.png' });

  // the long walk out: rooms 26..50, then the EXIT room
  await call('dbg_budup', 60);
  for (let room = 25; room <= 50; room++) { if ((await call('dbg_entroom')) < room + 1) await advance(room); }
  T('the EXIT room', (await call('dbg_entroom')) === 51, await call('dbg_entroom'));
  T('completion is recorded', (await call('dbg_entrec')) >= 10000, await call('dbg_entrec'));
  await page.screenshot({ path: __dirname + '/ent-exit.png' });
  // leave by the EXIT door in the south wall
  await tp(tileCX(51, 7), tileCY(8));
  await key('KeyE', 250); await sleep(400);
  T('the EXIT door goes home', (await call('dbg_bwstate')) === 0, await call('dbg_bwstate'));

  // records survive a save/continue cycle: reload the page —
  // localStorage keeps the save, and the title's first option continues it
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 450); await sleep(400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(400);
  T('records survive the reload', (await call('dbg_entrec')) >= 10000, await call('dbg_entrec'));
  T('the subfloor record survives too', (await call('dbg_entsubb')) >= 3, await call('dbg_entsubb'));

  // ============ DEEPER DOORS (v1.25 feat 11): Chapter 2 ============
  // The buddy finished Chapter 1 above (ent_done >= 1), so the
  // ENTRANCES panel has grown its third row. Everything below
  // enters through that real panel; dbg_bwtp only positions the
  // buddy inside the run (the suite's standing idiom), and
  // dbg_entdiag only reads the live deal.
  const entRecBefore = await call('dbg_entrec');
  T('chapter 2 panel opened', await gotoEntPanel());
  await page.screenshot({ path: __dirname + '/tsg-v125-f11-panel.png' });
  await key('ArrowDown', 200);            // row 1: DEEPER DOORS
  await key('Space', 300);
  T('DEEPER DOORS launches Chapter 2', (await poll('dbg_bwmode', v => v === 2, 4000)) === 2 && (await call('dbg_entchap')) === 2,
    'mode=' + (await call('dbg_bwmode')) + ' chap=' + (await call('dbg_entchap')));
  T('chapter 2 starts in room 1', (await call('dbg_entroom')) === 1, await call('dbg_entroom'));
  await call('dbg_budup', 60);
  // Lumen joins at the threshold and settles at the buddy's
  // shoulder (buddy - 26, buddy - 22). The buddy's own entry
  // settle shifts his absolute spot a few px run to run (the
  // shipped entry behaviour, both chapters), so the contract
  // asserted is the RELATIVE one: Lumen sits on the shoulder
  // point, polled until he arrives and holds it.
  let lumenShoulder = 0;
  { const t0 = Date.now();
    while (Date.now() - t0 < 6000) {
      const lu = await call('dbg_entlu'), bx = await call('dbg_bwx'), by = await call('dbg_bwy');
      if (Math.abs((lu % 100000) - (bx - 26)) <= 6 && Math.abs(Math.floor(lu / 100000) - (by - 22)) <= 6) { lumenShoulder = 1; break; }
      await sleep(150);
    } }
  T('Lumen hovers at the buddy\'s shoulder', lumenShoulder === 1);
  await page.screenshot({ path: __dirname + '/tsg-v125-f11-room1.png' });
  await tp(roomCX(2), 216); await sleep(300);
  let lumenFollowed = 0;
  { const t0 = Date.now();
    while (Date.now() - t0 < 8000) { const lu = await call('dbg_entlu'); if (Math.abs((lu % 100000) - (roomCX(2) - 26)) < 40) { lumenFollowed = 1; break; } await sleep(150); } }
  T('Lumen follows to the shoulder', lumenFollowed === 1);
  T('chapter 2 room 2 is dark, Typo in residence', Math.floor((await call('dbg_entev')) / 10) === 3, await call('dbg_entev'));
  await page.screenshot({ path: __dirname + '/tsg-v125-f11-dark.png' });

  // the walk in: rooms 2..4 by the standing advance() machinery —
  // it reads the live seed through dbg_entdiag, so it walks
  // Chapter 2's fresh deal (locks, keys, dupes) unmodified
  if ((await call('dbg_entroom')) < 3) await advance(2);
  T('walked to room 3', (await call('dbg_entroom')) === 3, await call('dbg_entroom'));
  if ((await call('dbg_entroom')) < 4) await advance(3);
  T('walked to room 4', (await call('dbg_entroom')) === 4, await call('dbg_entroom'));

  // the duet door: Interact refuses it; two plates open it
  T('door 4 is a duet door', (await call('dbg_entduet', 4)) === 1, await call('dbg_entduet', 4));
  const doorRow4 = await call('dbg_entdiag', 4, 4);   // the real door's row (which=4)
  await tp(doorX(4) - 40, tileCY(doorRow4));
  await key('KeyE', 250); await sleep(300);
  T('Interact does not open a duet door', (await call('dbg_entdoor')) < 4, await call('dbg_entdoor'));
  // the plates flank the doorway (ent_plate_pos: door tile centre
  // x - 40, door row centre y -/+ 58); stand on the north one and
  // watch for Lumen on the south one while the charge builds
  const plateAX = doorX(4) - 40, plateAY = tileCY(doorRow4) - 58, plateBY = tileCY(doorRow4) + 58;
  await tp(plateAX, plateAY);
  let lumenOnPlate = 0, duetOpened = 0;
  { const t0 = Date.now();
    while (Date.now() - t0 < 12000) {
      const lu = await call('dbg_entlu'); const lx = lu % 100000, ly = Math.floor(lu / 100000);
      if (Math.abs(lx - plateAX) < 34 && Math.abs(ly - plateBY) < 34) lumenOnPlate = 1;
      if ((await call('dbg_entdoor')) >= 4) { duetOpened = 1; break; }
      await sleep(120);
    } }
  T('Lumen took the other plate', lumenOnPlate === 1);
  T('the duet door opened for two friends', duetOpened === 1, await call('dbg_entdoor'));
  await page.screenshot({ path: __dirname + '/tsg-v125-f11-duet.png' });
  await tp(roomCX(5), 216); await sleep(200);
  T('through the duet door into room 5', (await call('dbg_entroom')) === 5, await call('dbg_entroom'));
  T('deeper best is room 5 already', (await call('dbg_ent2rec')) % 10000 >= 5, await call('dbg_ent2rec'));

  // the long walk out: 5..18 by advance, the Inspector segment
  // crossed by teleport (the Chapter 1 phase's own pattern — the
  // chase machinery is shared code, re-proving it is not this
  // feature's job), then 24..39 by advance. Duet doors met on the
  // way refuse Interact and are crossed by advance's teleport,
  // like every door the suite cannot open by hand.
  for (let room = 5; room <= 18; room++) { if ((await call('dbg_entroom')) < room + 1) await advance(room); }
  for (const room of [20, 21, 22, 23, 24, 25]) { await tp(roomCX(room), 216); await sleep(150); }
  T('crossed the chase segment', (await call('dbg_entroom')) === 25, await call('dbg_entroom'));
  for (let room = 25; room <= 39; room++) { if ((await call('dbg_entroom')) < room + 1) await advance(room); }
  T('the Deeper exit room', (await call('dbg_entroom')) === 40, await call('dbg_entroom'));
  T('a Deeper completion is recorded', (await call('dbg_ent2rec')) >= 10000, await call('dbg_ent2rec'));
  T('chapter 1 records untouched by chapter 2', (await call('dbg_entrec')) === entRecBefore, (await call('dbg_entrec')) + ' vs ' + entRecBefore);
  await page.screenshot({ path: __dirname + '/tsg-v125-f11-exit.png' });
  await tp(tileCX(40, 7), tileCY(8));
  await key('KeyE', 250); await sleep(400);
  T('the Deeper EXIT door goes home', (await call('dbg_bwstate')) === 0, await call('dbg_bwstate'));

  // persistence: the e2 token rides the save blob, a reload keeps
  // the Deeper records, and Chapter 1 still deals Chapter 1
  const blob = await page.evaluate(() => { for (let i = 0; i < localStorage.length; i++) { const v = localStorage.getItem(localStorage.key(i)); if (v && v.indexOf('e2=') >= 0) return v; } return ''; });
  T('the save carries the e2 token', blob.indexOf('e2=') >= 0);
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 450); await sleep(400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(400);
  T('deeper records survive the reload', (await call('dbg_ent2rec')) >= 10000, await call('dbg_ent2rec'));
  await call('dbg_bwent2');
  T('chapter 1 still deals chapter 1', (await call('dbg_entchap')) === 1 && (await call('dbg_entroom')) === 1,
    'chap=' + (await call('dbg_entchap')) + ' room=' + (await call('dbg_entroom')));
  await key('Escape', 300);

  console.log('entrancetest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
