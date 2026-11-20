// intensitytest.js — v1.25 feature 15: ADAPTIVE MUSIC INTENSITY.
// The v1.23 arrangements breathe in two levels: SPARSE (the lead
// alone) far from danger, FULL (lead + bass + accompaniment) as bug
// density or an active chase rises — latched at bar boundaries,
// with a 4-second held breath on the release. Plus two stings: an
// elite appearing (sfx 24) and a bounty claimed (sfx 25). Audio is
// verified through the Nov 6 js_note probe (wrapped at instantiate;
// records [ch, note, vel, instr, t]); RMS is the Nov 6 energy
// metric, sqrt(sum(peak^2 * voice_dur)/window), voicePeak mirrored
// from audioprobe. The game's own seams stage the danger
// (dbg_spawnbug / dbg_tp / dbg_bwhackhere / dbg_bnforce) and the
// feature's exports only OBSERVE (dbg_muslvl / dbg_mushot /
// dbg_musctx). Spec: v125-feat15-spec.md.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 200) : ''); }
}
function voicePeak(ch, vel, instr) {
  const v = Math.min(1, vel / 110);
  if (instr === 3) return v * 0.7;
  if (instr === 4) return v * (ch === 0 ? 0.26 : 0.32);
  return v * (ch === 0 ? 0.30 : 0.36);
}
function rmsOf(notes, sec) {
  let e = 0;
  for (const [ch, , vel, instr] of notes) {
    if (ch === 3) continue;
    const pk = voicePeak(ch, vel, instr);
    const dur = instr === 3 ? 0.12 : (ch === 1 ? 0.24 : 0.16);
    e += pk * pk * dur;
  }
  return Math.sqrt(e / sec);
}
const median = a => { if (!a.length) return -1; const s = [...a].sort((x, y) => x - y); return s[s.length >> 1]; };

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    window.__notes = []; window.__collect = 0;
    const wrap = (imports) => {
      try {
        const orig = imports.env.js_note;
        imports.env.js_note = function (ch, note, vel, instr) {
          if (window.__collect) window.__notes.push([ch, note, vel, instr, performance.now()]);
          return orig(ch, note, vel, instr);
        };
      } catch (e) {}
      return imports;
    };
    const inst = WebAssembly.instantiate.bind(WebAssembly);
    WebAssembly.instantiate = (bytes, imports) => inst(bytes, wrap(imports));
    const instS = WebAssembly.instantiateStreaming && WebAssembly.instantiateStreaming.bind(WebAssembly);
    if (instS) WebAssembly.instantiateStreaming = (src, imports) => instS(src, wrap(imports));
  });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(150); } return v; };
  // collect for ms, polling a contamination predicate every 250ms;
  // a sparse window is contaminated if the latch leaves the expected
  // level or the raw signal stays hot for >700ms (a shorter blip
  // cannot cross the latch — the held breath absorbs it, by design)
  async function collectWin(ms, expectLvl) {
    await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
    const t0 = Date.now(); let contaminated = false, hotSince = -1;
    while (Date.now() - t0 < ms) {
      await sleep(250);
      if (expectLvl !== undefined) {
        const lvl = await call('dbg_muslvl');
        const hot = await call('dbg_mushot');
        if (lvl !== expectLvl) { contaminated = true; break; }
        if (hot === 1) { if (hotSince < 0) hotSince = Date.now(); else if (Date.now() - hotSince > 700) { contaminated = true; break; } }
        else hotSince = -1;
      }
    }
    const notes = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
    return { notes, contaminated };
  }
  const chan = (notes, ch) => notes.filter(n => n[0] === ch);
  async function tpClear() { await call('dbg_tp', 5000, 5000); await sleep(500); await call('dbg_tp', 0, 80); await sleep(300); }
  // The spawner's approach stream converges on a player who stands
  // still ANYWHERE (that convergence is the game working — far out
  // it is a flood: pop target 200, paired spawns, 100+ u/s bugs),
  // so a long calm window is collected while hopping through empty
  // far country. Each hop culls the whole field (the 1750 rule);
  // the 1.5s hop interval is derived, not guessed: the fastest bug
  // the game can make (~106 u/s) spawning at the nearest radius
  // (620) needs (620-430)/106 = 1.79s just to reach the density
  // ring, so at 1.5s the meter honestly reads clear throughout —
  // the same calm a player crosses between waves, held still for
  // the probe.
  const HOPS = [[20000, 20000], [-24000, 6000], [6000, -24000], [-20000, -20000], [26000, -6000], [-6000, 26000]];
  let hopI = 0;
  async function hop() { hopI = (hopI + 1) % HOPS.length; await call('dbg_tp', HOPS[hopI][0], HOPS[hopI][1]); }
  function hopClock() { const c = { last: Date.now(), async tick() { if (Date.now() - c.last > 1500) { await hop(); c.last = Date.now(); } } }; return c; }
  async function waitLvlHopping(want, ms) {
    const hc = hopClock(); const t0 = Date.now();
    while (Date.now() - t0 < ms) {
      if ((await call('dbg_muslvl')) === want) return true;
      await hc.tick(); await sleep(200);
    }
    return (await call('dbg_muslvl')) === want;
  }
  async function collectCalm(ms) {
    await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
    const hc = hopClock(); const t0 = Date.now();
    let contaminated = false, hotSince = -1;
    while (Date.now() - t0 < ms) {
      await sleep(250);
      await hc.tick();
      const lvl = await call('dbg_muslvl');
      const hot = await call('dbg_mushot');
      if (lvl !== 0) { contaminated = true; break; }
      if (hot === 1) { if (hotSince < 0) hotSince = Date.now(); else if (Date.now() - hotSince > 700) { contaminated = true; break; } }
      else hotSince = -1;
    }
    const notes = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
    return { notes, contaminated };
  }
  async function stageChase() {
    const px = await call('dbg_px'), py = await call('dbg_py');
    await call('dbg_spawnbug', px + 150, py);
    await call('dbg_spawnbug', px - 150, py + 30);
    await call('dbg_spawnbug', px + 40, py - 150);
  }
  async function menuGoTo(idx) {
    const n = (await call('dbg_menun')) + 1;
    let cur = await call('dbg_menusel');
    const steps = ((idx - cur) % n + n) % n;
    for (let i = 0; i < steps; i++) await key('ArrowDown', 110);
    return (await call('dbg_menusel')) === idx;
  }
  async function openSettings(idx) {
    await menuGoTo(idx);
    await key('Space', 400);
    return (await poll('dbg_state', v => v === 6, 6000)) === 6;
  }
  async function saveQuitToTitle() {
    await key('KeyP', 500);
    if ((await poll('dbg_state', v => v === 5, 6000)) !== 5) return false;
    // walk to SAVE + QUIT (row 2) one verified tap at a time — the
    // poll between taps is this suite's guard against the known
    // held-key double-step race (it never holds the key)
    for (let i = 0; i < 8 && (await call('dbg_pausesel')) !== 2; i++) { await key('ArrowDown', 160); }
    if ((await call('dbg_pausesel')) !== 2) return false;
    await key('Space', 600);
    return (await poll('dbg_state', v => v === 0, 8000)) === 0;
  }
  async function continueRun() {
    await menuGoTo(0);
    await key('Space', 500);
    await key('Enter', 500);
    return (await poll('dbg_state', v => v === 1, 8000)) === 1;
  }
  async function exitMall() {
    await call('dbg_tp', 0, -5810); await sleep(250);
    await key('KeyE', 300);
    return (await poll('dbg_mall', v => v === 0, 6000)) === 0;
  }

  // ============ boot + a fresh run (no buddy yet, on purpose) ============
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(400);
  T('boots to the title', (await call('dbg_state')) === 0, await call('dbg_state'));
  await menuGoTo(0);
  await key('Space', 500);
  await key('Enter', 500);
  T('run started', (await poll('dbg_state', v => v === 1, 8000)) === 1, await call('dbg_state'));

  // ============ piece identity is unmoved (gating never selects) ============
  T('calm ladder reads piece 0', (await call('dbg_piece')) === 0, await call('dbg_piece'));
  for (const [t, idx] of [[25, 1], [45, 2], [70, 3]]) {
    await call('dbg_setthreat', t); await sleep(400);
    T('threat override ' + t + ' still selects piece ' + idx, (await call('dbg_piece')) === idx, await call('dbg_piece'));
  }
  await call('dbg_setthreat', -1); await sleep(300);
  T('ladder restored to piece 0', (await call('dbg_piece')) === 0, await call('dbg_piece'));

  // ============ leg 1: a calm field is SPARSE (lead alone) ============
  let sparseWin = null;
  for (let attempt = 0; attempt < 3 && !sparseWin; attempt++) {
    await hop();
    await poll('dbg_musctx', v => v === 1, 4000);
    await waitLvlHopping(0, 15000);
    const w = await collectCalm(9000);
    if (!w.contaminated) sparseWin = w; else console.log('  (calm window contaminated, retrying)');
  }
  T('calm field: context is the overworld', (await call('dbg_musctx')) === 1, await call('dbg_musctx'));
  T('calm field: the latch sits SPARSE', sparseWin !== null, 'no clean 9s window in 3 attempts');
  if (sparseWin) {
    const n = sparseWin.notes;
    T('sparse: no bass notes at all', chan(n, 1).length === 0, 'bass=' + chan(n, 1).length);
    T('sparse: no accompaniment notes at all', chan(n, 2).length === 0, 'mid=' + chan(n, 2).length);
    T('sparse: the lead still plays', chan(n, 0).length >= 1, 'lead=' + chan(n, 0).length);
    console.log('  [sparse overworld] lead ' + chan(n, 0).length + ' / bass ' + chan(n, 1).length + ' / mid ' + chan(n, 2).length + ' / RMS ' + rmsOf(n, 9).toFixed(4));
  }
  const sparseRms = sparseWin ? rmsOf(sparseWin.notes, 9) : 0;

  // ============ leg 2: a staged chase FILLS IN at the next bar ============
  await call('dbg_tp', 0, 80); await sleep(300);   // home turf: the staged bugs are small and slow
  await stageChase();
  T('danger: the raw signal flips hot at once', (await poll('dbg_mushot', v => v === 1, 3000)) === 1, await call('dbg_mushot'));
  T('danger: the latch fills within a bar', (await poll('dbg_muslvl', v => v === 1, 9000)) === 1, await call('dbg_muslvl'));
  const fullWin = await collectWin(4000);
  const fn = fullWin.notes;
  T('full: bass is back', chan(fn, 1).length > 0, 'bass=' + chan(fn, 1).length);
  T('full: accompaniment is back', chan(fn, 2).length > 0, 'mid=' + chan(fn, 2).length);
  T('full: the lead plays on', chan(fn, 0).length > 0, 'lead=' + chan(fn, 0).length);
  const fullRms = rmsOf(fn, 4);
  console.log('  [full overworld]   lead ' + chan(fn, 0).length + ' / bass ' + chan(fn, 1).length + ' / mid ' + chan(fn, 2).length + ' / RMS ' + fullRms.toFixed(4));
  T('full RMS dwarfs sparse RMS (>2x)', sparseRms > 0 && fullRms > 2 * sparseRms, 'full ' + fullRms.toFixed(4) + ' vs sparse ' + sparseRms.toFixed(4));
  const leadVelFull = median(chan(fn, 0).map(n => n[2]));

  // ============ leg 3: the release holds its breath, then lets go ============
  await hop();   // away from the fight: everything hunting us is culled where it stands
  T('release: the signal clears at once', (await poll('dbg_mushot', v => v === 0, 3000)) === 0, await call('dbg_mushot'));
  { const hc = hopClock(); const t0 = Date.now(); while (Date.now() - t0 < 2800) { await hc.tick(); await sleep(200); } }
  T('release: 2.8s into the clear, the latch still holds FULL', (await call('dbg_muslvl')) === 1, await call('dbg_muslvl'));
  T('release: the latch lets go after the breath + a bar', await waitLvlHopping(0, 8000), await call('dbg_muslvl'));
  let reSparse = null;
  for (let attempt = 0; attempt < 3 && !reSparse; attempt++) {
    const w = await collectCalm(6000);
    if (!w.contaminated) reSparse = w;
    else console.log('  (release window contaminated, retrying)');
  }
  T('release: the field is sparse again', reSparse !== null && chan(reSparse.notes, 1).length === 0 && chan(reSparse.notes, 2).length === 0,
    reSparse ? 'bass=' + chan(reSparse.notes, 1).length + ' mid=' + chan(reSparse.notes, 2).length : 'no clean window');

  // ============ leg 4: the Mall interior is NOT a gated moment ============
  await call('dbg_adopt', 0);
  T('buddy adopted for the Banzai legs', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1, 6000)) === 1, await call('dbg_mall'));
  T('mall: context reads ungated', (await poll('dbg_musctx', v => v === 0, 4000)) === 0, await call('dbg_musctx'));
  const mallWin = await collectWin(6000);
  T('mall: the full arrangement plays (bass present)', chan(mallWin.notes, 1).length > 0, 'bass=' + chan(mallWin.notes, 1).length);
  T('mall: the full arrangement plays (accompaniment present)', chan(mallWin.notes, 2).length > 0, 'mid=' + chan(mallWin.notes, 2).length);
  console.log('  [mall interior]    lead ' + chan(mallWin.notes, 0).length + ' / bass ' + chan(mallWin.notes, 1).length + ' / mid ' + chan(mallWin.notes, 2).length + ' / RMS ' + rmsOf(mallWin.notes, 6).toFixed(4));

  // ============ leg 5: Banzai World breathes (Sheep, roam) ============
  await call('dbg_tp', 215, -5868); await sleep(250);   // the Banzai gate disc
  await key('KeyE', 250);
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('free play entered Banzai World', (await poll('dbg_bwmode', v => v === 0, 6000)) === 0, await call('dbg_bwmode'));
  T('the plaza plays Sheep (piece 4)', (await call('dbg_piece')) === 4, await call('dbg_piece'));
  T('banzai: context is Banzai', (await poll('dbg_musctx', v => v === 2, 4000)) === 2, await call('dbg_musctx'));
  let bwSparse = null;
  for (let attempt = 0; attempt < 3 && !bwSparse; attempt++) {
    if (attempt > 0) { await call('dbg_bwexit'); await sleep(300); await call('dbg_bwenter'); await sleep(500); }
    await poll('dbg_mushot', v => v === 0, 6000);
    await poll('dbg_muslvl', v => v === 0, 9000);
    const w = await collectWin(6000, 0);
    if (!w.contaminated && (await call('dbg_bwbattle')) === 0) bwSparse = w; else console.log('  (banzai calm window contaminated, retrying)');
  }
  T('banzai calm: the latch sits SPARSE', bwSparse !== null, 'no clean window in 3 attempts');
  if (bwSparse) {
    T('banzai sparse: the Sheep sheds its bass', chan(bwSparse.notes, 1).length === 0, 'bass=' + chan(bwSparse.notes, 1).length);
    T('banzai sparse: the lead flute plays on', chan(bwSparse.notes, 0).length > 0, 'lead=' + chan(bwSparse.notes, 0).length);
    console.log('  [sparse banzai]    lead ' + chan(bwSparse.notes, 0).length + ' / bass ' + chan(bwSparse.notes, 1).length + ' / mid ' + chan(bwSparse.notes, 2).length + ' / RMS ' + rmsOf(bwSparse.notes, 6).toFixed(4));
  }
  // a battle fills it in
  await call('dbg_setbudxp', 300);
  await call('dbg_bwhackhere');
  T('banzai: the staged battle starts', (await poll('dbg_bwbattle', v => v > 0, 9000)) > 0, await call('dbg_bwbattle'));
  T('banzai danger: the signal reads hot in a battle', (await poll('dbg_mushot', v => v === 1, 3000)) === 1, await call('dbg_mushot'));
  T('banzai danger: the latch fills within a bar', (await poll('dbg_muslvl', v => v === 1, 7000)) === 1, await call('dbg_muslvl'));
  const bwFull = await collectWin(4000);
  T('banzai full: the bass is back under the Sheep', chan(bwFull.notes, 1).length > 0, 'bass=' + chan(bwFull.notes, 1).length);
  console.log('  [full banzai]      lead ' + chan(bwFull.notes, 0).length + ' / bass ' + chan(bwFull.notes, 1).length + ' / mid ' + chan(bwFull.notes, 2).length + ' / RMS ' + rmsOf(bwFull.notes, 4).toFixed(4));
  // win it (storytest's choose-the-first-move idiom) and watch it settle
  for (let i = 0; i < 60 && (await call('dbg_bwbattle')) > 0; i++) {
    if ((await call('dbg_bwbattle')) === 1) { await call('dbg_bwchoose', 0); }
    await sleep(350);
  }
  T('banzai: the battle is won', (await call('dbg_bwbattle')) === 0 && (await call('dbg_bwstate')) === 1,
    'battle=' + (await call('dbg_bwbattle')) + ' bwstate=' + (await call('dbg_bwstate')));
  T('banzai release: the signal clears after the battle', (await poll('dbg_mushot', v => v === 0, 4000)) === 0, await call('dbg_mushot'));
  T('banzai release: the latch settles back to sparse', (await poll('dbg_muslvl', v => v === 0, 10000)) === 0, await call('dbg_muslvl'));
  await call('dbg_bwexit'); await sleep(300);
  T('back in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));

  // ============ leg 6: Comfort stays sovereign, even at full heat ============
  T('out of the mall for the comfort legs', await exitMall());
  await stageChase();
  await poll('dbg_muslvl', v => v === 1, 9000);
  T('comfort: danger staged and latched full', (await call('dbg_muslvl')) === 1, await call('dbg_muslvl'));
  T('comfort: saved + quit to title', await saveQuitToTitle());
  T('comfort: settings opens (save layout)', await openSettings(3));
  await key('ArrowDown', 120); await key('ArrowDown', 120);   // MUTE row
  await key('ArrowRight', 250);
  const muteVal = await call('dbg_comfort');
  T('comfort: MUTE ALL is on', Math.floor(muteVal / 10) % 10 === 1, muteVal);
  await key('Escape', 400);
  T('comfort: run continued under mute', await continueRun());
  await stageChase();
  await call('dbg_bnforce');
  { const px = await call('dbg_px'), py = await call('dbg_py'); await call('dbg_spawnbug', px + 300, py); }
  await sleep(600);
  T('comfort: the (muted) elite still went live — the game is real, only the sound is off', (await call('dbg_bnelite')) >= 0, await call('dbg_bnelite'));
  const mutedWin = await collectWin(3000);
  T('muted at full heat: ZERO notes on every channel (music + sting alike)', mutedWin.notes.length === 0, 'notes=' + mutedWin.notes.length);
  await call('dbg_tp', 5000, 5000); await sleep(400);   // cull the muted elite; its sting never sounded and never will
  // volume: music at 5 halves the full level's velocities
  T('comfort: saved + quit again', await saveQuitToTitle());
  T('comfort: settings opens to unmute + set volume', await openSettings(3));
  await key('ArrowDown', 120); await key('ArrowDown', 120);
  await key('ArrowLeft', 250);   // mute off
  await key('ArrowUp', 120); await key('ArrowUp', 120);   // MUSIC VOLUME row
  for (let i = 0; i < 5; i++) await key('ArrowLeft', 110);
  const volVal = await call('dbg_comfort');
  T('comfort: unmuted, MUSIC VOLUME at 5', Math.floor(volVal / 10) % 10 === 0 && Math.floor(volVal / 10000) === 5, volVal);
  await key('Escape', 400);
  T('comfort: run continued at half volume', await continueRun());
  await stageChase();
  await poll('dbg_muslvl', v => v === 1, 9000);
  const halfWin = await collectWin(4000);
  const leadVelHalf = median(chan(halfWin.notes, 0).map(n => n[2]));
  const ratio = leadVelFull > 0 && leadVelHalf > 0 ? leadVelHalf / leadVelFull : -1;
  T('half volume: the FULL level still fills in (bass present)', chan(halfWin.notes, 1).length > 0, 'bass=' + chan(halfWin.notes, 1).length);
  T('half volume: lead velocities halve (ratio ~0.5)', ratio > 0.35 && ratio < 0.65, 'full median ' + leadVelFull + ' -> half median ' + leadVelHalf);
  // restore defaults for the sting legs
  T('comfort: saved + quit to restore volume', await saveQuitToTitle());
  T('comfort: settings opens for the restore', await openSettings(3));
  for (let i = 0; i < 5; i++) await key('ArrowRight', 110);
  const restVal = await call('dbg_comfort');
  T('comfort: MUSIC VOLUME restored to 10', Math.floor(restVal / 10000) === 10, restVal);
  await key('Escape', 400);
  T('comfort: run continued at full volume', await continueRun());

  // ============ leg 7: the two stings ============
  await tpClear();
  await poll('dbg_mushot', v => v === 0, 6000);
  // ordinary spawn: NO sting (elitetest's one-evaluate force idiom, minus the force)
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  { const px = await call('dbg_px'), py = await call('dbg_py'); await call('dbg_spawnbug', px - 300, py); }
  await sleep(1300);
  const plainWin = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  T('an ordinary spawn is silent (no sting)', !plainWin.some(n => n[0] === 3 && n[3] === 0 && (n[1] === 45 || n[1] === 40)),
    JSON.stringify(plainWin.filter(n => n[0] === 3).map(n => n[1])));
  await call('dbg_tp', 5000, 5000); await sleep(400); await call('dbg_tp', 0, 80); await sleep(300);
  // the elite: THE BOUNTY STIRS
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  const elite0 = await page.evaluate(() => {
    const px = window.__TSG.dbg_px(), py = window.__TSG.dbg_py();
    window.__TSG.dbg_bnforce();
    window.__TSG.dbg_spawnbug(px + 300, py);
    return window.__TSG.dbg_bnelite();
  });
  await sleep(1300);
  const stirWin = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  T('the forced elite went live', elite0 >= 0, elite0);
  T('THE BOUNTY STIRS sounds as the elite appears (45... 40, square)', stirWin.some(n => n[0] === 3 && n[3] === 0 && n[1] === 45) && stirWin.some(n => n[0] === 3 && n[3] === 0 && n[1] === 40),
    JSON.stringify(stirWin.filter(n => n[0] === 3).map(n => [n[1], n[3]])));
  // kill it (elitetest's kite) and listen for the claim
  await page.evaluate(() => { window.__notes = []; window.__collect = 1; });
  const kills0 = await call('dbg_bndone');
  const fightT0 = Date.now();
  let claimed = false;
  while (Date.now() - fightT0 < 90000) {
    if ((await call('dbg_bndone')) > kills0) { claimed = true; break; }
    if ((await call('dbg_mall')) === 1) { await exitMall(); continue; }   // died: step back out
    const tx = await call('dbg_bnx'), ty = await call('dbg_bny');
    if (tx === -99999) { await sleep(300); continue; }
    const x = await call('dbg_px'), y = await call('dbg_py');
    const dx = tx - x, dy = ty - y, d = Math.hypot(dx, dy);
    const toward = [], away = [];
    if (dx > 10) { toward.push('ArrowRight'); away.push('ArrowLeft'); } else if (dx < -10) { toward.push('ArrowLeft'); away.push('ArrowRight'); }
    if (dy > 10) { toward.push('ArrowDown'); away.push('ArrowUp'); } else if (dy < -10) { toward.push('ArrowUp'); away.push('ArrowDown'); }
    if (d > 85) { for (const k of toward) await page.keyboard.down(k); await sleep(140); for (const k of toward) await page.keyboard.up(k); }
    else if (d < 55) { for (const k of away) await page.keyboard.down(k); await sleep(120); for (const k of away) await page.keyboard.up(k); }
    else {
      for (const k of toward) await page.keyboard.down(k); await sleep(50);
      await key('Space', 100); if (d >= 65) await key('Space', 100);
      for (const k of toward) await page.keyboard.up(k);
      for (const k of away) await page.keyboard.down(k); await sleep(130); for (const k of away) await page.keyboard.up(k);
    }
  }
  if (!claimed) claimed = (await call('dbg_bndone')) > kills0;
  await sleep(900);
  const claimWin = await page.evaluate(() => { window.__collect = 0; return window.__notes; });
  T('the elite was killed and the bounty claimed', claimed, 'bndone=' + (await call('dbg_bndone')));
  const flourish = claimWin.filter(n => n[0] === 3 && n[3] === 1).map(n => n[1]);
  T('BOUNTY CLAIMED sounds at the settle (triangle 67/72/76/79)', flourish.includes(67) && flourish.includes(79) && [67, 72, 76, 79].filter(p => flourish.includes(p)).length >= 3,
    'instr1 ch3 notes: ' + flourish.join(','));
  const jingle = claimWin.filter(n => n[0] === 3 && n[3] === 0).map(n => n[1]);
  T('the settle jingle (sfx 11) still sounds beside it', jingle.includes(72) && jingle.includes(76), 'instr0 ch3 has 72:' + jingle.includes(72) + ' 76:' + jingle.includes(76));

  console.log('intensitytest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
