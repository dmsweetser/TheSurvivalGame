// tanktest.js — TANK RESCUE progression (Nov 11, Daniel: progression
// was confusing and the tank too strong). The mission chain must read
// in order: RESCUE (objective 0) -> DELIVER (aboard, objective 1) ->
// quota met -> BREAK THE BLOCKADE (objective 2) -> the mid-field
// barrier only opens once the quota is earned AND the blockade gun is
// dead -> DESTROY THE CORE (objective 3) -> CROSS THE LINE (4) ->
// level 2 loads and the record (tank_done) moves to 1.
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
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // straight into the arcade, then the tank cabinet. Two-row layout
  // (Nov 14): TANK RESCUE is island-row west — its interact point is
  // (ARC_SX[3], ARC_CY + ARC_ROWY[3] + 58) = (-256, -11898). The old
  // single-row spot (219, -12108) now lands on BUDDY CRAFT's point.
  await call('dbg_arcade'); await sleep(300);
  T('in the arcade', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', -256, -11898); await sleep(200);
  T('standing at the tank cabinet', (await call('dbg_cabinet')) === 3, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('tank cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('TANK RESCUE launched (bw_mode 5)', (await poll('dbg_bwmode', v => v === 5, 4000)) === 5, await call('dbg_bwmode'));
  T('level 1', (await call('dbg_tanklvl')) === 1, await call('dbg_tanklvl'));

  // GRENADES (Nov 14): a switchable secondary weapon. Cannon is the
  // default; F swaps to the pouch with 8 grenades; firing lobs one
  // (flight shows in the boom counter at +10) and it bursts ahead.
  T('cannon is the default weapon', (await call('dbg_tankwep')) === 0, await call('dbg_tankwep'));
  T('grenade pouch starts full (8)', (await call('dbg_tankgren')) === 8, await call('dbg_tankgren'));
  await key('KeyF', 250);
  T('F swaps to grenades', (await poll('dbg_tankwep', v => v === 1, 3000)) === 1, await call('dbg_tankwep'));
  await hold('Space', 700);
  T('a grenade was lobbed (pouch down, shell in flight or burst)',
    (await poll('dbg_tankboom', v => v > 0, 3000)) > 0 && (await call('dbg_tankgren')) < 8,
    'boom=' + (await call('dbg_tankboom')) + ' gren=' + (await call('dbg_tankgren')));
  await sleep(1200);
  T('the grenade burst (nothing left flying)', (await call('dbg_tankboom')) < 10, await call('dbg_tankboom'));
  await key('KeyF', 250);
  T('F swaps back to the cannon', (await poll('dbg_tankwep', v => v === 0, 3000)) === 0, await call('dbg_tankwep'));

  // POWERUPS (v1.27, Daniel: the tank game is too hard): a HEALTH
  // medkit (entity kind 6) is guaranteed a short drive north of the
  // spawn — placement band ty TANK_LEN-14..TANK_LEN-8, so its y sits
  // in [96*48+24, 102*48+24] = [4632, 4920] — and a GRENADE LAUNCHER
  // (kind 7) waits in the mid-field south of the blockade (band ty
  // TANK_MID_TY+6..TANK_LEN-24, y in [52*48+24, 86*48+24] = [2520,
  // 4152]). Observed through the dbg_tankpick/x/y seams.
  let tankReentered = 0;   // set by the boss-fight re-entry branch far below; the launcher-persistence check at the end reads it
  T('a health medkit is placed in level 1', (await call('dbg_tankpick', 6)) >= 1, await call('dbg_tankpick', 6));
  const medY = await call('dbg_tankpicky', 6);
  T('...in the early band north of the spawn', medY >= 4632 && medY <= 4920, 'y=' + medY);
  T('a grenade launcher pickup is placed in level 1', (await call('dbg_tankpick', 7)) >= 1, await call('dbg_tankpick', 7));
  const lauY = await call('dbg_tankpicky', 7);
  T('...in the mid-field band', lauY >= 2520 && lauY <= 4152, 'y=' + lauY);

  // stage real damage: park south of the mid-field barrier, inside
  // the blockade gun's range — its shells fly clean over the barrier
  // (the same staging the barrier checks below rely on)
  await call('dbg_tankwarp', 2350); await sleep(200);
  const hullHit = await poll('dbg_tankhull', v => v < 100, 30000);
  T('enemy fire bloodies the tank (hull < 100)', hullHit < 100, 'hull=' + hullHit);

  // drive-over helper: the warp puts the tank on the spine (x 600)
  // at the pickup's own row, then pulse-steer onto it (placement
  // guarantees the row between the spine and the pickup is open;
  // the count-drop poll is the verify, the steering retries)
  const driveOver = async (kind) => {
    const py = await call('dbg_tankpicky', kind), px = await call('dbg_tankpickx', kind);
    const n0 = await call('dbg_tankpick', kind);
    if (py < 0 || n0 === 0) return 0;
    await call('dbg_tankwarp', py); await sleep(150);
    const t0 = Date.now();
    while (Date.now() - t0 < 9000) {
      if ((await call('dbg_tankpick', kind)) < n0) return 1;
      const cx = await call('dbg_tankx'), cy = await call('dbg_tanky');
      const k2 = Math.abs(cx - px) > 12 ? (px < cx ? 'ArrowLeft' : 'ArrowRight')
               : Math.abs(cy - py) > 12 ? (py < cy ? 'ArrowUp' : 'ArrowDown') : null;
      if (!k2) { await sleep(100); continue; }
      await page.keyboard.down(k2); await sleep(130); await page.keyboard.up(k2);
      await sleep(60);
    }
    return (await call('dbg_tankpick', kind)) < n0 ? 1 : 0;
  };

  const hullBefore = await call('dbg_tankhull');
  T('the medkit is collected by driving over it', (await driveOver(6)) === 1);
  const hullAfter = await call('dbg_tankhull');
  T('the medkit restores +50 hull (a stray hit during the drive aside)',
    hullAfter === 100 || hullAfter >= hullBefore + 32, hullBefore + ' -> ' + hullAfter);

  // the launcher: collect it, then prove the pouch is bottomless —
  // fire MORE grenades than the pouch holds, one confirmed lob at a
  // time, and watch the count never move. Firing happens from the
  // spawn apron facing north: the spine column (tx 12) is open by
  // construction, so every lob flies its full 248px and bursts far
  // from the tank (no splash self-harm in the count).
  T('the grenade launcher is collected by driving over it', (await driveOver(7)) === 1);
  T('...and the launcher state is set', (await poll('dbg_tankglauncher', v => v === 1, 3000)) === 1, await call('dbg_tankglauncher'));
  await call('dbg_tankwarp', 5160); await hold('ArrowUp', 300); await page.keyboard.up('ArrowUp');
  if ((await call('dbg_tankwep')) !== 1) { await key('KeyF', 250); await poll('dbg_tankwep', v => v === 1, 3000); }
  const g0 = await call('dbg_tankgren');
  let lobbed = 0; const ft0 = Date.now();
  while (lobbed < g0 + 3 && Date.now() - ft0 < 90000) {
    await hold('Space', 300);
    const seen = await poll('dbg_tankboom', v => v > 0, 2000);
    if (seen > 0) { lobbed++; await poll('dbg_tankboom', v => v === 0, 2500); }
    else await sleep(400);   // no lob that press (dead tank, cooldown) — retry, don't count it
  }
  T('launcher: lobbed more grenades than the pouch holds (' + (g0 + 3) + ' > ' + g0 + ')', lobbed === g0 + 3, 'lobbed=' + lobbed);
  T('...and the pouch count never dropped', (await call('dbg_tankgren')) === g0, 'gren=' + (await call('dbg_tankgren')) + ' g0=' + g0);
  await key('KeyF', 250);
  T('F swaps back to the cannon (post-launcher)', (await poll('dbg_tankwep', v => v === 0, 3000)) === 0, await call('dbg_tankwep'));

  // the mission chain opens on RESCUE, nothing delivered, gun alive
  T('objective opens on RESCUE (0)', (await call('dbg_tankobj')) === 0, await call('dbg_tankobj'));
  T('nothing delivered yet', (await call('dbg_tankdeliv')) === 0, await call('dbg_tankdeliv'));
  T('blockade gun alive', (await call('dbg_tankmid')) > 0, await call('dbg_tankmid'));

  // the mid-field barrier (gate row ty 46, y 2208..2255) is SHUT while
  // the quota is unearned: warp just south of it and drive north
  await call('dbg_tankwarp', 2350); await sleep(200);
  await hold('ArrowUp', 2500);
  const gy = await call('dbg_tanky');
  T('barrier holds the tank while the quota is unearned', gy > 2240, 'y=' + gy);

  // earn the quota (debug grant): the objective must swing to the
  // blockade, and the barrier must STILL hold while the gun lives
  await call('dbg_tankquota');
  T('objective swings to BREAK THE BLOCKADE (2)', (await poll('dbg_tankobj', v => v === 2, 4000)) === 2, await call('dbg_tankobj'));
  await hold('ArrowUp', 1500);
  const gy2 = await call('dbg_tanky');
  T('barrier still holds while the blockade gun lives', gy2 > 2240, 'y=' + gy2);

  // shoot the gun out from south of the barrier (shells fly over it):
  // facing is already north from the driving above
  await page.keyboard.down('Space');
  const mid = await poll('dbg_tankmid', v => v === 0, 60000);
  await page.keyboard.up('Space');
  T('blockade gun destroyed by fire over the barrier', mid === 0, 'mid=' + mid);
  T('objective swings to the FORTRESS CORE (3)', (await poll('dbg_tankobj', v => v === 3, 4000)) === 3, await call('dbg_tankobj'));

  // with quota + dead gun the barrier is open: drive through it
  // (wiggle while driving — a patrol crossing the road is a fair block
  // for a player, who would steer around it)
  { const s0 = Date.now();
    await page.keyboard.down('ArrowUp');
    while (Date.now() - s0 < 9000) { if ((await call('dbg_tanky')) < 2100) break;
      await page.keyboard.down('ArrowLeft'); await sleep(220); await page.keyboard.up('ArrowLeft');
      await page.keyboard.down('ArrowRight'); await sleep(220); await page.keyboard.up('ArrowRight'); }
    await page.keyboard.up('ArrowUp'); }
  const gy3 = await call('dbg_tanky');
  T('the tank rolls through the opened barrier', gy3 < 2100, 'y=' + gy3);

  // the core: warp south of the boss gate (ty 8, y 384..431), face
  // north, and hold fire (the tankdiag-proven pattern: a parked tank
  // kills the core in ~30s at the cost of ~2 lives; the core's shells
  // fizzle when it dies, so the kill always counts). Deaths respawn
  // at the last pad (re-warp); a full game over (3 lives) re-enters
  // the cabinet and replays the chain quickly.
  await page.keyboard.down('Space');
  let obj = -1; const t0 = Date.now();
  while (Date.now() - t0 < 300000) {
    obj = await call('dbg_tankobj');
    if (obj === 4) break;
    if ((await call('dbg_bwmode')) !== 5) {
      tankReentered = 1;   // v1.27: a fresh cabinet entry resets the launcher — the persistence check below needs to know
      await page.keyboard.up('Space');
      await call('dbg_tp', 219, -12108); await sleep(200);
      await key('KeyE', 250); await key('Space', 300);
      await poll('dbg_bwmode', v => v === 5, 4000);
      await call('dbg_tankquota');
      await call('dbg_tankwarp', 2350); await hold('ArrowUp', 400);
      await page.keyboard.down('Space');
      await poll('dbg_tankmid', v => v === 0, 60000);
      await call('dbg_tankwarp', 500);
      continue;
    }
    const y = await call('dbg_tanky');
    if (y > 900) { await call('dbg_tankwarp', 500); await page.keyboard.up('Space'); await hold('ArrowUp', 250); await page.keyboard.down('Space'); }
    await sleep(300);
  }
  await page.keyboard.up('Space');
  T('fortress core destroyed — objective is CROSS THE LINE (4)', obj === 4, 'obj=' + obj);

  // cross the finish line: level 2 loads, the record moves
  const lvl = await (async () => { const s0 = Date.now(); let v = 1;
    await page.keyboard.down('ArrowUp');
    while (Date.now() - s0 < 20000) { v = await call('dbg_tanklvl'); if (v === 2) break; await sleep(200); }
    await page.keyboard.up('ArrowUp'); return v; })();
  T('level 2 loaded after crossing the line', lvl === 2, 'lvl=' + lvl);
  T('record updated (tank_done 1)', (await call('dbg_tankdone')) === 1, await call('dbg_tankdone'));

  // v1.27: the grenade launcher lasts the whole run — level 2 loads
  // through tank_load_level, which must NOT reset it. If the boss
  // fight burned all 3 lives on the way in, the cabinet was
  // re-entered (tankReentered) and a fresh entry DOES reset it;
  // that branch asserts the reset instead.
  if (!tankReentered) T('grenade launcher persists across the level advance', (await call('dbg_tankglauncher')) === 1, await call('dbg_tankglauncher'));
  else T('a fresh cabinet entry reset the grenade launcher', (await call('dbg_tankglauncher')) === 0, await call('dbg_tankglauncher'));

  await call('dbg_bwexit'); await sleep(300);
  T('quitting returns to the arcade', (await call('dbg_inarcade')) === 1 && (await call('dbg_bwmode')) === -1,
    'arc=' + (await call('dbg_inarcade')) + ' mode=' + (await call('dbg_bwmode')));

  console.log('tanktest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
