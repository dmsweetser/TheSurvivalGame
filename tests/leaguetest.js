// leaguetest.js — v1.25 BUG BOWL LEAGUE (feature #10): a six-night
// season at the lanes against three house rivals (KINGPIN, SPARE
// CHANGE, GUTTER BUG) whose scores are a pure function of (season,
// night, rival) via the game's unsigned hash2 — posted on the card
// BEFORE you bowl, standings by cumulative pins, ties to the
// player. Every completed non-Zen game is the next league night;
// the book (career + season) is per-player, kept at the bowl_record
// finalize, and rides one pl-line tail token (bl=).
// Scoring drives the REAL finalize: exact-score games go through
// dbg_bowl_roll -> bowl_record (the feat-5 disclosed seam — the
// game's own record function), and one full game is bowled with
// real throws at the lanes to prove the hook sits on the player's
// path too.
// Phase 1 (fresh): clean book; the lanes, walked to on the real
//   Mall path, show SEASON 1 with tonight's card equal to the JS
//   replica for all three rivals (screenshot).
// Phase 2 (a night, exactly): all-spares 150 — career, night,
//   spins, last_place all read back against the replica; the blob
//   gains bl=1,150,150,0,1,1,150,...; standings screenshot.
// Phase 3 (strikes): a perfect game — 12 strikes counted, best 300,
//   WON THE NIGHT outright (300 beats every possible card).
// Phase 4 (the real path + save/load mid-season): adopt a buddy,
//   bank 50 kg, bowl a FULL game with real throws at the lanes
//   (night 3, relational assertions — a thrown game has no fixed
//   score), then reload the slot with hat ON and money banked:
//   every league field, the hat and the bank read back identical.
// Phase 5 (a full season): nights 4-6 at 300 apiece crown the
//   player (worst rival season total is 188*6=1128 < 1350+):
//   trophies 1, fin_place 1, buddy XP exactly +50, season 2 opens
//   clean, and a gutter game starts it at last_place replica(0).
// Phase 6 (pre-feature): the live blob minus bl= loads with
//   records/bank/buddy intact and an empty book; a save cycle
//   writes NO bl= until a game is bowled, then bl= appears.
// Phase 7 (Zen): a crafted Zen run bowls a scripted game and the
//   book does not move; the lanes card reads PRACTICE (screenshot).
// Mall coordinates below are the game's own named constants
// (world.c: MALL_CY, BOWL_SX and the counters' stand points), kept
// as constants here so a Mall re-spacing pass has one place to
// touch. All state sampled right after boot/reload is polled (the
// Nov 19 rule: wasm exports answer before the first frame).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

// ---- the game's own geography (world.c named constants) --------
const MALL_CY = -6000;                 // the Mall interior's centre line
const BOWL_SX = -180;                  // BOWL_SX: the lanes counter, south-west (v1.25 MALL ENLARGEMENT)
const BOWL_STAND = [BOWL_SX, MALL_CY + 132];   // the bowl counter's interact disc (south disc row MALL_CY+132)
const BANK_STAND = [105, MALL_CY - 148 + 0];   // banktest's stand point (105, -6148) — v1.25 MALL ENLARGEMENT: MALL_SX[2]=105, disc row MALL_CY-148
const BUD_STAND = [315, MALL_CY - 148];       // budtest's stand point (315, -6148) — MALL_SX[3]=315
const EXIT_MAT = [0, MALL_CY + 190];          // the exit mat (0, -5810) - mat zone MALL_CY+168..+212 since the ENLARGEMENT

