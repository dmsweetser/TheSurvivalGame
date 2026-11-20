// runplay.js — PLAYTEST driver for SUPER BUDDY RUN (not a suite).
// Enters the mode the way a player does: title -> walk to the hut ->
// Mall -> walk to the Arcade door -> walk to the RUN cabinet -> panel
// -> PLAY. Inside levels it plays with real key holds: run right,
// jump on stalls / rhythm / fall-save. Debug getters OBSERVE only
// (positions, hearts, clock) — never move the player or enter modes.
// (dbg_adopt at boot is the standard suite setup, outside the mode.
// Note: dbg_budrevive only RETURNS the revive price — a fainted buddy
// is revived in-game at the Banzai Buddies station for hat kg, so a
// death ends a profile's session here, as it would for a player.)
// Usage: node runplay.js <profileDir> <mode> [args]
//   climb  <toLvl>            play frontier levels until plat_done >= toLvl
//   replay <lvl,tries> [...]  e.g. replay 1,3 5,2 — level-select replays
//   current                   just play whatever level loads on entry
const { chromium } = require('playwright-core');
const fs = require('fs');
const path = require('path');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = path.join(__dirname, 'frun-logs');
fs.mkdirSync(OUT, { recursive: true });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const stamp = () => new Date().toISOString().slice(11, 19);

