// toppertest.js — v1.25 HAT TOPPERS (feature #5): cabinet trophies
// worn on the carcass hat. Earned set derives from the same records
// the Trophy Case reads (so the two can never disagree); the BOWL
// PIN is awarded at the bowl finalize site because the lanes keep
// only a house best. Equip is a wardrobe row (row 10) in the Banzai
// Buddies station; the worn id mirrors onto the Player for draw_hat
// and rides the net snapshot.
// Phase B (fresh): the wardrobe row exists, reads NONE, and FIRE
//   with nothing earned changes nothing.
// Phase E (bowl): a gutter game earns nothing; a perfect game (300)
//   through the REAL bowl_record earns the PIN the moment it ends.
// Phase A (crafted): known records derive exactly CROWN / GENERAL
//   CAP / CHECKER FLAG / STORY MEDAL / GOLD STAR / HARD HAT on load
//   (no PIN — house record can't attribute; no ANVIL — no heaviest);
//   dbg_topheavy + one save cycle adds the ANVIL. The wardrobe row
//   cycles NONE through the earned set in id order; the Player
//   mirror tracks it. A2: equip persists (tp=/tw= in the blob,
//   worn restored after reload).
// Phase C (stripped): the same blob minus mc=/mr=/fs= derives only
//   the cabinet four — derivation reads records, not tokens — and
//   the next save writes tp= with exactly that mask.
// Phase D (Zen): a gold-pace Zen race, a Zen perfect game and a
//   200 KG Zen hat earn NOTHING — medal bests, the pin and the
//   anvil are all record-gated, and Zen sets no records.
// Phase F (Eddie): with Eddie on, the worn CROWN still renders on
//   the (now-Eddie) stack — Eddie restyles carcasses, not trophies.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
// topper ids / bits
const CROWN = 1, CAP = 2, FLAG = 4, MEDAL = 8, PIN = 16, GOLD = 32, ANVIL = 64, HARDHAT = 128;

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
  const pollSave = async (n, needle, ms = 20000) => {
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
  // the real player path: hut door in, walk the mall floor to the
  // Banzai Buddies counter (budtest's stand point), INTERACT it open
  async function openBuddies() {
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 300);
    if ((await call('dbg_mall')) !== 1) return false;
    if (!await walkTo(315, -6148, 14)) return false;   // MALL_SX[3]=315, disc row MALL_CY-148 (v1.25 MALL ENLARGEMENT)
    await key('KeyE', 350);
    return (await poll('dbg_station', v => v === 3, 6000)) === 3;
  }
  async function selectRow(n) {
    for (let i = 0; i < 14; i++) { if ((await call('dbg_storesel')) === n) return true; await key('ArrowDown', 130); }
    return (await call('dbg_storesel')) === n;
  }
  async function bowlScripted(knock) {
    await call('dbg_bowl_new');
    for (let i = 0; i < 24; i++) {
      if ((await call('dbg_bowl_over')) === 1) break;
      await call('dbg_bowl_roll', knock);
    }
    return (await call('dbg_bowl_over')) === 1;
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

  // ================= PHASE B: fresh — an empty wardrobe ============
  ok((await call('dbg_state')) === 0, 'B: boots to title');
  ok((await str('dbg_topname', 1)) === 'CROWN', 'B: topper 1 is the CROWN', await str('dbg_topname', 1));
  ok((await str('dbg_topname', 5)) === 'BOWL PIN', 'B: topper 5 is the BOWL PIN', await str('dbg_topname', 5));
  ok((await str('dbg_topname', 8)) === 'HARD HAT', 'B: topper 8 is the HARD HAT', await str('dbg_topname', 8));
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'B: fresh run started');
  await call('dbg_adopt', 0);
  ok(await openBuddies(), 'B: walked the mall and opened BANZAI BUDDIES (station 3)');
  ok((await call('dbg_budrows')) === 11, 'B: buddy menu has 11 rows (10 + HAT TOPPER; the PHOTO BOOTH row left with the booth — station 8 since the MALL ENLARGEMENT)', await call('dbg_budrows'));
  ok((await str('dbg_budrowname', 10)) === 'HAT TOPPER', 'B: row 10 is the HAT TOPPER wardrobe row', await str('dbg_budrowname', 10));
  ok((await str('dbg_budrowright', 10)) === 'NONE', 'B: row 10 right reads NONE', await str('dbg_budrowright', 10));
  ok((await call('dbg_topearned')) === 0, 'B: nothing earned on a fresh buddy');
  ok(await selectRow(10), 'B: selection reaches the wardrobe row');
  await key('Space', 300);
  ok((await call('dbg_topworn')) === 0, 'B: FIRE with nothing earned wears nothing');
  ok((await str('dbg_budrowright', 10)) === 'NONE', 'B: row 10 still reads NONE');
  await page.screenshot({ path: 'tsg-v125-f5-before-panel.png' });
  await key('Escape', 300);

  // ================= PHASE E: the bowl pin, real path =============
  ok(await bowlScripted(0), 'E: gutter game played to the end');
  ok((await call('dbg_bowl_score')) === 0, 'E: gutter game scores 0', await call('dbg_bowl_score'));
  ok((await call('dbg_topearned')) === 0, 'E: a gutter game earns no pin (nothing at all)');
  ok(await bowlScripted(10), 'E: perfect game played to the end');
  ok((await call('dbg_bowl_score')) === 300, 'E: perfect game scores 300', await call('dbg_bowl_score'));
  ok((await call('dbg_topearned')) === PIN, 'E: the 300 game earns exactly the BOWL PIN', await call('dbg_topearned'));

  // ================= PHASE A: crafted records derive ==============
  const blobSrc = await pollSave(1, 'bk=');
  ok(blobSrc.includes('pl= 0 '), 'A: slot 1 save carries the P1 line', blobSrc.slice(0, 60));
  const SQ = 2752682;   // quests 0-5 done + Zed bit 17 (131072), feat 4's fixture value
  const setTok = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);
  const crafted = blobSrc.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    let L = line;
    L = setTok(L, 'sq', SQ); L = setTok(L, 'sq2', 170);
    L = setTok(L, 'er', 20051); L = setTok(L, 'sb', 14);
    L = setTok(L, 'pd', 9); L = setTok(L, 'td', 6);
    L = setTok(L, 'cb', 1250); L = setTok(L, 'cr', 9); L = setTok(L, 'wr', 8);
    L = setTok(L, 'bx', 240); L = setTok(L, 'b1', 3); L = setTok(L, 'b2', 2); L = setTok(L, 'b3', 1);
    L = setTok(L, 'bs', 15); L = setTok(L, 'bn', 26);
    L = L.replace(/ tp=-?[0-9]+/g, '').replace(/ tw=-?[0-9]+/g, '');   // the topper tokens start absent
    return L + ' mc=230,271,0,0,0,0,0,880 mr=261,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0';
  }).join('\n').replace(/^fs=.*$/m, 'fs=2,6,0');
  ok(crafted.includes('cb=1250') && crafted.includes('wr=8') && crafted.includes('fs=2,6,0'), 'A: crafted blob carries the record tokens');
  ok(!crafted.includes(' tp=') && !crafted.includes(' tw='), 'A: crafted blob carries NO topper tokens');
  await page.evaluate(s => localStorage.setItem('tsg_save2', s), crafted);
  ok(await loadSlot(2, 1), 'A: crafted slot 2 loads into play');
  const wantA = CROWN | CAP | FLAG | MEDAL | GOLD | HARDHAT;   // 175
  ok((await poll('dbg_topearned', v => v === wantA, 10000)) === wantA,
     'A: load derives CROWN+CAP+FLAG+MEDAL+GOLD+HARDHAT (no PIN, no ANVIL)', await call('dbg_topearned'));
  await call('dbg_topheavy', 170);
  const wantA2 = wantA | ANVIL;   // 239
  ok((await poll('dbg_topearned', v => v === wantA2, 20000)) === wantA2,
     'A: a 170 KG heaviest + a save cycle adds the ANVIL', await call('dbg_topearned'));
  ok((await pollSave(2, ' tp=' + wantA2)).includes(' tp=' + wantA2), 'A: the derived mask persists as tp=');
  ok(await openBuddies(), 'A: opened BANZAI BUDDIES on the crafted save');
  ok((await str('dbg_budrowright', 10)) === 'NONE', 'A: nothing worn yet', await str('dbg_budrowright', 10));
  ok(await selectRow(10), 'A: selection reaches the wardrobe row');
  const cycle = [1, 2, 3, 4, 6, 7, 8, 0, 1];   // earned ids in order, wrap through NONE, land back on CROWN
  for (let i = 0; i < cycle.length; i++) {
    await key('Space', 280);
    ok((await call('dbg_topworn')) === cycle[i], `A: cycle step ${i + 1} wears ${cycle[i]}`, await call('dbg_topworn'));
    if (cycle[i] === 2) {
      ok((await call('dbg_topwornp')) === 2, 'A: the Player mirror tracks the equip');
      ok((await str('dbg_budrowright', 10)) === 'GENERAL CAP', 'A: row 10 right names the worn topper', await str('dbg_budrowright', 10));
      await page.screenshot({ path: 'tsg-v125-f5-after-panel.png' });
    }
  }
  ok((await call('dbg_topwornp')) === 1, 'A: mirror back on the CROWN at cycle end');
  await key('Escape', 300);

  // ---- A2: the equip persists across save/load --------------------
  ok((await pollSave(2, ' tw=1')).includes(' tw=1'), 'A2: the blob carries tw=1 (CROWN worn)');
  ok(await loadSlot(2, 2), 'A2: slot 2 reloads');
  ok((await call('dbg_topearned')) === wantA2, 'A2: earned set intact after reload', await call('dbg_topearned'));
  ok((await call('dbg_topworn')) === 1, 'A2: CROWN still worn after reload');
  ok((await call('dbg_topwornp')) === 1, 'A2: mirror restored after reload');

  // ---- the worn crown, on a bare head and on a real stack ---------
  await call('dbg_tp', 0, 80); await sleep(250);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'A: back in the mall to leave by the mat');
  ok(await walkTo(0, -5810, 10), 'A: walked to the exit mat');
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 0, 'A: left the mall');
  await call('dbg_tp', 0, 300); await sleep(300);
  await page.screenshot({ path: 'tsg-v125-f5-after-crown-bare.png' });
  for (let i = 0; i < 8; i++) await call('dbg_feed');   // 40 carcasses, 80 KG — a real stack via hat_push
  await sleep(400);
  await page.screenshot({ path: 'tsg-v125-f5-after-crown-stack.png' });

  // ================= PHASE C: a pre-feature save ===================
  const stripped = crafted.replace(/ mc=[0-9,]+/g, '').replace(/ mr=[0-9,]+/g, '').replace(/^fs=.*\n/m, '');
  ok(!stripped.includes('mc=') && !stripped.includes('fs=') && !stripped.includes(' tp='), 'C: stripped blob is v1.25-free');
  await page.evaluate(s => localStorage.setItem('tsg_save3', s), stripped);
  ok(await loadSlot(3, 2), 'C: pre-feature slot 3 loads into play');
  ok((await call('dbg_casbest')) === 1250, 'C: castle record intact', await call('dbg_casbest'));
  const wantC = CROWN | CAP | FLAG | MEDAL;   // 15
  ok((await poll('dbg_topearned', v => v === wantC, 10000)) === wantC,
     'C: only the cabinet four derive (no medals token, no First Shift line)', await call('dbg_topearned'));
  ok((await pollSave(3, ' tp=' + wantC)).includes(' tp=' + wantC), 'C: the next save writes tp= from derivation alone');

  // ================= PHASE D: Zen earns nothing ====================
  await page.evaluate(() => localStorage.removeItem('tsg_save'));
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  ok((await call('dbg_mzen')) === 0, 'D: Zen preference starts off');
  // ZEN is the second-to-last title row in BOTH menu flavours
  // (keyboard 7 rows, pad 6 — the flavour latch is racy at boot,
  // so navigate by the live count: the cartcoop lesson)
  ok((await call('dbg_menucount')) >= 6, 'D: title rows present', await call('dbg_menucount'));
  let onZen = false;
  for (let i = 0; i < 10; i++) {
    const c = await call('dbg_menucount'), s = await call('dbg_menusel');
    if (s === c - 2) { onZen = true; break; }
    await key('ArrowDown', 180);
  }
  ok(onZen, 'D: selection on the ZEN row');
  await key('Space', 350);
  ok((await call('dbg_mzen')) === 1, 'D: Zen toggled on from the title', await call('dbg_mzen'));
  let onStart = false;
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 1) { onStart = true; break; } await key('ArrowUp', 150); }
  ok(onStart, 'D: selection on START');
  await key('Space', 400);
  ok((await call('dbg_state')) === 7, 'D: slot picker open', await call('dbg_state'));
  await key('Enter', 350);
  ok((await call('dbg_state')) === 7, 'D: overwrite confirm armed', await call('dbg_state'));
  await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'D: fresh Zen run started in slot 3');
  await call('dbg_adopt', 0);
  ok((await call('dbg_zen')) === 1, 'D: Zen run is live', await call('dbg_zen'));
  ok(await startRace(), 'D: Zen race running');
  ok(await finishRaceAt(200), 'D: Zen finish at gold pace (banked nowhere)');
  await call('dbg_bwexit');
  await sleep(400);
  ok(await bowlScripted(10), 'D: Zen perfect game played');
  ok((await call('dbg_bowl_score')) === 300, 'D: Zen perfect game scores 300', await call('dbg_bowl_score'));
  await call('dbg_topheavy', 200);
  await sleep(7000);   // let the autosave-tick refresh run on all of it
  ok((await call('dbg_topearned')) === 0, 'D: Zen earns NO topper — gold, pin and anvil are all record-gated', await call('dbg_topearned'));

  // ================= PHASE F: Eddie keeps the trophy ===============
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  ok((await call('dbg_meddie')) === 0, 'F: Eddie starts off');
  // EDDIE is the LAST title row in both menu flavours — navigate
  // by the live count, never a fixed row (the flavour latch race)
  let onEddie = false;
  for (let i = 0; i < 10; i++) {
    const c = await call('dbg_menucount'), s = await call('dbg_menusel');
    if (c >= 6 && s === c - 1) { onEddie = true; break; }
    await key('ArrowDown', 160);
  }
  ok(onEddie, 'F: selection on the EDDIE row (last row, live count)');
  await key('Space', 350);
  ok((await call('dbg_meddie')) === 1, 'F: Eddie toggled on from the title', await call('dbg_meddie'));
  // Continue slot 2. The picker's preselect follows the global
  // blob's slot — which phase D deleted and recreated — so probe
  // the rows empirically: slot 2 is the save whose castle record
  // is 1250. Eddie persists in the global blob across reloads;
  // poll for it (boot applies the save over the first frames).
  let continued = false;
  for (let downs = 0; downs < 4 && !continued; downs++) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    if ((await poll('dbg_meddie', v => v === 1, 4000)) !== 1) break;
    await key('Space', 400);   // CONTINUE -> slot picker
    if ((await poll('dbg_state', v => v === 7, 4000)) !== 7) continue;
    for (let i = 0; i < downs; i++) await key('ArrowDown', 200);
    await key('Enter', 400);
    if ((await call('dbg_state')) === 7) await key('Enter', 400);   // overwrite-confirm arm (empty row)
    if ((await poll('dbg_state', v => v === 1, 5000)) !== 1) continue;
    continued = (await call('dbg_casbest')) === 1250;
  }
  ok(continued, 'F: slot 2 continued under Eddie');
  ok((await call('dbg_topwornp')) === 1, 'F: the CROWN is still worn', await call('dbg_topwornp'));
  await call('dbg_tp', 0, 300); await sleep(250);
  for (let i = 0; i < 8; i++) await call('dbg_feed');   // the stack, now all Eddies
  await call('dbg_spawnbug', 45, 335);
  await sleep(1100);
  await page.screenshot({ path: 'tsg-v125-f5-after-eddie.png' });

  console.log(`\nTOPPERTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