// ---- an independent replica of the league derivation ----------
// lg_rival_score (mall.c) = clamp(mean[r] + hash2(season*7+night,
// r*7919+13, 0xB0B1E5) % 61 - 30, 0, 300); hash2 is the game's
// unsigned helper (prelude.c). If the tables or seeds drift, this
// says so.
function hash2(x, y, seed) {
  let h = (seed + Math.imul(x, 374761393) + Math.imul(y, 668265263)) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177) >>> 0;
  return (h ^ (h >>> 16)) >>> 0;
}
const LG_MEANS = [158, 128, 96];
const LG_NAMES = ['KINGPIN', 'SPARE CHANGE', 'GUTTER BUG'];
const lgScore = (s, n, r) => Math.min(300, Math.max(0, LG_MEANS[r] + (hash2(s * 7 + n, r * 7919 + 13, 0xB0B1E5) % 61) - 30));
const lgTotal = (s, r, nights) => { let t = 0; for (let n = 0; n < nights; n++) t += lgScore(s, n, r); return t; };
const lgPlace = (score, s, n) => 1 + [0, 1, 2].filter(r => lgScore(s, n, r) > score).length;
// dbg_lg field ids (platform.c)
const LG = { games: 0, best: 1, pins: 2, strikes: 3, season: 4, night: 5, spins: 6, trophies: 7, lastScore: 8, lastPlace: 9, finPlace: 10, lastNight: 11, finSeason: 12 };

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
  const poll = async (fn, pred, ms = 15000, ...args) => {
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn, ...args); if (pred(v)) return v; await sleep(200); }
    return v;
  };
  const saveSlot = n => page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n);
  const pollSave = async (n, needle, ms = 20000) => {
    const t0 = Date.now(); let s = '';
    while (Date.now() - t0 < ms) { s = (await saveSlot(n)) || ''; if (s.includes(needle)) return s; await sleep(300); }
    return s;
  };
  const lg = f => call('dbg_lg', f);
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
  async function enterMall() {
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 300);
    return (await poll('dbg_mall', v => v === 1, 6000)) === 1;
  }
  async function exitMall() {
    await call('dbg_tp', EXIT_MAT[0], EXIT_MAT[1]); await sleep(250);
    await key('KeyE', 300);
    return (await poll('dbg_mall', v => v === 0, 6000)) === 0;
  }
  async function openLanes() {
    if (!await walkTo(BOWL_STAND[0], BOWL_STAND[1], 14)) return false;
    await key('KeyE', 350);
    return (await poll('dbg_station', v => v === 4, 6000)) === 4;
  }
  const roll = k => call('dbg_bowl_roll', k);
  async function game(rolls) { await call('dbg_bowl_new'); for (const r of rolls) await roll(r); }
  const setTok = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);

  // ====== boot + fresh run ======================================
  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'run started');

  // ====== PHASE 1: a fresh book, and the card is the replica ====
  ok((await lg(LG.games)) === 0 && (await lg(LG.season)) === 0 && (await lg(LG.night)) === 0,
     '1: fresh book — no games, no season yet');
  for (let r = 0; r < 3; r++) {
    ok((await str('dbg_lgname', r)) === LG_NAMES[r], `1: rival ${r} is ${LG_NAMES[r]}`, await str('dbg_lgname', r));
    ok((await call('dbg_lgrival', 1, 0, r)) === lgScore(1, 0, r),
       `1: tonight's card for ${LG_NAMES[r]} matches the replica`, `${await call('dbg_lgrival', 1, 0, r)} vs ${lgScore(1, 0, r)}`);
    ok((await call('dbg_lgtotal', 1, r, 6)) === lgTotal(1, r, 6), `1: ${LG_NAMES[r]}'s season total matches the replica`);
  }
  ok(await enterMall(), '1: stepped into the mall');
  ok(await openLanes(), '1: the bowling view opens (station 4)');
  await sleep(400);
  await page.screenshot({ path: __dirname + '/tsg-v125-f10-lanes-fresh.png' });
  await key('Escape', 300);
  ok((await call('dbg_state')) === 1, '1: bowling view closes');
  ok(await exitMall(), '1: left the mall');

  // ====== PHASE 2: night 1, exactly (all spares = 150) ==========
  await game(Array(21).fill(5));
  ok((await call('dbg_bowl_over')) === 1 && (await call('dbg_bowl_score')) === 150, '2: the scripted game is 150');
  ok((await lg(LG.games)) === 1, '2: career games = 1');
  ok((await lg(LG.best)) === 150 && (await lg(LG.pins)) === 150, '2: career best/pins = 150');
  ok((await lg(LG.strikes)) === 0, '2: an all-spares game has no strikes');
  ok((await lg(LG.season)) === 1 && (await lg(LG.night)) === 1 && (await lg(LG.spins)) === 150,
     '2: season 1, night 1 in the books, 150 spins');
  ok((await lg(LG.lastScore)) === 150 && (await lg(LG.lastNight)) === 1, '2: last night recorded');
  ok((await lg(LG.lastPlace)) === lgPlace(150, 1, 0),
     '2: last_place is the replica placing of 150 against the posted card',
     `${await lg(LG.lastPlace)} vs ${lgPlace(150, 1, 0)}`);
  const blob2 = await pollSave(1, 'bl=1,150,150,0,1,1,150');
  ok(blob2.includes('bl=1,150,150,0,1,1,150'), '2: the blob carries the league book', (blob2.match(/ bl=[0-9,]+/) || [''])[0]);
  // the standings after a night, on the lanes' own wall
  ok(await enterMall(), '2: back into the mall');
  ok(await openLanes(), '2: lanes open over the updated table');
  await sleep(400);
  await page.screenshot({ path: __dirname + '/tsg-v125-f10-standings.png' });
  await key('Escape', 300);
  ok(await exitMall(), '2: left the mall');

  // ====== PHASE 3: a perfect game — strikes, and the night ======
  await game(Array(12).fill(10));
  ok((await call('dbg_bowl_score')) === 300, '3: the scripted game is 300');
  ok((await lg(LG.strikes)) === 12, '3: twelve strikes counted', await lg(LG.strikes));
  ok((await lg(LG.best)) === 300 && (await lg(LG.games)) === 2 && (await lg(LG.pins)) === 450,
     '3: career reads 2 games, best 300, 450 pins');
  ok((await lg(LG.lastPlace)) === 1, '3: 300 WON THE NIGHT outright — no card beats it');
  ok((await lg(LG.night)) === 2 && (await lg(LG.spins)) === 450, '3: night 2 in the books at 450');
  // the game-over verdict, on the lanes' own screen
  ok(await enterMall(), '3: back into the mall');
  ok(await openLanes(), '3: lanes open over the finished game');
  await sleep(400);
  await page.screenshot({ path: __dirname + '/tsg-v125-f10-gameover.png' });
  await key('Escape', 300);
  ok(await exitMall(), '3: left the mall');

  // ====== PHASE 4: buddy, bank, a REAL thrown game, save/load ===
  ok(await enterMall(), '4: into the mall');
  // adopt the pet rock (budtest's proven path: 6 feeds = 60 kg, row 4)
  ok(await walkTo(BUD_STAND[0], BUD_STAND[1], 14), '4: walked to the buddies counter');
  for (let i = 0; i < 6; i++) await call('dbg_feed');
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, '4: buddies panel open');
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350);
  await key('Space', 350);   // name row 0
  await key('Space', 400);   // coat row 0
  ok((await poll('dbg_buddy', v => v !== 0, 20000)) !== 0, '4: a buddy is adopted');
  await key('Escape', 300);
  // bank 50 kg (banktest's path: 5 feeds, DEPOSIT ALL)
  ok(await walkTo(BANK_STAND[0], BANK_STAND[1], 14), '4: walked to the bank counter');
  for (let i = 0; i < 5; i++) await call('dbg_feed');
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 2, '4: bank panel open');
  await key('Space', 400);
  await key('Escape', 300);
  ok((await poll('dbg_bank', v => v === 50, 8000)) === 50, '4: 50 kg banked', await call('dbg_bank'));
  // a full game with REAL throws at the lanes (night 3)
  for (let i = 0; i < 6; i++) await call('dbg_feed');   // 30 carcasses: a full game needs at most 21
  ok(await openLanes(), '4: lanes open for the real game');
  const gamesBefore = await lg(LG.games), pinsBefore = await lg(LG.pins);
  // the lanes still hold phase 3's finished game — start a FRESH
  // one with a real FIRE press (game_over + act = new game on the
  // player's own path) and wait until it is genuinely underway
  await key('Space', 300);
  ok((await poll('dbg_bowl_over', v => v === 0, 8000)) === 0, '4: a fresh real game is underway');
  { let guard = 0;
    while ((await call('dbg_bowl_over')) !== 1 && guard++ < 200) {
      const ph = await call('dbg_bowl_ph', 0);
      // ph -1: attract — FIRE starts a game; ph 5 with the old game
      // over: FIRE starts the next; ph 0/1/2: position, aim, throw
      if (ph === -1 || ph === 5 || (ph >= 0 && ph <= 2)) await key('Space', 200);
      else await sleep(350);
    } }
  ok((await call('dbg_bowl_over')) === 1, '4: the real thrown game completed');
  const realFinal = await call('dbg_bowl_score');
  ok((await lg(LG.games)) === gamesBefore + 1, '4: the real game counted as a league game');
  ok((await lg(LG.night)) === 3, '4: night 3 in the books');
  ok((await lg(LG.lastScore)) === realFinal, '4: last_score is the thrown game\'s final', `${await lg(LG.lastScore)} vs ${realFinal}`);
  ok((await lg(LG.pins)) === pinsBefore + realFinal, '4: career pins grew by exactly the thrown final');
  await key('Escape', 300);
  ok(await exitMall(), '4: left the mall');
  // save/load mid-season, realistic state: hat ON, money banked
  const snap = {};
  for (const [k, f] of Object.entries(LG)) snap[k] = await lg(f);
  const carriedSnap = await call('dbg_carried');
  ok(carriedSnap > 0, '4: the hat is ON for the save', carriedSnap);
  const blob4 = await pollSave(1, `bl=${snap.games},`);
  ok(blob4.includes(`bl=${snap.games},${snap.best},${snap.pins},${snap.strikes},1,3,${snap.spins}`),
     '4: the mid-season blob carries the whole book', (blob4.match(/ bl=[0-9,]+/) || [''])[0]);
  ok(await loadSlot(1), '4: slot 1 reloads mid-season');
  for (const [k, f] of Object.entries(LG))
    ok((await poll('dbg_lg', v => v === snap[k], 8000, f)) === snap[k], `4: ${k} survives the reload`, `${await lg(f)} vs ${snap[k]}`);
  ok((await call('dbg_bank')) === 50, '4: the bank survived');
  ok((await call('dbg_carried')) === carriedSnap, '4: the hat survived', `${await call('dbg_carried')} vs ${carriedSnap}`);

  // ====== PHASE 5: the season closes — a champion ===============
  const xp0 = await call('dbg_buddyxp');
  await game(Array(12).fill(10));   // night 4
  await game(Array(12).fill(10));   // night 5
  ok((await lg(LG.night)) === 5 && (await lg(LG.season)) === 1, '5: five nights in, season 1 still open');
  await game(Array(12).fill(10));   // night 6 closes it: 150+300+R+900 >= 1350 > 1128 (the best possible rival total)
  ok((await lg(LG.trophies)) === 1, '5: the championship is the player\'s');
  ok((await lg(LG.finPlace)) === 1 && (await lg(LG.finSeason)) === 1, '5: season 1 closes CHAMPION');
  ok((await lg(LG.season)) === 2 && (await lg(LG.night)) === 0 && (await lg(LG.spins)) === 0,
     '5: season 2 opens clean');
  ok((await call('dbg_buddyxp')) === xp0 + 50, '5: the crown pays exactly +50 buddy XP, once',
     `${await call('dbg_buddyxp')} vs ${xp0}+50`);
  ok((await lg(LG.games)) === 6, '5: career games = 6');
  await game(Array(20).fill(0));   // season 2, night 1: a gutter game
  ok((await lg(LG.season)) === 2 && (await lg(LG.night)) === 1 && (await lg(LG.spins)) === 0,
     '5: season 2 night 1 is the gutter game');
  ok((await lg(LG.lastPlace)) === lgPlace(0, 2, 0), '5: the gutter game places by the replica',
     `${await lg(LG.lastPlace)} vs ${lgPlace(0, 2, 0)}`);
  ok((await call('dbg_buddyxp')) === xp0 + 50, '5: no second crown payment');

  // ====== PHASE 6: a pre-feature save joins cleanly ==============
  const blobNow = await pollSave(1, 'bl=7,');
  const stripped = blobNow.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    return setTok(line, 'bx', 4242).replace(/ bl=[0-9,]+/, '');
  }).join('\n');
  ok(!/ bl=/.test(stripped) && stripped.includes(' bx=4242'), '6: stripped blob has no bl= and a marker XP');
  await writeAtTitle('tsg_save2', stripped);
  ok(await loadSlot(2), '6: pre-feature slot 2 loads');
  ok((await call('dbg_buddyxp')) === 4242, '6: this IS slot 2 (marker XP)');
  ok((await call('dbg_bank')) === 50, '6: the bank is intact from the old save');
  ok((await poll('dbg_lg', v => v === 0, 8000, LG.games)) === 0, '6: the book starts empty');
  await sleep(6500);   // an autosave cycle: nothing bowled, nothing to write
  const blob6 = (await saveSlot(2)) || '';
  ok(!/ bl=/.test(blob6), '6: a league-less save cycle writes NO bl= (byte-identical rule)');
  await game(Array(21).fill(5));
  ok((await lg(LG.games)) === 1 && (await lg(LG.season)) === 1 && (await lg(LG.night)) === 1 && (await lg(LG.spins)) === 150,
     '6: the old save\'s first game opens season 1');
  const blob6b = await pollSave(2, 'bl=1,150,150,0,1,1,150');
  ok(blob6b.includes('bl=1,150,150,0,1,1,150'), '6: bl= appears once the book has a game');

  // ====== PHASE 7: Zen bowls practice ============================
  const zenCraft = blobNow.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    return setTok(line, 'bx', 5151);
  }).join('\n').replace(/^zrun=.*$/m, 'zrun=1');
  ok(zenCraft.includes('zrun=1') && zenCraft.includes(' bx=5151') && zenCraft.includes(' bl=7,'), '7: Zen blob crafted (book intact)');
  await writeAtTitle('tsg_save3', zenCraft);
  ok(await loadSlot(3), '7: Zen slot 3 loads');
  ok((await call('dbg_zen')) === 1, '7: the run is Zen');
  ok((await call('dbg_buddyxp')) === 5151, '7: this IS slot 3 (marker XP)');
  ok((await lg(LG.games)) === 7 && (await lg(LG.night)) === 1, '7: the book carried into the Zen run');
  ok(await enterMall(), '7: into the mall');
  ok(await openLanes(), '7: lanes open in Zen');
  await sleep(400);
  await page.screenshot({ path: __dirname + '/tsg-v125-f10-zen.png' });
  await key('Escape', 300);
  ok(await exitMall(), '7: left the mall');
  await game(Array(21).fill(5));
  ok((await call('dbg_bowl_score')) === 150, '7: the Zen game still scores 150');
  ok((await lg(LG.games)) === 7 && (await lg(LG.night)) === 1 && (await lg(LG.spins)) === 0,
     '7: Zen moves nothing — no game, no night, no pins');

  console.log(`\nLEAGUETEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('LEAGUETEST CRASH', e); process.exit(2); });