const profileDir = process.argv[2];
const mode = process.argv[3] || 'climb';
const arg = process.argv[4] || '';
let SEED = 0;
// the bot's "eyes": the offline generator replica (runmap.js) says
// where the pits and critters are, the way a player sees them on
// screen — used only to TIME inputs, never to move the runner
const { execFileSync } = require('child_process');
const mapCache = {};
function mapFor(seed, lvl) {
  const k = seed + ':' + lvl;
  if (mapCache[k]) return mapCache[k];
  const gaps = [], foes = [], leaps = [], bonks = [];
  if (seed > 0) {
    try {
      const out = execFileSync('node', [path.join(__dirname, 'runmap.js'), String(seed), String(lvl), String(lvl)], { encoding: 'utf8' });
      for (const m of out.matchAll(/GAP at tile (\d+) width (\d+)/g)) gaps.push({ x0: (+m[1]) * 24, x1: (+m[1] + +m[2]) * 24 });
      const em = out.match(/enemies: (.*)/);
      if (em) for (const m of em[1].matchAll(/kind\d+@tile(\d+)/g)) foes.push((+m[1]) * 24 + 12);
      // pipes and stairs are leapt in stride, not climbed from a stall
      for (const m of out.matchAll(/x=(\d+) \(px \d+\) \{"x":\d+,"type":"(pipe|stairs)"/g)) leaps.push((+m[1]) * 24);
      const wm = out.match(/W=(\d+)/);
      if (wm) leaps.push((+wm[1] - 13) * 24);   // the run-home staircase
      // '?' blocks in row 9 of the ASCII map: bonk them for powerups
      const rows = out.split('\n').filter(l => /^\s*\d+ /.test(l));
      const r9 = rows.find(l => l.startsWith(' 9 '));
      if (r9) { const body = r9.slice(3); for (let i = 0; i < body.length; i++) if (body[i] === '?') bonks.push(i * 24 + 12); }
    } catch (e) { log({ ev: 'map-fail', seed, lvl }); }
  }
  return mapCache[k] = { gaps, foes, leaps, bonks };
}
const RUNLOG = path.join(OUT, 'play-' + path.basename(profileDir) + '-' + mode + '.jsonl');
function log(ev) { const line = JSON.stringify({ t: stamp(), ...ev }); console.log(line); fs.appendFileSync(RUNLOG, line + '\n'); }

(async () => {
  const ctx = await chromium.launchPersistentContext(profileDir, {
    executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'],
    viewport: { width: 1300, height: 800 },
  });
  const page = ctx.pages()[0] || await ctx.newPage();
  page.on('pageerror', e => log({ ev: 'PAGEERROR', msg: String(e).slice(0, 200) }));
  const fresh = !fs.existsSync(path.join(profileDir, '.booted'));
  await page.goto(URL, { waitUntil: 'load' });
  if (fresh) {
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    fs.writeFileSync(path.join(profileDir, '.booted'), '1');
  }
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const S = () => page.evaluate(() => {
    const T = window.__TSG;
    return {
      state: T.dbg_state(), px: T.dbg_px(), py: T.dbg_py(), mall: T.dbg_mall(), inarcade: T.dbg_inarcade(),
      arcgate: T.dbg_arcgate(), bwmode: T.dbg_bwmode(), bwx: T.dbg_bwx(), bwy: T.dbg_bwy(),
      plath: T.dbg_plath(), platlvl: T.dbg_platlvl(), plattime: T.dbg_plattime(), platdone: T.dbg_platdone(),
      platsel: T.dbg_platsel(), platpick: T.dbg_platpick(), platsub: T.dbg_platsub(), platpow: T.dbg_platpow(),
      platbits: T.dbg_platbits(), platvaultx: T.dbg_platvaultx(), faint: T.dbg_fainted(), platregion: T.dbg_platregion(),
      budhp: T.dbg_buddyhp(),
    };
  });
  const shot = async name => { await page.screenshot({ path: path.join(OUT, name) }); log({ ev: 'shot', name }); };
  // lite poll for the play loop: few exports, ~30ms cadence
  const SP = () => page.evaluate(() => {
    const T = window.__TSG;
    return {
      bwmode: T.dbg_bwmode(), bwx: T.dbg_bwx(), bwy: T.dbg_bwy(), plath: T.dbg_plath(),
      platlvl: T.dbg_platlvl(), plattime: T.dbg_plattime(), platsub: T.dbg_platsub(),
      platpow: T.dbg_platpow(), platvaultx: T.dbg_platvaultx(), faint: T.dbg_fainted(),
    };
  });
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };

  // ---- title settle (poll for stability — exports answer pre-frame)
  async function titleStart() {
    let prev = -1;
    for (let i = 0; i < 30; i++) { const c = await call('dbg_menucount'); if (c === prev && c > 0) break; prev = c; await sleep(250); }
    await key('Space', 350);
    let st = await call('dbg_state');
    if (st !== 1) { await key('Enter', 350); st = await call('dbg_state'); }
    if (st === 7) { await key('Enter', 350); await sleep(300); await key('Enter', 350); }  // slot picker -> confirm
    const t0 = Date.now();
    while (Date.now() - t0 < 9000) { if ((await call('dbg_state')) === 1) return true; await sleep(200); }
    return (await call('dbg_state')) === 1;
  }
  // ---- walk the player (overworld / mall / arcade) to a point:
  // hold the keys continuously like a person, re-aiming as we go
  async function walkTo(tx, ty, timeout = 25000, tol = 12, opts = {}) {
    const t0 = Date.now(); let lastPos = '', stallSince = Date.now(), jig = false;
    const held = new Set();
    const setKeys = async want => {
      for (const k of [...held]) if (!want.includes(k)) { await page.keyboard.up(k); held.delete(k); }
      for (const k of want) if (!held.has(k)) { await page.keyboard.down(k); held.add(k); }
    };
    while (Date.now() - t0 < timeout) {
      const s = await S();
      const x = s.px, y = s.py;
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) { await setKeys([]); return true; }
      const pos = x + ',' + y, now = Date.now();
      if (pos !== lastPos) { lastPos = pos; stallSince = now; }
      const want = [];
      if (x < tx - tol) want.push('ArrowRight'); else if (x > tx + tol) want.push('ArrowLeft');
      if (y < ty - tol) want.push('ArrowDown'); else if (y > ty + tol) want.push('ArrowUp');
      if (!opts.noJiggle && now - stallSince > 900) { want.push(jig ? 'ArrowDown' : 'ArrowUp'); if (now - stallSince > 1600) { jig = !jig; stallSince = now; } }
      await setKeys(want);
      await sleep(80);
    }
    await setKeys([]);
    return false;
  }
  // ---- the road: wherever we are -> inside SUPER BUDDY RUN
  async function roadToRun() {
    let s = await S();
    if (s.bwmode === 3) return true;
    if (s.state === 0) { if (!await titleStart()) { log({ ev: 'road-fail', at: 'title' }); return false; } s = await S(); }
    // the feat-7 reload quirk: a save written in the Arcade reloads at
    // the Mall centre still flagged in_arcade — walk to the exit mat
    // (arcade coords) and step out before doing anything else
    if (s.inarcade && (s.py < -12300 || s.py > -11400)) {
      log({ ev: 'road', at: 'arcade-quirk-recovery', px: s.px, py: s.py });
      await walkTo(-340, -11976, 20000, 14, { noJiggle: true });
      await key('KeyE', 250); await sleep(600);
      s = await S();
      log({ ev: 'road', at: 'quirk-recovered', inarcade: s.inarcade, mall: s.mall, px: s.px, py: s.py });
    }
    if (!s.mall && !s.inarcade) {
      await walkTo(0, 0, 30000);
      await key('KeyE', 250); await sleep(500);
      s = await S();
      if (!s.mall) { await walkTo(0, 10, 8000); await key('KeyE', 250); await sleep(500); s = await S(); }
      if (!s.mall) { log({ ev: 'road-fail', at: 'hut', px: s.px, py: s.py }); return false; }
      log({ ev: 'road', at: 'mall', budhp: s.budhp });
    }
    if (!s.inarcade) {
      await walkTo(238, -5976, 25000);
      await key('KeyE', 250); await sleep(500);
      s = await S();
      if (!s.inarcade) { log({ ev: 'road-fail', at: 'arcade-door', px: s.px, py: s.py }); return false; }
      log({ ev: 'road', at: 'arcade', budhp: s.budhp });
    }
    await walkTo(0, -11976, 20000, 14);          // east along the corridor
    await walkTo(0, -12108, 12000, 16, { noJiggle: true });   // north to the RUN cabinet (collision stops ~13 short)
    await sleep(500);                            // let any latched key edges flush before the panel opens
    for (let attempt = 0; attempt < 3; attempt++) {
      await key('KeyE', 250);
      const t0 = Date.now(); s = await S();
      while (!s.arcgate && Date.now() - t0 < 2500) { await sleep(120); s = await S(); }
      if (!s.arcgate) {
        if (s.faint) { log({ ev: 'road', at: 'buddy-fainted-on-road', budhp: s.budhp }); await call('dbg_budrevive'); await sleep(500); continue; }
        log({ ev: 'road-fail', at: 'cabinet', px: s.px, py: s.py, budhp: s.budhp, faint: s.faint }); await shot('road-fail-cabinet.png'); return false;
      }
      if (attempt === 0) { log({ ev: 'road', at: 'cabinet-panel' }); await shot('cabinet-panel.png'); }
      await key('Space', 220);                   // PLAY (row 0) — quick, before any stray nav edge can land
      const t1 = Date.now(); s = await S();
      while (s.bwmode !== 3 && Date.now() - t1 < 2500) { await sleep(150); s = await S(); }
      if (s.bwmode === 3) { SEED = await call('dbg_bwseed'); log({ ev: 'road', at: 'in-run', platlvl: s.platlvl, platsel: s.platsel, seed: SEED }); return true; }
      log({ ev: 'road', at: 'launch-retry', attempt });
      await sleep(400);
    }
    log({ ev: 'road-fail', at: 'launch', bwmode: s.bwmode }); return false;
  }
  // if the level select is up, start the preselected (frontier) level
  async function startPicked() {
    let s = await S();
    if (!s.platsel) return true;
    await sleep(600);
    await key('Space', 250);
    const t0 = Date.now();
    while (Date.now() - t0 < 6000) { s = await S(); if (!s.platsel) return true; await sleep(150); }
    return false;
  }
  async function selectLevel(lvl) {
    let s = await S();
    if (!s.platsel) return false;
    let guard = 0;
    while (s.platpick !== lvl && guard++ < 40) {
      await key(s.platpick < lvl ? 'ArrowRight' : 'ArrowLeft', 170);
      await sleep(120);
      s = await S();
    }
    if (s.platpick !== lvl) return false;
    await sleep(500);
    await key('Space', 250);
    const t0 = Date.now();
    while (Date.now() - t0 < 6000) { s = await S(); if (!s.platsel && s.platlvl === lvl) return true; await sleep(150); }
    return false;
  }

  // ---- leave the mode the player's way: Esc out to the cabinet
  async function exitToCabinet() {
    for (let i = 0; i < 4; i++) {
      const s = await S();
      if (s.bwmode !== 3) return true;
      await key('Escape', 200);
      const t0 = Date.now();
      while (Date.now() - t0 < 3500) { const q = await S(); if (q.bwmode !== 3) return true; await sleep(200); }
    }
    return (await S()).bwmode !== 3;
  }

  // ---- play one level attempt with the reactive policy.
  // Returns {result: 'clear'|'death'|'exit'|'timeout', ...telemetry}
  async function playLevel(lvl, opts = {}) {
    const T = {
      lvl, hits: [], pits: [], stalls: 0, jumps: 0, maxX: 0, vault: 0,
      startHearts: null, startTime: Date.now(), notes: [],
    };
    let s = await S();
    T.startHearts = s.plath;
    await page.keyboard.down('ArrowRight');
    let prev = s, prevY = s.bwy, lastJumpAt = 0, lastProgressX = s.bwx, lastProgressAt = Date.now();
    let groundedSince = 0, vaultTried = false, dead = false, lastSnapAt = Date.now(), stuckShotAt = 0;
    const bonked = new Set();
    const deadline = Date.now() + (opts.timeoutMs || 300000);
    const jump = async (holdMs) => { T.jumps++; lastJumpAt = Date.now(); await page.keyboard.down('Space'); await sleep(holdMs); await page.keyboard.up('Space'); };
    while (Date.now() < deadline) {
      s = await SP();
      if (s.bwmode !== 3) { dead = true; break; }                    // ejected (faint) or escaped
      if (s.platlvl !== lvl) break;                                  // cleared -> next level loaded
      if (s.plath < prev.plath) {                                    // took a hit or a pit
        const wasPit = prev.bwy > 330 || Math.abs(prev.bwx - s.bwx) > 120;
        const rec = { x: Math.round(prev.bwx), from: prev.plath, to: s.plath, pit: !!wasPit };
        (wasPit ? T.pits : T.hits).push(rec);
        log({ ev: 'hurt', lvl, ...rec });
        if (T.hits.filter(h => Math.abs(h.x - rec.x) < 30).length === 2 && !wasPit) await shot('hit-' + lvl + '-' + Math.round(rec.x) + '.png');
      }
      if (prev.bwx - s.bwx > 120 && s.plath === prev.plath) {        // silent respawn (shouldn't happen, hearts drop on pits)
        T.pits.push({ x: Math.round(prev.bwx), silent: true });
      }
      if (s.bwx > T.maxX) T.maxX = s.bwx;
      // progress tracking
      if (s.bwx > lastProgressX + 4) { lastProgressX = s.bwx; lastProgressAt = Date.now(); }
      const now = Date.now();
      if (now - lastSnapAt > 10000) { lastSnapAt = now; log({ ev: 'snap', lvl, x: Math.round(s.bwx), y: Math.round(s.bwy), h: s.plath, t: s.plattime, sub: s.platsub, pow: s.platpow }); }
      if (now - lastProgressAt > 20000 && now - stuckShotAt > 30000) { stuckShotAt = now; log({ ev: 'stuck', lvl, x: Math.round(s.bwx), y: Math.round(s.bwy) }); await shot('stuck-L' + lvl + '-' + Math.round(s.bwx) + '.png'); }
      const falling = s.bwy - prevY > 9;
      const grounded = Math.abs(s.bwy - prevY) <= 2;
      if (grounded) groundedSince += 55; else groundedSince = 0;
      // vault: standing on the gold pipe (feet 216 = ph4 pipe top)
      if (!vaultTried && s.platvaultx >= 0 && !s.platsub && grounded && Math.abs(s.bwy - 216) <= 3
          && s.bwx > s.platvaultx - 20 && s.bwx < s.platvaultx + 70) {
        vaultTried = true;
        await page.keyboard.up('ArrowRight');
        await key('ArrowDown', 220); await sleep(700);
        const sv = await S();
        if (sv.platsub === 1) {
          T.vault = 1; log({ ev: 'vault-in', lvl });
          await shot('vault-' + lvl + '.png');
          // cross the vault floor, then creep onto the exit pipe
          // (tiles 32-33, top feet y=240, centre x=780) and press Down.
          // Running flat-out overshoots onto the wall perch east of it.
          await page.keyboard.down('ArrowRight');
          const vt0 = Date.now();
          while (Date.now() - vt0 < 12000) { const v = await S(); if (!v.platsub || v.bwx > 690) break; await sleep(70); }
          await page.keyboard.up('ArrowRight');
          let exited = false;
          const vt1 = Date.now();
          while (Date.now() - vt1 < 45000) {
            const v = await S();
            if (!v.platsub) { exited = true; break; }
            const onPipe = Math.abs(v.bwy - 240) <= 4 && v.bwx > 750 && v.bwx < 812;
            if (onPipe) {
              await key('ArrowDown', 220); await sleep(700);
              if (!(await S()).platsub) { exited = true; break; }
            }
            // steer: from the floor, run at the pipe and hop on top;
            // at pipe-top height (incl. the wall perch east of it),
            // creep to the pipe centre
            if (v.bwy > 244) {
              const dir = v.bwx < 780 ? 'ArrowRight' : 'ArrowLeft';
              await page.keyboard.down(dir);
              if (Math.abs(v.bwx - 780) < 90) { await page.keyboard.down('Space'); await sleep(280); await page.keyboard.up('Space'); }
              await sleep(110); await page.keyboard.up(dir);
            } else {
              const dir = v.bwx < 774 ? 'ArrowRight' : (v.bwx > 786 ? 'ArrowLeft' : null);
              if (dir) { await page.keyboard.down(dir); await sleep(110); await page.keyboard.up(dir); }
            }
            await sleep(120);
          }
          const sv2 = await S();
          T.vault = sv2.platsub ? -1 : 2; log({ ev: 'vault-out', lvl, ok: !sv2.platsub, exited });
          await page.keyboard.down('ArrowRight');
          s = sv2; prev = sv2; prevY = sv2.bwy;
          continue;
        } else {
          log({ ev: 'vault-miss', lvl, x: Math.round(s.bwx), y: s.bwy });
          await page.keyboard.down('ArrowRight');
        }
      }
      // jump decisions
      if (now - lastJumpAt > 260) {
        const map = mapFor(SEED, lvl);
        const gapWin = s.bwy < 290 ? [10, 60] : [2, 20];   // a full-speed jump flies ~100px (~150 from a height): a 4-wide gap needs a late takeoff
        const gapAhead = grounded && !s.platsub ? map.gaps.find(g => g.x0 - s.bwx > gapWin[0] && g.x0 - s.bwx < gapWin[1]) : null;
        const leapAhead = grounded && !s.platsub ? map.leaps.find(x0 => x0 - s.bwx > 24 && x0 - s.bwx < 74) : null;
        const gapNear = !s.platsub && (map.gaps.some(g => g.x0 - s.bwx > -20 && g.x0 - s.bwx < 140) || map.leaps.some(x0 => x0 - s.bwx > -20 && x0 - s.bwx < 140));
        const foeAhead = grounded && !s.platsub && !gapNear ? map.foes.some(f => f - s.bwx > 26 && f - s.bwx < 68) : false;
        if (gapAhead) { await jump(320); }                                                     // leap the pit from its lip
        else if (leapAhead) { await jump(320); }                                               // take pipes/stairs in stride
        else if (foeAhead) { await jump(250); }                                                // stomp hop
        else if (grounded && !s.platsub && !gapNear && map.bonks.some(b => b - s.bwx > -4 && b - s.bwx < 16 && ![...bonked].some(x => Math.abs(x - b) < 20))) {
          const bx = map.bonks.find(b => b - s.bwx > -4 && b - s.bwx < 16);
          bonked.add(bx); await jump(190);                                                     // bonk the '?' block
        }
        else if (now - lastProgressAt > 330 && grounded) { await jump(300); T.stalls++; }        // walled: pipe/stairs
        else if (falling && now - lastJumpAt > 300) { await jump(280); }                     // coyote save
        else if (groundedSince > 850 && s.bwx > 200 && !gapNear) { await jump(250); groundedSince = 0; } // rhythm hop (also the stomp game: ground contact trades hits)
      }
      prevY = prev.bwy; prev = s;
      await sleep(25);
    }
    await page.keyboard.up('ArrowRight');
    await page.keyboard.up('Space');
    const after = await S();
    T.wallMs = Date.now() - T.startTime;
    if (after.platlvl !== lvl && after.bwmode === 3) {
      T.result = 'clear';
      T.banked = await call('dbg_plattbest', lvl);
      T.medal = await call('dbg_platmedal', lvl);
    } else if (dead || after.bwmode !== 3) {
      T.result = after.faint ? 'death' : 'exit';
    } else T.result = 'timeout';
    T.endHearts = after.plath;
    log({ ev: 'level-end', ...T });
    return T;
  }

  // ================= main =================
  if (fresh) { log({ ev: 'boot-fresh' }); } else log({ ev: 'boot-existing' });
  let s = await S();
  if (s.state === 0) await titleStart();
  s = await S();
  if (fresh) { await call('dbg_adopt', 0); log({ ev: 'adopted', seed: await call('dbg_bwseed') }); }
  else log({ ev: 'seed', seed: await call('dbg_bwseed'), platdone: s.platdone });

  if (mode === 'climb') {
    const toLvl = parseInt(arg || '18', 10);
    let guard = 0, timeoutRetried = -1;
    while (guard++ < 30) {
      s = await S();
      if (s.platdone >= toLvl) { log({ ev: 'climb-done', platdone: s.platdone }); break; }
      if (s.state === 1 && s.faint) { log({ ev: 'climb-abort', at: 'buddy-fainted' }); break; }
      if (!await roadToRun()) { log({ ev: 'climb-abort', at: 'road' }); break; }
      await startPicked();
      s = await S();
      const lvl = s.platlvl;
      await shot('start-L' + lvl + '.png');
      const T = await playLevel(lvl);
      if (T.result === 'death') {
        // a Run death faints the buddy in the Mall; BW stays closed
        // until the player pays the Banzai Buddies revive (30kg of hat
        // for a fresh buddy) — the profile's climb ends here.
        await shot('death-L' + lvl + '.png');
        log({ ev: 'climb-abort', at: 'death', lvl, revivePrice: await call('dbg_budrevive') });
        break;
      } else if (T.result === 'timeout') {
        if (timeoutRetried === lvl) { log({ ev: 'climb-abort', at: 'timeout-twice', lvl }); await exitToCabinet(); break; }
        timeoutRetried = lvl;
        log({ ev: 'climb-note', at: 'timeout-retry', lvl });
        await exitToCabinet();
      } else if (T.result === 'exit') { log({ ev: 'climb-abort', at: 'exit', lvl }); break; }
      else { timeoutRetried = -1; await exitToCabinet(); }   // clear: fresh hearts next entry
    }
  } else if (mode === 'replay') {
    // arg like "1,3 5,2" is passed as separate argv pieces after mode
    const specs = process.argv.slice(4).map(p => p.split(',').map(Number));
    if (!await roadToRun()) { log({ ev: 'replay-abort', at: 'road' }); }
    else {
      for (const [lvl, tries] of specs) {
        for (let t = 0; t < tries; t++) {
          s = await S();
          if (s.bwmode !== 3) { if (!await roadToRun()) break; }
          if ((await S()).platsel) { if (!await selectLevel(lvl)) { log({ ev: 'replay-abort', at: 'select', lvl }); break; } }
          else if ((await S()).platlvl !== lvl) { log({ ev: 'replay-note', msg: 'no select; playing current', lvl: (await S()).platlvl }); }
          const T = await playLevel((await S()).platlvl, { timeoutMs: 240000 });
          if (T.result === 'death') { log({ ev: 'replay-abort', at: 'death' }); break; }
          if (T.result === 'clear') { await exitToCabinet(); if (!await roadToRun()) break; }
        }
      }
    }
  } else if (mode === 'enter') {
    await roadToRun();
    await startPicked();
    await shot('entered.png');
    await sleep(1000);
    log({ ev: 'entered', ...(await S()) });
  } else if (mode === 'probe') {
    // speed probe: hold Right from the level start, sample clock vs x
    if (!await roadToRun()) { log({ ev: 'probe-abort', at: 'road' }); }
    else {
      await startPicked();
      await sleep(300);
      await page.keyboard.down('ArrowRight');
      const t0 = Date.now();
      while (Date.now() - t0 < 14000) {
        const s = await S();
        log({ ev: 'probe', tds: s.plattime, x: Math.round(s.bwx), y: Math.round(s.bwy), lvl: s.platlvl });
        if (s.bwmode !== 3) break;
        await sleep(200);
      }
      await page.keyboard.up('ArrowRight');
      await exitToCabinet();
    }
  }
  await ctx.close();
  log({ ev: 'done' });
  process.exit(0);
})().catch(e => { console.error('ERR', e); try { log({ ev: 'ERR', msg: String(e).slice(0, 300) }); } catch (_) {} process.exit(1); });
