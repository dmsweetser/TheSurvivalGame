// scorchtest.js — CRISPY DIRT (v1.35, Daniel): the Arcade's ninth
// cabinet, a Scorched Earth clone. Buddy catapult vs a bug battery
// on destructible dirt: angle/power/wind artillery, best-of-3
// rounds, a session-only bits purse with a between-rounds shop,
// EASY/NORMAL/HARD paying Buddy XP 30/50/80, the HARD Trophy Case
// plaque, and hot-seat 2P for glory only.
//
// v1.36 (Daniel): the FIREWALL shield (80 bits, strength 100 —
// eats blasts, never falls; carries across rounds; a re-buy is a
// full recharge), 100 starting bits a side, and the shop opening
// BEFORE round 1 (the battery shops silently first, both hot-seat
// players in turn). The pre-shop flows, the absorption / overflow /
// fall-bypass proofs and the AI's shield priorities are covered
// below; the v1.35 checks stand at their updated expectations.
//
// The player path is walked for real throughout: the cabinet
// panel (once by a HUMAN-TIMED touch tap — the v1.34 lesson),
// keyboard aiming, touch gauge drags, touch and mouse FIRE presses
// held like a real finger, the shop's rows, and Esc-Esc forfeits.
// The dbg_scr* seams only stage what a long match would make
// tedious (a bug's HP before a solver-aimed kill, a top-up for the
// player) — every shot asserted on is flown by the game's own
// ballistics, aimed by a JS mirror of the battery's own solver.
// One more seam joined in the v1.36 fix pass: dbg_scrregen restores
// the round's generated field at the ballistic proofs' doors —
// the self-shot proofs dig the player's own column toward the
// world floor by design, and from the bottom of that pit no honest
// arc reaches the far hill, so the later proofs would be aiming
// out of a hole the round never dug.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(120); };
  const poll = async (fn, pred, ms = 12000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const poll2 = async (pred, ms = 30000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await pred(); if (v) return v; await sleep(150); } return v; };
  const saveSlot = n => page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n);
  const pollSave = async (n, needle, ms = 15000) => { const t0 = Date.now(); let s = ''; while (Date.now() - t0 < ms) { s = (await saveSlot(n)) || ''; if (s.includes(needle)) return s; await sleep(300); } return s; };
  // human-timed touch: a real down, a rest on the glass, then up
  const touch = async (x, y, holdMs = 160, moveTo = null) => {
    await page.evaluate(([tx, ty]) => window.__TSG.touch_point(11, tx, ty, 0), [x, y]);
    await sleep(holdMs);
    if (moveTo) { await page.evaluate(([tx, ty]) => window.__TSG.touch_point(11, tx, ty, 1), moveTo); await sleep(140); }
    await page.evaluate(([tx, ty]) => window.__TSG.touch_point(11, tx, ty, 2), [moveTo ? moveTo[0] : x, moveTo ? moveTo[1] : y]);
    await sleep(250);
  };
  const sumH = (x0, x1) => page.evaluate(([a, b]) => { let s = 0; for (let x = a; x <= b; x++) s += window.__TSG.dbg_scrh(x); return s; }, [x0, x1]);

  // The JS mirror of the game's ballistics: same constants, same
  // wind, finer substeps — and, like the game, a shell DIES at the
  // first thing it touches (a unit's disc, the terrain, the edge
  // of the world). Two rankings, because the game integrates with
  // real frame steps and a simulated arc can drift a few pixels
  // by the time it lands:
  //  - robust (the default): the arc must PASS close to the target
  //    centre (minDist) and die within splash reach of it — a pass
  //    through the 14px disc kills the real shell on the disc even
  //    if the arc has drifted, so the hit is sturdy.
  //  - precise (the Big Carcass fall shot): the arc must DIE right
  //    at the target (deathDist), steepest first — the crater has
  //    to open under the unit, not near it.
  let lastScore = 1e9;
  async function solveFor(tx, unitXs, precise) {
    const wind = await call('dbg_scrwind');
    const H = await page.evaluate(() => { const a = []; for (let x = 0; x < 640; x++) a.push(window.__TSG.dbg_scrh(x)); return a; });
    const units = unitXs.map(ux => ({ x: ux, y: H[ux] - 9 }));
    const tgt = units.find(u => u.x === tx) || { x: tx, y: H[tx] - 9 };
    let best = null;
    for (let a = 10; a <= 88; a += 0.5) for (let p = 20; p <= 100; p += 1) {
      const th = a * Math.PI / 180, v = 60 + 3.4 * p;
      let x = 84 + Math.cos(th) * 18, y = (H[84] - 12) - Math.sin(th) * 18;
      let vx = v * Math.cos(th), vy = -v * Math.sin(th);
      let dead = null, minD = 1e18;
      for (let st = 0; st < 900; st++) {
        const h2 = 1 / 120;
        vy += 260 * h2; vx += wind * 1.15 * h2; x += vx * h2; y += vy * h2;
        const dx = x - tgt.x, dy = y - tgt.y, d = dx * dx + dy * dy;
        if (d < minD) minD = d;
        let hit = false;
        for (const u of units) { const ux2 = x - u.x, uy2 = y - u.y; if (ux2 * ux2 + uy2 * uy2 < 196) { hit = true; break; } }
        if (hit || (x >= 0 && x < 640 && y >= H[Math.floor(x)]) || x < -24 || x > 664 || y < -80) { dead = { x, y, disc: hit }; break; }
      }
      if (!dead) continue;
      const dx = dead.x - tgt.x, dy = dead.y - tgt.y;
      const cand = { a, p, minDist: Math.sqrt(minD), deathDist: Math.sqrt(dx * dx + dy * dy), disc: dead.disc ? 1 : 0 };
      // A shell that reaches a unit DIES ON THE DISC'S EDGE (~14
      // from the centre) — so a disc death is a direct hit however
      // the distances read, and for those the pass closeness
      // (minDist) is what ranks the crater's depth.
      if (precise) {
        if (cand.disc && cand.minDist <= 11.5 &&
            (!best || cand.minDist < best.minDist - 0.01 ||
             (Math.abs(cand.minDist - best.minDist) <= 0.01 && cand.a > best.a))) best = cand;
      } else {
        if ((cand.disc || cand.minDist <= 9) && cand.deathDist <= 34 &&
            (!best || cand.minDist + cand.deathDist * 0.25 < best.minDist + best.deathDist * 0.25)) best = cand;
      }
    }
    return best;
  }
  // Fire one solver-aimed shot from side 0 at the unit standing on
  // column tx, then wait for the turn to come back (or the round /
  // match to end). Returns 'turn' | 'shop' | 'end' | 'timeout'.
  async function solverShot(tx, unitXs, precise) {
    await call('dbg_scrhurt', 0, 150);   // staging: the battery's return fire must not end the round mid-assertion (150 — see waitPlayerTurn)
    const sol = await solveFor(tx, unitXs || [tx], precise);
    lastScore = sol ? sol.minDist : 1e9;
    T('solver found a firing solution for x=' + tx, !!sol, sol ? lastScore.toFixed(1) : 'none');
    if (!sol) return 'timeout';
    await call('dbg_scrsetangle', sol.a);
    await call('dbg_scrsetpower', sol.p);
    await key('Space', 200);
    return await poll2(async () => {
      const ph = await call('dbg_scrphase');
      if (ph === 3) return 'shop';
      if (ph === 5) return 'end';
      if (ph === 0 && (await call('dbg_scrturn')) === 0) return 'turn';
      return null;
    }, 45000);
  }
  async function openPanel() {
    await call('dbg_arcade'); await sleep(300);
    await call('dbg_tp', 256, -11758); await sleep(200);
    if ((await call('dbg_cabinet')) !== 8) return false;
    await key('KeyE', 250);
    return (await poll('dbg_arcgate', v => v === 1, 3000)) === 1;
  }
  async function pickRow(r) {   // panel rows: 0 EASY, 1 NORMAL, 2 HARD, 3 2P, 4 LEAVE
    for (let i = 0; i < r; i++) await key('ArrowDown', 160);
    await key('Space', 300);
    return (await poll('dbg_bwmode', v => v === 10, 4000)) === 10;
  }
  const waitAim = () => poll('dbg_scrphase', v => v === 0, 15000);
  // Later rounds open with the OTHER side (the opener alternates),
  // so after a round break the suite waits for the player's own
  // AIM, not just any AIM. The wait keeps both guns topped up
  // while it polls: the opening shot of a round is live fire, and
  // a unit carried into the round wounded can be killed by it
  // before the suite ever gets to stage the exchange (it happened:
  // the battery's round-2 opener buried the player at ~50 HP and
  // the "wait" read as a hang while the match shopped on without
  // us). 150, not 100: a point-blank Big Carcass plus its fall can
  // deal ~134, and the top-up must outlast one full hit.
  const waitPlayerTurn = () => poll2(async () => {
    await call('dbg_scrhurt', 0, 150);
    await call('dbg_scrhurt', 1, 150);
    return (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null;
  }, 45000);

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // ---- the cabinet stands where the spec put it, and nobody moved ----
  await call('dbg_arcade'); await sleep(300);
  T('in the arcade', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11758); await sleep(200);
  T('WAR! is still cabinet 6', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  await call('dbg_tp', -256, -11758); await sleep(200);
  T('Mordor is still cabinet 7', (await call('dbg_cabinet')) === 7, await call('dbg_cabinet'));
  await call('dbg_tp', 256, -11758); await sleep(200);
  T('CRISPY DIRT is cabinet 8 at the east end of the row', (await call('dbg_cabinet')) === 8, await call('dbg_cabinet'));

  // ---- entry by a HUMAN-TIMED touch tap on the panel's EASY row ----
  // Panel (640x360 view): w 320, h 196 -> x 160, y 82; row r centre
  // is y + 52 + r*20 + 5. Row 0 (PLAY - EASY) taps at (320, 139).
  await key('KeyE', 250);
  T('cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await touch(320, 139, 180);
  T('a held touch tap on PLAY - EASY launched the cabinet (bw_mode 10)', (await poll('dbg_bwmode', v => v === 10, 4000)) === 10, await call('dbg_bwmode'));
  // v1.36: the shop opens BEFORE round 1 — exactly 100 bits a side,
  // and the battery has already shopped silently (on EASY it never
  // shields, so its gunner stands bare)
  T('the pre-round shop opens before round 1', (await poll('dbg_scrphase', v => v === 3, 6000)) === 3, await call('dbg_scrphase'));
  T('the opening purse is exactly 100 bits', (await call('dbg_scrbits', 0)) === 100, await call('dbg_scrbits', 0));
  T('EASY battery never shields (its silent pre-shop bought no dome)', (await call('dbg_scrshield', 1)) === 0, await call('dbg_scrshield', 1));
  for (let i = 0; i < 6; i++) await key('ArrowDown', 130);   // row 6: FIREWALL
  await key('Space', 250);
  T('buying the FIREWALL deducted exactly 80', (await call('dbg_scrbits', 0)) === 20, await call('dbg_scrbits', 0));
  T('the shield stands at full strength 100', (await call('dbg_scrshield', 0)) === 100, await call('dbg_scrshield', 0));
  await key('ArrowDown', 140);   // row 7: TO BATTLE
  await key('Space', 300);
  T('the round banner gives way to AIM', (await waitAim()) === 0, await call('dbg_scrphase'));
  T('EASY fields one bug (2 units)', (await call('dbg_scrunits')) === 2, await call('dbg_scrunits'));
  T('the shield rode into round 1 at 100', (await call('dbg_scrshield', 0)) === 100, await call('dbg_scrshield', 0));
  T('round 1, player to open', (await call('dbg_scrround')) === 1 && (await call('dbg_scrturn')) === 0,
    (await call('dbg_scrround')) + '/' + (await call('dbg_scrturn')));
  T('the wind is on the table (-30..30)', Math.abs(await call('dbg_scrwind')) <= 30, await call('dbg_scrwind'));

  // ---- touch: drag the angle gauge, then a held FIRE press ----
  const a0 = await call('dbg_scrangle');
  await touch(152, 338, 130, [214, 338]);   // press mid-gauge, drag right
  const a1 = await call('dbg_scrangle');
  T('dragging the angle gauge moved the angle', a1 > a0 + 20, a0 + ' -> ' + a1);
  await call('dbg_scrsetangle', 85); await call('dbg_scrsetpower', 18);   // a harmless steep lob: the input checks must not wound anyone
  const shots0 = await call('dbg_scrshots');
  await touch(604, 337, 220);   // the FIRE disc, held like a finger
  T('a held touch on FIRE fired exactly once', (await call('dbg_scrshots')) === shots0 + 1, await call('dbg_scrshots'));
  T('the shell flew (phase left AIM)', (await poll('dbg_scrphase', v => v !== 0, 3000)) !== 0, await call('dbg_scrphase'));
  // staging: the battery's answer is live fire — since v1.36's
  // 100-bit purse it owns Big Carcasses, and a point-blank Big
  // plus its fall can deal ~134 — so the wait tops the player up
  // (the waitPlayerTurn idiom) and the exchange cannot end the
  // round mid-suite
  T('the turn came back after the battery answered', (await poll2(async () => {
    await call('dbg_scrhurt', 0, 150);
    return (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null;
  }, 45000)) === 1, await call('dbg_scrturn'));

  // ---- mouse: a real press on FIRE (the shell forwards the mouse
  // as a touch point; the press is held, never same-frame) ----
  await call('dbg_scrsetangle', 85); await call('dbg_scrsetpower', 18);
  const shots1 = await call('dbg_scrshots');
  const box = await page.locator('canvas').boundingBox();
  await page.mouse.move(box.x + 604 / 640 * box.width, box.y + 337 / 360 * box.height);
  await page.mouse.down(); await sleep(200); await page.mouse.up(); await sleep(250);
  T('a held mouse press on FIRE fired exactly once', (await call('dbg_scrshots')) === shots1 + 1, await call('dbg_scrshots'));
  T('the turn came back again', (await poll2(async () => {
    await call('dbg_scrhurt', 0, 150);   // staging, same topped wait as the touch shot
    return (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null;
  }, 45000)) === 1, await call('dbg_scrturn'));

  // ---- keyboard aiming sweeps both gauges ----
  await call('dbg_scrsetangle', 45); await call('dbg_scrsetpower', 60);
  await hold('ArrowLeft', 500);
  T('holding LEFT swept the angle up', (await call('dbg_scrangle')) > 65, await call('dbg_scrangle'));
  await hold('ArrowRight', 250);
  T('holding RIGHT walked it back down', (await call('dbg_scrangle')) < 70, await call('dbg_scrangle'));
  await hold('ArrowUp', 400);
  T('holding UP swept the power up', (await call('dbg_scrpower')) > 75, await call('dbg_scrpower'));
  await hold('ArrowDown', 200);
  T('holding DOWN eased the power off', (await call('dbg_scrpower')) < 80, await call('dbg_scrpower'));

  // ---- v1.36: the FIREWALL eats blasts, not falls ----
  // A carcass fired at OUR OWN column dies on our own disc (or in
  // the dirt at our tracks — the crater opens under us either
  // way); the blast is read at the moment SETTLE opens (blast
  // damage applied, falls not yet), so the dome's share and the
  // HP's share can never blur together.
  async function solveSelf() {
    const wind = await call('dbg_scrwind');
    const H = await page.evaluate(() => { const a = []; for (let x = 0; x < 640; x++) a.push(window.__TSG.dbg_scrh(x)); return a; });
    const units = [{ x: 84, y: H[84] - 9 }, { x: 556, y: H[556] - 9 }];
    const tgt = units[0];
    let best = null, bestScore = 1e18;
    // The scan runs past 90 degrees on purpose: the gauge (and
    // scr_fire) honestly aim to 180, and with a strong tailwind a
    // forward lob can never come back down on our own column —
    // the shot that lands the self-hit then aims back over our
    // own head and lets the wind carry it home.
    for (let a = 10; a <= 170; a += 0.5) for (let p = 20; p <= 100; p += 1) {
      const th = a * Math.PI / 180, v = 60 + 3.4 * p;
      let x = 84 + Math.cos(th) * 18, y = (H[84] - 12) - Math.sin(th) * 18;
      let vx = v * Math.cos(th), vy = -v * Math.sin(th);
      let dead = null, minD = 1e18;
      for (let st = 0; st < 900; st++) {
        const h2 = 1 / 120;
        vy += 260 * h2; vx += wind * 1.15 * h2; x += vx * h2; y += vy * h2;
        const dx = x - tgt.x, dy = y - tgt.y, d = dx * dx + dy * dy;
        if (d < minD) minD = d;
        let hit = false;
        for (const u of units) { const ux2 = x - u.x, uy2 = y - u.y; if (ux2 * ux2 + uy2 * uy2 < 196) { hit = true; break; } }
        if (hit || (x >= 0 && x < 640 && y >= H[Math.floor(x)]) || x < -24 || x > 664 || y < -80) { dead = { x, y, disc: hit }; break; }
      }
      if (!dead) continue;
      const dx = dead.x - tgt.x, dy = dead.y - tgt.y;
      const cand = { a, p, minDist: Math.sqrt(minD), deathDist: Math.sqrt(dx * dx + dy * dy), disc: dead.disc ? 1 : 0 };
      const score = (cand.disc ? 0 : 1000) + cand.minDist + cand.deathDist * 0.25;   // disc deaths rank first (point-blank), then closeness
      if (cand.deathDist <= 34 && score < bestScore) { best = cand; bestScore = score; }
    }
    return best;
  }
  async function selfShot() {
    await call('dbg_scrhurt', 0, 150);   // staging, the solverShot idiom
    const sol = await solveSelf();
    T('solver found a firing solution for the self-shot', !!sol, sol ? sol.minDist.toFixed(1) : 'none');
    if (!sol) return null;
    await call('dbg_scrsetangle', sol.a);
    await call('dbg_scrsetpower', sol.p);
    // Sample densely from BEFORE the press: the blast lands inside
    // FLY and SETTLE follows within a second — the first phase-2
    // sample is the blast's own ledger, taken before the battery
    // can answer and muddy it. (A plain poll after the press
    // missed that window under load and read the battery's settle
    // instead: shield 0, HP 140, both true and both useless.)
    let read = null;
    const t0 = Date.now();
    const sampler = (async () => {
      while (Date.now() - t0 < 20000 && !read) {
        if ((await call('dbg_scrphase')) === 2) { read = { sh: await call('dbg_scrshield', 0), hp: await call('dbg_scrhp', 0) }; break; }
        await sleep(25);
      }
    })();
    await key('Space', 200);
    await sampler;
    return read;
  }
  const waitTurnTopped = () => poll2(async () => {
    await call('dbg_scrhurt', 0, 150);
    return (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null;
  }, 45000);
  // Staging (v1.36 fix): the dome goes into this shot FULL, set
  // explicitly like the sliver and fall stagings below — the
  // bought dome has been eating the battery's live return fire
  // (Big Carcasses since the 100-bit purse) all through the input
  // checks above, and an absorption proof cannot ride on how much
  // of it the battery happened to leave standing.
  await call('dbg_scrsetshield', 0, 100);
  const abs1 = await selfShot();
  T('blast absorbed: the dome ate the whole hit (HP untouched at 150)', !!abs1 && abs1.hp === 150 && abs1.sh > 0 && abs1.sh < 100,
    abs1 ? abs1.sh + '/' + abs1.hp : 'no read');
  T('the turn came back after the absorbed self-shot', (await waitTurnTopped()) === 1, await call('dbg_scrturn'));
  // Overflow: the dome staged to a sliver — the same blast must
  // burn through it and reach the HP underneath.
  await call('dbg_scrsetshield', 0, 5);
  const abs2 = await selfShot();
  T('overflow reaches HP: a 5-strength dome burns through', !!abs2 && abs2.sh === 0 && abs2.hp < 150 && abs2.hp > 60,
    abs2 ? abs2.sh + '/' + abs2.hp : 'no read');
  T('the turn came back after the overflow self-shot', (await waitTurnTopped()) === 1, await call('dbg_scrturn'));
  // Fall bypass: a full dome, a pit dug under our own catapult,
  // and a solver shot at the bug purely to run the settle. The
  // carve drops the ground exactly 30 at our column, so the fall
  // must cost (30-14)*1.2 = 19 HP and the dome nothing. (The pit
  // is dug AFTER the solve: until the settle runs, the unit still
  // stands at the old height the solver aimed from.)
  // Staging: the proof's arithmetic (an exact 30-carve, an exact
  // 19 HP) only holds on the field the round generated — by now
  // the self-shots and the return fire have dug our column most
  // of the way to the world floor, where the carve clamps and the
  // floor itself kills. Restore the round's field first.
  await call('dbg_scrregen');
  await call('dbg_scrsetshield', 0, 100);
  await call('dbg_scrhurt', 0, 150);
  await call('dbg_scrhurt', 1, 500);   // staging: the bug must survive the shot that runs the settle
  // The shot's only job is to run a settle — what it lands on is
  // immaterial to the fall's arithmetic (an exact 30-carve under
  // our own tracks). Aim at the bug first; if the self-dug pit at
  // our column has eaten every arc that reaches it (it happens —
  // the pit's depth rides on where the return fire landed), fall
  // back to a solver shot at the mid-field valley, just as real
  // and just as far from both units.
  let solF = await solveFor(556, [84, 556]);
  if (!solF) solF = await solveFor(320, [84, 556]);
  T('solver found the fall-test shot', !!solF, solF ? solF.minDist.toFixed(1) : 'none');
  if (!solF) { console.log(`scorchtest: ${pass} passed, ${fail} failed (aborted at the fall test — no solver shot)`); await browser.close(); process.exit(1); }
  await call('dbg_scrsetangle', solF.a); await call('dbg_scrsetpower', solF.p);
  await call('dbg_scrdig', 84);
  await key('Space', 200);
  const fallRead = await poll2(async () => (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 1
    ? { hp: await call('dbg_scrhp', 0), sh: await call('dbg_scrshield', 0) } : null, 30000);
  T('the fall hurt HP through the full shield (150 -> 131)', !!fallRead && fallRead.hp === 131, fallRead ? fallRead.hp : 'no read');
  T('the shield never felt the fall (still 100)', !!fallRead && fallRead.sh === 100, fallRead ? fallRead.sh : 'no read');
  T('the turn came back after the fall test', (await poll2(async () => {
    await call('dbg_scrhurt', 0, 150); await call('dbg_scrhurt', 1, 150);
    return (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null;
  }, 45000)) === 1, await call('dbg_scrturn'));

  // ---- a direct hit: blast, crater, and the hit bounty ----
  // Staging: the round's field restored (see the fall proof) —
  // the solver aims from the launch hill the round generated,
  // not from the bottom of the self-dug pit, out of which no
  // honest arc reaches the far hill at all.
  await call('dbg_scrregen');
  await call('dbg_scrhurt', 1, 100);   // staging: the bug starts this exchange whole, whatever the lob practice cost it
  const hBugBefore = await call('dbg_scrh', 556);
  const sumBefore = await sumH(0, 639);
  const bitsBefore = await call('dbg_scrbits', 0);
  const r1 = await solverShot(556, [84, 556]);
  T('the first solver shot resolved back to a live turn', r1 === 'turn', r1);
  const hpBug = await call('dbg_scrhp', 1);
  T('a solver-true carcass hit hurt the bug hard', hpBug <= 54, hpBug);
  T('the crater dropped the ground under the bug', (await call('dbg_scrh', 556)) >= hBugBefore + 4,
    hBugBefore + ' -> ' + (await call('dbg_scrh', 556)));
  T('the shot carved the field (total dirt fell)', (await sumH(0, 639)) > sumBefore, sumBefore + ' -> ' + (await sumH(0, 639)));
  T('the hit bounty paid (+10 bits)', (await call('dbg_scrbits', 0)) >= bitsBefore + 10, await call('dbg_scrbits', 0));

  // ---- the kill, the round, and the shop's exact prices ----
  await call('dbg_scrregen');   // staging: the round's field restored, same as the direct-hit proof
  await call('dbg_scrhurt', 1, 30);   // staging: the kill shot's verdict must not ride on splash arithmetic
  await call('dbg_scrsetshield', 0, 53);   // staging (v1.36): the round's wear on the dome, so the carry into the shop is a number worth asserting
  const bitsKillBefore = await call('dbg_scrbits', 0);
  const r2 = await solverShot(556, [84, 556]);
  T('the killing shot ended the round into the shop', r2 === 'shop', r2);
  T('round 1 is the player\'s', (await call('dbg_scrwins', 0)) === 1, await call('dbg_scrwins', 0));
  T('the bug is dead', (await call('dbg_scralive', 1)) === 0, await call('dbg_scralive', 1));
  const shopBits = await call('dbg_scrbits', 0);
  // The two purses, measured against the purse the section
  // started with (v1.36: the pre-shop FIREWALL spend makes any
  // absolute total ride on which earlier shots happened to clip
  // the bug — the kill's +50 and the round's +100 are the claim)
  T('kill + round purses landed (+150 or more)', shopBits >= bitsKillBefore + 150, bitsKillBefore + ' -> ' + shopBits);
  // v1.36: the dome carried across the round break at its worn
  // strength — and the FIREWALL row recharges it, exactly
  T('the worn shield carried into the shop at 53', (await call('dbg_scrshield', 0)) === 53, await call('dbg_scrshield', 0));
  for (let i = 0; i < 6; i++) await key('ArrowDown', 130);   // row 6: FIREWALL
  await key('Space', 250);
  T('recharging the FIREWALL restored exactly 100', (await call('dbg_scrshield', 0)) === 100, await call('dbg_scrshield', 0));
  T('the recharge cost exactly 80', (await call('dbg_scrbits', 0)) === shopBits - 80, await call('dbg_scrbits', 0));
  const bitsFW = await call('dbg_scrbits', 0);
  for (let i = 0; i < 5; i++) await key('ArrowUp', 130);   // back up to row 1: BIG CARCASS, 40 bits
  await key('Space', 250);
  T('buying a Big Carcass stocked it', (await call('dbg_scrinv', 0, 2)) === 1, await call('dbg_scrinv', 0, 2));
  T('the Big Carcass cost exactly 40', (await call('dbg_scrbits', 0)) === bitsFW - 40, (await call('dbg_scrbits', 0)));
  const bits2 = await call('dbg_scrbits', 0);
  await key('ArrowDown', 160);   // row 2: DIRT CLOD, 30 bits
  await key('Space', 250);
  T('buying a Dirt Clod stocked it', (await call('dbg_scrinv', 0, 3)) === 1, await call('dbg_scrinv', 0, 3));
  T('the Dirt Clod cost exactly 30', (await call('dbg_scrbits', 0)) === bits2 - 30, await call('dbg_scrbits', 0));
  for (let i = 0; i < 5; i++) await key('ArrowDown', 140);   // row 7: TO BATTLE
  await key('Space', 300);
  T('TO BATTLE opened round 2', (await poll('dbg_scrround', v => v === 2, 6000)) === 2, await call('dbg_scrround'));
  T('round 2 opens under the recharged dome (100)', (await call('dbg_scrshield', 0)) === 100, await call('dbg_scrshield', 0));
  T('round 2 reaches the player\'s turn on fresh hills', (await waitPlayerTurn()) === 1, await call('dbg_scrturn'));
  await call('dbg_scrhurt', 1, 100);   // undo the wait's protective top-up: the freshness assertion wants the honest number
  T('fresh hills, fresh bug (100 HP)', (await call('dbg_scrhp', 1)) === 100, await call('dbg_scrhp', 1));

  // ---- FALL DAMAGE, isolated: the Big Carcass point-blank ----
  // A direct hit dies on the bug's disc edge, ~14 from its centre:
  // blast there is 125 * (1 - 0.6*14/42) = 100 at most, so a bug
  // staged to 200 HP that comes out under 84 took 16+ of the rest
  // from the fall — its crater floor drops ~31, well past the
  // 14px safe drop. (A plain Carcass cannot isolate this: its
  // crater under a standing unit is shallower than its own blast
  // reach, which is honest physics.)
  await key('Digit3', 200);   // weapon key 3 -> weapon 2 (BIG CARCASS)
  T('the digit key selected the Big Carcass', (await call('dbg_scrweap')) === 2, await call('dbg_scrweap'));
  await call('dbg_scrhurt', 1, 200);
  await call('dbg_scrhurt', 0, 100);
  const hBigBefore = await call('dbg_scrh', 556);
  const solBig = await solveFor(556, [84, 556], true);
  T('solver found the Big firing solution', !!solBig, solBig ? solBig.minDist.toFixed(1) : 'none');
  await call('dbg_scrsetangle', solBig.a); await call('dbg_scrsetpower', solBig.p);
  await key('Space', 200);
  // Sample the exchange: the blast lands when SETTLE begins, the
  // fall damage lands as its own step while the unit drops — the
  // two must be separable in the HP trace, and the explosion the
  // game reports must predict the fall by the carve formula.
  let hpBlast = -1, hpFinal = -1, exB = -1, eyB = -1;
  {
    const t0 = Date.now();
    while (Date.now() - t0 < 30000) {
      const ph = await call('dbg_scrphase');
      if (ph === 2 && hpBlast < 0) {
        hpBlast = await call('dbg_scrhp', 1);
        exB = await call('dbg_scrlastx'); eyB = await call('dbg_scrlasty');
      }
      if (ph === 0 && (await call('dbg_scrturn')) === 0) { hpFinal = await call('dbg_scrhp', 1); break; }
      await sleep(100);
    }
  }
  const hBigAfter = await call('dbg_scrh', 556);
  T('the Big blast landed (HP stepped down in SETTLE)', hpBlast > 40 && hpBlast < 200, hpBlast);
  T('the bug survived the Big blast (staged deep)', hpFinal > 40 && (await call('dbg_scralive', 1)) === 1, hpFinal);
  {
    // The fall the geometry promises: crater bottom at the bug's
    // column from the reported explosion centre, minus the ground
    // it stood on; past the 14px safe drop, 1.2 damage per pixel.
    const dxB = 556 - exB;
    const bottomB = Math.abs(dxB) < 42 ? eyB + Math.sqrt(42 * 42 - dxB * dxB) : hBigBefore;
    const fellB = bottomB - hBigBefore;
    const fallDmgB = fellB > 14 ? (fellB - 14) * 1.2 : 0;
    T('fall damage rode the blast as its own step', hpFinal <= hpBlast - Math.min(fallDmgB, 6) + 1 && hpFinal < hpBlast,
      'blast hp ' + hpBlast + ' -> final ' + hpFinal + ' (explosion ' + exB + ',' + eyB + ', fell ' + fellB.toFixed(1) + ')');
    T('the Big crater dropped the bug\'s ground by 18+', hBigAfter >= hBigBefore + 18,
      hBigBefore + ' -> ' + hBigAfter);
  }

  // ---- the Dirt Clod ADDS dirt ----
  await key('Digit4', 200);   // weapon key 4 -> weapon 3 (DIRT CLOD)
  T('the digit key selected the Dirt Clod', (await call('dbg_scrweap')) === 3, await call('dbg_scrweap'));
  const winBefore = await sumH(526, 586);
  const rd = await solverShot(556, [84, 556]);
  T('the clod shot resolved', rd === 'turn' || rd === 'shop', rd);
  T('the clod raised the dirt around its landing', (await sumH(526, 586)) < winBefore - 60,
    winBefore + ' -> ' + (await sumH(526, 586)));
  await key('Digit1', 200);
  T('the digit key selected the plain Carcass back', (await call('dbg_scrweap')) === 0, await call('dbg_scrweap'));

  // ---- win the match on EASY: XP pays exactly 30, once ----
  // Staging: the round's field restored — the Dirt Clod proof
  // just heaped a mound over the bug's column, and a unit buried
  // under its own hill is honestly hard to hit (the mirror aims
  // at the surface; the bug stands underneath it). The claim here
  // is the match end and the XP, not splash arithmetic through
  // twenty feet of fresh dirt.
  await call('dbg_scrregen');
  await call('dbg_scrhurt', 1, 20);
  const xpBefore = await call('dbg_buddyxp');
  const r3 = await solverShot(556, [84, 556]);
  T('the second round win ended the match', r3 === 'end', r3);
  T('the match is the player\'s, 2 rounds', (await call('dbg_scrwins', 0)) === 2, await call('dbg_scrwins', 0));
  T('EASY paid exactly +30 Buddy XP at the match\'s end', (await call('dbg_buddyxp')) === xpBefore + 30,
    xpBefore + ' -> ' + (await call('dbg_buddyxp')));
  T('no trophy for EASY', (await call('dbg_scrchamp')) === 0, await call('dbg_scrchamp'));
  await key('Space', 300);
  T('leaving the end panel returns to the arcade', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));

  // ---- hot-seat 2P: turns alternate, and glory pays no XP ----
  T('panel reopens for 2P', await openPanel(), await call('dbg_arcgate'));
  T('2P HOT-SEAT launched', await pickRow(3), await call('dbg_bwmode'));
  // v1.36: both players shop before round 1, in turn, 100 each —
  // P1 stocks a Big Carcass, P2 raises a dome, and neither
  // purchase may touch the other side's purse or stock
  T('the 2P pre-round shop opens (P1 first)', (await poll('dbg_scrphase', v => v === 3, 6000)) === 3, await call('dbg_scrphase'));
  T('both purses start at exactly 100', (await call('dbg_scrbits', 0)) === 100 && (await call('dbg_scrbits', 1)) === 100,
    (await call('dbg_scrbits', 0)) + '/' + (await call('dbg_scrbits', 1)));
  await key('ArrowDown', 140);   // P1 -> row 1: BIG CARCASS
  await key('Space', 250);
  T('P1 bought a Big Carcass in the pre-shop (100 -> 60, stocked)', (await call('dbg_scrbits', 0)) === 60 && (await call('dbg_scrinv', 0, 2)) === 1,
    (await call('dbg_scrbits', 0)) + '/' + (await call('dbg_scrinv', 0, 2)));
  for (let i = 0; i < 6; i++) await key('ArrowDown', 130);   // row 7: TO BATTLE -> P2 shops
  await key('Space', 250);
  T('the pre-round shop passes to P2 (still shopping)', (await call('dbg_scrphase')) === 3, await call('dbg_scrphase'));
  for (let i = 0; i < 6; i++) await key('ArrowDown', 130);   // P2 -> row 6: FIREWALL
  await key('Space', 250);
  T('P2 bought the FIREWALL (100 -> 20, dome up at 100)', (await call('dbg_scrbits', 1)) === 20 && (await call('dbg_scrshield', 1)) === 100,
    (await call('dbg_scrbits', 1)) + '/' + (await call('dbg_scrshield', 1)));
  T('P2\'s purchase left P1 untouched (purse 60, Big stocked, no dome)', (await call('dbg_scrbits', 0)) === 60 && (await call('dbg_scrinv', 0, 2)) === 1 && (await call('dbg_scrshield', 0)) === 0,
    (await call('dbg_scrbits', 0)) + '/' + (await call('dbg_scrshield', 0)));
  await key('ArrowDown', 140);   // row 7: TO BATTLE
  await key('Space', 300);
  T('2P gives way to AIM', (await waitAim()) === 0, await call('dbg_scrphase'));
  T('2P fields two catapults', (await call('dbg_scrunits')) === 2, await call('dbg_scrunits'));
  T('the pre-bought Big Carcass is still stocked as round 1 begins', (await call('dbg_scrinv', 0, 2)) === 1, await call('dbg_scrinv', 0, 2));
  const xp2p = await call('dbg_buddyxp');
  await call('dbg_scrhurt', 0, 100);
  const s2p = await solveFor(556, [84, 556]);
  T('solver found the 2P opening solution', !!s2p, s2p ? s2p.minDist.toFixed(1) : 'none');
  await call('dbg_scrsetangle', s2p ? s2p.a : 45); await call('dbg_scrsetpower', s2p ? s2p.p : 62);
  await key('Space', 200);
  T('P1\'s shot passed the turn to P2', (await poll2(async () =>
    (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 1 ? 1 : null, 30000)) === 1, await call('dbg_scrturn'));
  // v1.36: P2's dome took the opening hit — HP whole, shield spent down
  T('P2\'s dome took the hit and P2 lived whole', (await call('dbg_scrhp', 1)) === 100 && (await call('dbg_scrshield', 1)) > 0 && (await call('dbg_scrshield', 1)) < 100 && (await call('dbg_scralive', 1)) === 1,
    (await call('dbg_scrhp', 1)) + '/' + (await call('dbg_scrshield', 1)));
  await call('dbg_scrhurt', 0, 100);   // staging: P2's answer must not end the round either
  await call('dbg_scrsetangle', 45); await call('dbg_scrsetpower', 62);
  await key('Space', 200);
  T('P2\'s shot passed the turn back to P1', (await poll2(async () =>
    (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 0 ? 1 : null, 30000)) === 1, await call('dbg_scrturn'));
  // finish the 2P match in two staged rounds
  for (let round = 0; round < 2; round++) {
    await call('dbg_scrhurt', 1, 5);
    await call('dbg_scrsetshield', 1, 0);   // staging (v1.36): the kill must not ride on whatever dome P2 has left
    const rr = await solverShot(556, [84, 556]);
    if (round === 0) {
      T('2P round 1 ended into the shop', rr === 'shop', rr);
      for (let i = 0; i < 7; i++) await key('ArrowDown', 130);
      await key('Space', 250);   // P1: TO BATTLE -> P2 shops
      T('the 2P shop passes to P2 (still shopping)', (await call('dbg_scrphase')) === 3, await call('dbg_scrphase'));
      for (let i = 0; i < 7; i++) await key('ArrowDown', 130);
      await key('Space', 250);   // P2: TO BATTLE -> round 2
      T('2P round 2 opened', (await poll('dbg_scrround', v => v === 2, 6000)) === 2, await call('dbg_scrround'));
      // Round 2 opens with P2 (the opener alternates): P2 lobs a
      // harmless steep shot, then P1 is up.
      T('2P round 2 opens on P2\'s turn', (await poll2(async () =>
        (await call('dbg_scrphase')) === 0 && (await call('dbg_scrturn')) === 1 ? 1 : null, 20000)) === 1, await call('dbg_scrturn'));
      await call('dbg_scrsetangle', 85); await call('dbg_scrsetpower', 18);
      await key('Space', 200);
      T('2P round 2 reaches P1\'s turn', (await waitPlayerTurn()) === 1, await call('dbg_scrturn'));
    } else {
      T('2P match ended', rr === 'end', rr);
    }
  }
  T('2P paid no XP at all', (await call('dbg_buddyxp')) === xp2p, xp2p + ' -> ' + (await call('dbg_buddyxp')));
  T('2P earned no trophy', (await call('dbg_scrchamp')) === 0, await call('dbg_scrchamp'));
  await key('Space', 300);
  T('back in the arcade after 2P', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));

  // ---- HARD: three bugs, and the plaque ----
  T('panel reopens for HARD', await openPanel(), await call('dbg_arcgate'));
  T('PLAY - HARD launched', await pickRow(2), await call('dbg_bwmode'));
  // v1.36: the battery shopped silently before round 1 — on HARD
  // the Firewall is its first buy, so its gunner opens domed
  T('the HARD pre-round shop opens', (await poll('dbg_scrphase', v => v === 3, 6000)) === 3, await call('dbg_scrphase'));
  T('HARD battery opens round 1 shielded (gunner dome at 100)', (await call('dbg_scrshield', 1)) === 100, await call('dbg_scrshield', 1));
  for (let i = 0; i < 7; i++) await key('ArrowDown', 130);   // buy nothing: TO BATTLE
  await key('Space', 300);
  T('HARD gives way to AIM', (await waitAim()) === 0, await call('dbg_scrphase'));
  T('HARD fields three bugs (4 units)', (await call('dbg_scrunits')) === 4, await call('dbg_scrunits'));
  const BUGX = [556, 492, 436];
  for (let round = 0; round < 2; round++) {
    for (let bi = 0; bi < 3; bi++) {
      await call('dbg_scrhurt', bi + 1, 1);
      await call('dbg_scrsetshield', bi + 1, 0);   // staging (v1.36): the staged kill must not ride on the battery's domes (it re-shields between rounds)
      const rr = await solverShot(BUGX[bi], [84, 556, 492, 436]);
      T('HARD round ' + (round + 1) + ': bug ' + (bi + 1) + ' down', (await call('dbg_scralive', bi + 1)) === 0,
        await call('dbg_scralive', bi + 1));
      if (bi < 2) T('HARD round ' + (round + 1) + ': the fight goes on', rr === 'turn', rr);
      else if (round === 0) {
        T('HARD round 1 ended into the shop', rr === 'shop', rr);
        for (let i = 0; i < 7; i++) await key('ArrowDown', 130);
        await key('Space', 250);
        T('HARD round 2 opened', (await poll('dbg_scrround', v => v === 2, 6000)) === 2, await call('dbg_scrround'));
        T('HARD round 2 reaches the player\'s turn', (await waitPlayerTurn()) === 1, await call('dbg_scrturn'));
      } else {
        T('HARD match ended', rr === 'end', rr);
      }
    }
  }
  T('the HARD win set the champion flag', (await call('dbg_scrchamp')) === 1, await call('dbg_scrchamp'));
  const blobH = await pollSave(1, 'sc=1');
  T('the sc=1 token reached the save', blobH.includes('sc=1'), blobH.slice(0, 80));
  await key('Space', 300);
  T('back in the arcade after HARD', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));

  // ---- leaving mid-match forfeits; re-entry starts clean ----
  // (v1.36: every entry lands in the pre-round shop first — the
  // skip helper walks straight to TO BATTLE, buying nothing)
  async function skipPreShop() {
    if ((await poll('dbg_scrphase', v => v === 3, 6000)) !== 3) return false;
    for (let i = 0; i < 7; i++) await key('ArrowDown', 130);
    await key('Space', 300);
    return true;
  }
  T('panel reopens for the forfeit check', await openPanel(), await call('dbg_arcgate'));
  T('EASY launched again', await pickRow(0), await call('dbg_bwmode'));
  T('the pre-round shop opens for the forfeit check', await skipPreShop(), await call('dbg_scrphase'));
  T('AIM again', (await waitAim()) === 0, await call('dbg_scrphase'));
  await call('dbg_scrgivebits', 0, 500);
  T('staging purse in place', (await call('dbg_scrbits', 0)) === 600, await call('dbg_scrbits', 0));
  await key('Escape', 250);
  T('one Esc only arms the forfeit (still in the match)', (await call('dbg_bwmode')) === 10, await call('dbg_bwmode'));
  await key('Escape', 250);
  T('the second Esc forfeits back to the arcade', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));
  T('panel reopens after the forfeit', await openPanel(), await call('dbg_arcgate'));
  T('EASY relaunched', await pickRow(0), await call('dbg_bwmode'));
  T('the pre-round shop opens after re-entry', await skipPreShop(), await call('dbg_scrphase'));
  T('AIM after re-entry', (await waitAim()) === 0, await call('dbg_scrphase'));
  T('re-entry reset the purse to 100', (await call('dbg_scrbits', 0)) === 100, await call('dbg_scrbits', 0));
  T('leaving the pre-shop with no purchase started round 1 clean (no stock, no dome)',
    (await call('dbg_scrinv', 0, 2)) === 0 && (await call('dbg_scrshield', 0)) === 0,
    (await call('dbg_scrinv', 0, 2)) + '/' + (await call('dbg_scrshield', 0)));
  T('re-entry reset the match (round 1, no wins)', (await call('dbg_scrround')) === 1 && (await call('dbg_scrwins', 0)) === 0,
    (await call('dbg_scrround')) + '/' + (await call('dbg_scrwins', 0)));
  await page.screenshot({ path: 'scorch-aim.png' });
  await key('Escape', 200); await key('Escape', 250);

  console.log(`scorchtest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
