// portraittest.js — v1.25 feat 9: the HAT PORTRAIT BOOTH. Since the
// MALL ENLARGEMENT it is its own station (index 8): a freestanding
// island fixture west-centre in the Mall, with its own interact disc
// at (BOOTH_SX -232, MALL_CY+20 = -5980). The Banzai Buddies menu is
// back to 11 rows. dbg_portstage keeps feat 9's numbering (5 = the
// booth's rows open, 6 = the gallery) over the new booth_stage state.
// In the booth: pick one of six backdrops, pose the hat as worn or
// bare, SNAP a portrait — a FROZEN pose (backdrop, stack kinds, kg,
// topper, buddy look) that hangs in the gallery even after the hat
// is banked, survives save/load via one pt= tail token, and hands
// the shell a frame rectangle so the portrait also lands as a PNG
// through the share card's path.
// Entry, posing and snapping all happen on the REAL path (walk in,
// real keys); debug getters only observe. Pre-feature saves load
// with no portrait and write no pt= until a snap happens. Zen and
// Eddie both pose. Crafted blobs are written AT THE TITLE (the
// shuffletest rule: never rewrite a save under a live instance).
const { chromium } = require('playwright-core');
const fs = require('fs');
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
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 }, acceptDownloads: true });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const str = async (fn, ...args) => page.evaluate(([f, a]) => {
    const p = window.__TSG[f](...a); if (!p) return '';
    const b = new Uint8Array(window.__TSG.memory.buffer, p, 256); let s = '';
    for (let i = 0; i < b.length && b[i]; i++) s += String.fromCharCode(b[i]);
    return s;
  }, [fn, args]);
  const key = async (k, ms = 130) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const hold = async (k, ms) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(120); };
  async function poll(fn, want, timeout = 6000, ...args) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { if (await fn(...args) === want) return true; await sleep(150); }
    return false;
  }
  async function saveSlot(n) { return page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n); }
  async function pollSave(n, sub, timeout = 25000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const b = await saveSlot(n); if (b && b.includes(sub)) return b; await sleep(400); }
    return await saveSlot(n);
  }
  async function loadSlot(n, lastPlayed, expect) {
    // the real path: reload to the title, CONTINUE opens the picker
    // on the last-played slot, walk down to n, Enter loads it — and
    // VERIFY we landed in n: a single picker Down can fall between
    // frames under load (run 2 landed in slot 1 and limped on
    // through three phases before the log gave it away). Retries
    // keep the same lastPlayed — a lost Down leaves you in the
    // slot the picker started on. expect() must distinguish n
    // from every other slot.
    for (let attempt = 0; attempt < 3; attempt++) {
      await page.reload({ waitUntil: 'load' });
      await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
      for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(300); const b2 = await call('dbg_menucount'); if (a === b2 && a >= 6) break; }
      await key('Space', 400);
      if (!(await poll(call, 7, 4000, 'dbg_state'))) continue;
      const downs = ((n - lastPlayed) % 4 + 4) % 4;
      for (let i = 0; i < downs; i++) { await hold('ArrowDown', 90); await sleep(120); }
      await key('Enter', 500);
      if (await poll(call, 1, 8000, 'dbg_state')) {
        const t0 = Date.now();
        while (Date.now() - t0 < 6000) { if (await expect()) return true; await sleep(250); }
      }
      console.log('loadSlot retry: attempt', attempt, 'did not land in slot', n);
    }
    return false;
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
      await sleep(110);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }
  // the real path into the Banzai Buddies panel (toppertest's route);
  // MALL_SX[3]=315, disc row MALL_CY-148 (v1.25 MALL ENLARGEMENT)
  async function openBuddies() {
    await call('dbg_tp', 0, 80);
    await key('KeyE', 300);
    await walkTo(315, -6148, 14);
    await key('KeyE', 400);
    return poll(call, 3, 6000, 'dbg_station');
  }
  // the real path into the PHOTO BOOTH, station 8: its island disc
  // sits at (BOOTH_SX -232, BOOTH_DY MALL_CY+20 = -5980)
  async function openBooth() {
    await call('dbg_tp', 0, 80);
    await key('KeyE', 300);
    await walkTo(-232, -5980, 14);
    await key('KeyE', 400);
    return poll(call, 8, 6000, 'dbg_station');
  }
  // write a crafted blob the safe way (shuffletest's loadCrafted
  // pattern): reload to the title FIRST, write there, never under a
  // live game instance — then loadSlot's own reload picks it up.
  async function writeAtTitle(key2, blobText) {
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await page.evaluate(([k, b]) => localStorage.setItem(k, b), [key2, blobText]);
  }
  async function selectRow(row) {
    for (let i = 0; i < 30; i++) { if (await call('dbg_storesel') === row) return true; await key('ArrowDown', 90); }
    return false;
  }
  function setTok(blob, tok, val) {
    const re = new RegExp(' ' + tok + '=[0-9.]+');
    return re.test(blob) ? blob.replace(re, ' ' + tok + '=' + val) : blob + ' ' + tok + '=' + val;
  }

  // ============ Phase B: fresh run — the booth on the real path ====
  await key('Enter', 700);                      // START -> slot picker
  ok(await poll(call, 7, 6000, 'dbg_state'), 'B: slot picker opens');
  await key('Enter', 500); await key('Enter', 900);   // pick + confirm -> run
  ok(await poll(call, 1, 8000, 'dbg_state'), 'B: fresh run started');
  await call('dbg_adopt', 0);
  ok(await openBuddies(), 'B: Banzai Buddies panel opens on the real path');
  ok(await call('dbg_budrows') === 11, 'B: buddy menu is 11 rows (the booth moved out — station 8)', await call('dbg_budrows'));
  ok(await str('dbg_budrowname', 10) === 'HAT TOPPER', 'B: row 10 is HAT TOPPER, the last row', await str('dbg_budrowname', 10));
  await key('Escape', 400);
  ok(await poll(call, 1, 4000, 'dbg_state'), 'B: buddies panel closed');
  ok(await openBooth(), 'B: the PHOTO BOOTH fixture opens its own panel (station 8)');
  ok(await poll(call, 5, 4000, 'dbg_portstage'), 'B: the booth stage opens', await call('dbg_portstage'));
  const brow = async i => str('dbg_budrowname', i), bright = async i => str('dbg_budrowright', i);
  ok(await brow(0) === 'BACKDROP' && await brow(1) === 'THE HAT' && await brow(2) === 'SNAP PORTRAIT'
     && await brow(3) === 'YOUR PORTRAIT' && await brow(4) === 'STEP OUT',
     'B: booth rows are backdrop / hat / snap / portrait / step out');
  ok(await bright(0) === 'PLAZA DAWN' && await bright(1) === 'AS WORN' && await bright(2) === 'FREE' && await bright(3) === 'NONE YET',
     'B: booth rights: PLAZA DAWN / AS WORN / FREE / NONE YET',
     [await bright(0), await bright(1), await bright(2), await bright(3)].join('/'));
  // YOUR PORTRAIT with nothing snapped: a note, and the stage stays
  ok(await selectRow(3), 'B: YOUR PORTRAIT row selected');
  await key('Space', 400);
  ok(await call('dbg_portstage') === 5 && await call('dbg_porthas') === 0, 'B: no portrait yet — the gallery refuses politely');
  await page.screenshot({ path: 'tsg-v125-f9-booth-empty.png' });
  ok(await selectRow(4), 'B: step-out row selected');
  await key('Space', 400);
  ok(await poll(call, 0, 4000, 'dbg_portstage'), 'B: STEP OUT leaves the booth stage');
  ok(await poll(call, 1, 4000, 'dbg_state'), 'B: STEP OUT closed the panel (state back to play; dbg_station is stale by design)');

  // ============ Phase A: a real stack + CROWN — pose and SNAP ======
  // the crafted record set (toppertest's): Castle escaped, both cups,
  // all WAR! regions, Zed down, 200+ bowled, First Shift done — the
  // CROWN (topper 1) derives from it. bk= is DOUBLE kg: 200 = 100 in.
  let blob = await pollSave(1, 'bk=');
  ok(blob && blob.includes('pl= 0 '), 'A: slot 1 save carries the P1 line', blob && blob.slice(0, 60));
  // the crafted record set (toppertest's fixture, edited onto the
  // pl-line ONLY — the loader parses the blob line by line, and a
  // blob flattened to one line reads as an EMPTY slot): Castle
  // escaped (the CROWN derives), both cups, all WAR! regions, Zed
  // down, First Shift done. bk= is DOUBLE kg: 200 = 100 banked.
  const SQ = 2752682;   // quests 0-5 done + Zed bit 17
  const setTokL = (line, tok, val) => line.replace(new RegExp(' ' + tok + '=-?[0-9,]+'), ' ' + tok + '=' + val);
  blob = blob.split('\n').map(line => {
    if (!line.startsWith('pl= 0 ')) return line;
    let L = line;
    L = setTokL(L, 'sq', SQ); L = setTokL(L, 'sq2', 170);
    L = setTokL(L, 'er', 20051); L = setTokL(L, 'sb', 14);
    L = setTokL(L, 'pd', 9); L = setTokL(L, 'td', 6);
    L = setTokL(L, 'cb', 1250); L = setTokL(L, 'cr', 9); L = setTokL(L, 'wr', 8);
    L = setTokL(L, 'bx', 240); L = setTokL(L, 'bk', 200); L = setTokL(L, 'bn', 26);
    L = L.replace(/ tp=-?[0-9]+/g, '').replace(/ tw=-?[0-9]+/g, '').replace(/ pt=[0-9,:]+/g, '');
    return L + ' mc=92,0,0,0,0,0,0,0 mr=201,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0';
  }).join('\n').replace(/^fs=.*$/m, 'fs=2,6,0');
  ok(blob.includes(' cb=1250') && blob.includes(' bk=200') && !blob.includes(' pt='),
     'A: crafted blob carries cb=1250 / bk=200 and no portrait yet');
  await writeAtTitle('tsg_save2', blob);   // hardened: written at the title, not under slot 1's live run
  ok(await loadSlot(2, 1, async () => (await call('dbg_casbest')) === 1250), 'A: crafted slot 2 loads (Castle best 1250)');
  ok(await call('dbg_bank') === 100, 'A: 100 kg banked from the crafted bk=', await call('dbg_bank'));
  ok(await call('dbg_porthas') === 0, 'A: pre-feature slot has no portrait');
  // wear the CROWN through the real wardrobe row
  ok(await openBuddies(), 'A: buddies panel opens');
  ok(await selectRow(10), 'A: wardrobe row selected');
  for (let i = 0; i < 14 && await call('dbg_topworn') !== 1; i++) await key('Space', 220);
  ok(await call('dbg_topworn') === 1, 'A: the CROWN is worn', await call('dbg_topworn'));
  await key('Escape', 400);
  // a real stack: 8 feeds = 40 carcasses = 80 kg, all kind 2
  for (let i = 0; i < 8; i++) await call('dbg_feed');
  ok(await call('dbg_carried') === 800, 'A: carrying 80 kg of hat (dbg_carried is deci-kg)', await call('dbg_carried'));
  ok(await openBooth(), 'A: the booth fixture opens with the hat on');
  ok(await poll(call, 5, 4000, 'dbg_portstage'), 'A: booth stage opens');
  // pose: one backdrop cycle, hat off and back on
  ok(await selectRow(0), 'A: backdrop row selected');
  await key('Space', 300);
  ok(await call('dbg_portbd') === 1 && await bright(0) === 'FROZEN CACHE', 'A: backdrop cycles to FROZEN CACHE', await bright(0));
  ok(await selectRow(1), 'A: hat row selected');
  await key('Space', 300);
  ok(await call('dbg_porthat') === 0 && await bright(1) === 'BARE', 'A: hat dial goes BARE');
  await key('Space', 300);
  ok(await call('dbg_porthat') === 1 && await bright(1) === 'AS WORN', 'A: hat dial back to AS WORN');
  ok(await call('dbg_carried') === 800, 'A: posing never touched the real hat (deci-kg)');
  await page.screenshot({ path: 'tsg-v125-f9-booth-pose.png' });
  // SNAP — and catch the PNG the shell saves from the frame
  ok(await selectRow(2), 'A: snap row selected');
  const dlP = page.waitForEvent('download', { timeout: 20000 }).catch(() => null);
  await key('Space', 600);
  ok(await poll(call, 1, 5000, 'dbg_porthas'), 'A: the portrait is frozen');
  ok(await call('dbg_portkg') === 80, 'A: portrait lettered 80 kg', await call('dbg_portkg'));
  ok(await call('dbg_portn') === 40, 'A: portrait holds all 40 carcasses', await call('dbg_portn'));
  ok(await call('dbg_porttop') === 1, 'A: portrait wears the CROWN');
  ok(await call('dbg_portbk') === 1 && await call('dbg_porthaton') === 1, 'A: portrait froze FROZEN CACHE, hat on');
  let kindsOk = true;
  for (let i = 0; i < 40; i++) if (await call('dbg_portkind', i) !== 2) kindsOk = false;
  ok(kindsOk, 'A: every frozen carcass kind is the stack the buddy wore (2)');
  ok(await call('dbg_carried') === 800, 'A: the real hat is still on after the snap (deci-kg)');
  ok(await poll(call, 1, 5000, 'dbg_portserial'), 'A: the shell was handed a frame (serial >= 1)', await call('dbg_portserial'));
  const pinfo = await str('portrait_info');
  const pp = pinfo.split('|');
  ok(pp.length === 7 && +pp[0] >= 1 && +pp[3] === 208 && +pp[4] === 260 && +pp[6] === 80 && pp[5].length > 0,
     'A: portrait_info carries serial|rect 208x260|name|80', pinfo);
  const dl = await dlP;
  ok(!!dl && dl.suggestedFilename() === 'survival-portrait.png', 'A: the shell saved survival-portrait.png', dl && dl.suggestedFilename());
  if (dl) {
    const pth = await dl.path();
    const buf = fs.readFileSync(pth);
    ok(buf.length > 5000 && buf[0] === 0x89 && buf[1] === 0x50 && buf[2] === 0x4e && buf[3] === 0x47,
       'A: the download is a real PNG of substance', buf.length + ' bytes');
  }
  blob = await pollSave(2, ' pt=');
  ok(blob && / pt=1,1,1,1,1,80,40,/.test(blob), 'A: the pt= token froze dial/has/backdrop/hat/topper/kg/count', blob && (blob.match(/ pt=[0-9,:]+/) || ['<none>'])[0]);
  ok(await bright(3) === 'FROZEN CACHE', 'A: YOUR PORTRAIT now names its backdrop', await bright(3));
  await page.screenshot({ path: 'tsg-v125-f9-booth-snapped.png' });

  // ============ Phase A2: the gallery — a photo, not a mirror ======
  ok(await selectRow(3), 'A2: portrait row selected');
  await key('Space', 400);
  ok(await poll(call, 6, 4000, 'dbg_portstage'), 'A2: the gallery opens');
  await page.screenshot({ path: 'tsg-v125-f9-gallery.png' });
  await key('Space', 400);                      // the gallery's one row: back
  ok(await poll(call, 5, 4000, 'dbg_portstage'), 'A2: back to the booth');
  await key('Escape', 400);
  ok(await poll(call, 1, 4000, 'dbg_state'), 'A2: panel closed');
  // bank the whole hat at the Bug Bank, then look again
  await call('dbg_tp', 0, 80); await key('KeyE', 300);
  await walkTo(105, -6148, 14); await key('KeyE', 400);   // MALL_SX[2]=105, disc row MALL_CY-148
  ok(await poll(call, 2, 6000, 'dbg_station'), 'A2: Bug Bank opens');
  ok(await selectRow(0), 'A2: DEPOSIT ALL row selected');
  await key('Space', 500);
  ok(await poll(call, 0, 5000, 'dbg_carried'), 'A2: the hat is banked — head bare', await call('dbg_carried'));
  ok(await call('dbg_bank') === 180, 'A2: banked 100 + 80', await call('dbg_bank'));
  await key('Escape', 400);
  ok(await openBooth(), 'A2: the booth fixture reopens, hat gone');
  ok(await selectRow(3), 'A2: portrait row selected');
  await key('Space', 400);
  ok(await poll(call, 6, 4000, 'dbg_portstage'), 'A2: the gallery still hangs');
  ok(await call('dbg_portn') === 40 && await call('dbg_portkg') === 80,
     'A2: the portrait kept the banked hat — a photo, not a mirror');
  await page.screenshot({ path: 'tsg-v125-f9-gallery-afterbank.png' });
  await key('Escape', 500);

  // ============ Phase A3: the portrait survives a save/load ========
  ok(await loadSlot(2, 2, async () => (await call('dbg_porthas')) === 1), 'A3: portrait present after reload (polled)');
  ok(await call('dbg_portkg') === 80 && await call('dbg_portn') === 40 && await call('dbg_porttop') === 1 && await call('dbg_portbk') === 1,
     'A3: kg / count / topper / backdrop all survived');
  ok(await call('dbg_portbd') === 1, 'A3: the backdrop dial persisted too');
  ok(await call('dbg_bank') === 180, 'A3: the bank survived alongside');
  ok(await openBooth(), 'A3: the booth fixture opens after reload');
  ok(await bright(3) === 'FROZEN CACHE', 'A3: YOUR PORTRAIT names FROZEN CACHE after reload', await bright(3));
  ok(await selectRow(3), 'A3: portrait row selected');
  await key('Space', 400);
  ok(await poll(call, 6, 4000, 'dbg_portstage'), 'A3: the gallery reopens after reload');
  await page.screenshot({ path: 'tsg-v125-f9-gallery-reload.png' });
  await key('Escape', 500);

  // ============ Phase C: a pre-feature save loads intact ===========
  await writeAtTitle('tsg_save3', blob.replace(/ pt=[0-9,:]+/, ''));   // hardened: written at the title, not under slot 2's live run
  ok(await loadSlot(3, 2, async () => (await call('dbg_casbest')) === 1250 && (await call('dbg_bank')) === 100), 'C: pre-feature slot 3 loads intact (Castle 1250, bank 100 — not slot 2\'s 180)');
  ok(await call('dbg_bank') === 100, 'C: its bank is intact', await call('dbg_bank'));
  ok(await call('dbg_porthas') === 0 && await call('dbg_portbd') === 0, 'C: no portrait, dial at PLAZA DAWN');
  blob = await pollSave(3, 'cb=1250');
  ok(blob && !blob.includes(' pt='), 'C: a save cycle writes NO pt= while no portrait exists');
  // ...until the player actually snaps one there
  await call('dbg_feed'); await call('dbg_feed');
  ok(await openBooth(), 'C: the booth fixture opens');
  ok(await selectRow(2), 'C: snap row selected');
  await key('Space', 500);
  ok(await poll(call, 1, 5000, 'dbg_porthas'), 'C: the old slot can snap its first portrait');
  ok(await call('dbg_portkg') === 100 && await call('dbg_portn') === 50, 'C: 50 carcasses, 100 kg frozen (40 inherited from the fixture + 10 fed)', await call('dbg_portkg') + '/' + await call('dbg_portn'));
  blob = await pollSave(3, ' pt=');
  ok(blob && blob.includes(' pt='), 'C: pt= appears only after the snap');
  await key('Escape', 500);

  // ============ Phase F: Eddie sits for his portrait ================
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  let mc = 0;
  for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(300); const b2 = await call('dbg_menucount'); if (a === b2 && a >= 6) { mc = a; break; } }
  ok(mc >= 6, 'F: title settled', mc);
  let onEddie = false;
  for (let i = 0; i < 10; i++) { const c = await call('dbg_menucount'), s = await call('dbg_menusel'); if (s === c - 1) { onEddie = true; break; } await key('ArrowDown', 150); }
  ok(onEddie, 'F: selection on the EDDIE row (the last)');
  await key('Space', 400);
  ok(await poll(call, 1, 4000, 'dbg_meddie'), 'F: Eddie mode on');
  // back to CONTINUE (row 0) and into slot 2 (its portrait hangs there)
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 0) break; await key('ArrowUp', 120); }
  await key('Enter', 800);
  let inRun = false;
  for (let r = 0; r < 4 && !inRun; r++) {
    if (await poll(call, 7, 2500, 'dbg_state')) {
      await key('Enter', 500);
      if (await call('dbg_state') === 7) await key('Enter', 700);
    }
    inRun = await poll(call, 1250, 2500, 'dbg_casbest');
    if (!inRun) { await key('ArrowDown', 200); await key('Enter', 500); if (await call('dbg_state') === 7) await key('Enter', 700); inRun = await poll(call, 1250, 2500, 'dbg_casbest'); }
  }
  ok(inRun, 'F: continued into slot 2 with Eddie on');
  ok(await call('dbg_porthas') === 1, 'F: the portrait is there under Eddie');
  ok(await openBooth(), 'F: the booth fixture opens');
  ok(await poll(call, 5, 4000, 'dbg_portstage'), 'F: the booth opens under Eddie');
  await page.screenshot({ path: 'tsg-v125-f9-eddie-booth.png' });
  ok(await selectRow(3), 'F: portrait row selected');
  await key('Space', 400);
  ok(await poll(call, 6, 4000, 'dbg_portstage'), 'F: the Eddie gallery opens');
  await page.screenshot({ path: 'tsg-v125-f9-eddie-gallery.png' });
  await key('Escape', 400);

  // ============ Phase D: Zen poses too — and sets no record ========
  // nothing after this phase needs the old slots: wipe the whole
  // profile so the Zen run is deterministic (fresh title, slot 1)
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  // ZEN MODE is the second-to-last title row whatever the flavour —
  // navigate by the live selection (the cartcoop lesson), never by
  // counted presses: the flavour latch is racy at boot
  mc = 0;
  for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(300); const b2 = await call('dbg_menucount'); if (a === b2 && a >= 6) { mc = a; break; } }
  ok(mc >= 6, 'D: title settled', mc);
  let onZen = false;
  for (let i = 0; i < 10; i++) { const c = await call('dbg_menucount'), s = await call('dbg_menusel'); if (s === c - 2) { onZen = true; break; } await key('ArrowDown', 150); }
  ok(onZen, 'D: selection on the ZEN row');
  await key('Space', 400);
  ok(await poll(call, 1, 4000, 'dbg_mzen'), 'D: Zen toggled on');
  let onStart = false;
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 0) { onStart = true; break; } await key('ArrowUp', 150); }
  ok(onStart, 'D: selection back on START (fresh profile: row 0)');
  await key('Space', 500);
  ok(await poll(call, 7, 6000, 'dbg_state'), 'D: slot picker opens');
  await key('Enter', 500); await key('Enter', 900);
  ok(await poll(call, 1, 8000, 'dbg_state'), 'D: fresh Zen run started');
  ok(await call('dbg_zen') === 1, 'D: the run is Zen');
  await call('dbg_adopt', 1);
  await call('dbg_feed'); await call('dbg_feed'); await call('dbg_feed');
  ok(await openBooth(), 'D: the booth fixture opens in Zen');
  ok(await poll(call, 5, 4000, 'dbg_portstage'), 'D: the booth opens in Zen');
  ok(await selectRow(2), 'D: snap row selected');
  await key('Space', 500);
  ok(await poll(call, 1, 5000, 'dbg_porthas'), 'D: a Zen portrait freezes (a keepsake, not a record)');
  ok(await call('dbg_portkg') === 30 && await call('dbg_portn') === 15, 'D: 15 carcasses, 30 kg');
  await sleep(6500);                            // let an autosave land
  const recAfter = await page.evaluate(() => (localStorage.getItem('tsg_save') || ''));
  const hatOf = s => { const m = s.match(/rec_hat=([0-9.]+)/); return m ? +m[1] : 0; };
  ok(hatOf(recAfter) === 0,
     'D: no heaviest-hat record exists after a Zen portrait (records are Zen-gated)', 'rec_hat=' + hatOf(recAfter));
  await key('Escape', 500);

  console.log(`RESULT pass=${pass} fail=${fail}`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', e.message); process.exit(2); });
