// arcadetest.js — THE ARCADE (Nov 10): the Banzai World side games
// moved out of the torii gate panel into their own room in the mall.
// One cabinet per game: ENTRANCES, SUPER BUDDY RUN, BUDDY CRAFT, and
// the new TANK RESCUE. The gate keeps STORY / FREE PLAY / LEAVE.
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

  // into the mall
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));

  // the torii gate panel is trimmed: Down x2 from the top is LEAVE,
  // not ENTRANCES — Space there must close the panel, not launch a game
  await call('dbg_tp', 215, -5868); await sleep(250);   // the Banzai gate disc (BW_SX 215, MALL_CY+132 — v1.25 MALL ENLARGEMENT)
  await key('KeyE', 250);
  await key('ArrowDown', 200); await key('ArrowDown', 200);
  await key('Space', 300);
  T('gate row 3 is LEAVE now (no game launched)', (await call('dbg_bwmode')) === -1,
    'mode=' + (await call('dbg_bwmode')));
  // and row 1 is still FREE PLAY
  await key('KeyE', 250);
  await key('ArrowDown', 200);
  await key('Space', 300);
  T('gate row 2 still enters Banzai World (free play)', (await poll('dbg_bwmode', v => v === 0, 4000)) === 0, await call('dbg_bwmode'));
  await call('dbg_bwexit'); await sleep(300);
  T('back in the mall after the gate trip', (await call('dbg_mall')) === 1 && (await call('dbg_inarcade')) === 0);

  // through the east door into THE ARCADE
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  T('still in the mall safe zone', (await call('dbg_mall')) === 1, await call('dbg_mall'));

  // cabinet 4: TANK RESCUE — island row, west box since the Nov 14
  // two-row rework (3 north wall + 3 island, SUPER BUDDY CART sixth)
  await call('dbg_tp', -256, -11898); await sleep(200);
  T('standing at the tank cabinet', (await call('dbg_cabinet')) === 3, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('tank cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('TANK RESCUE launched (bw_mode 5)', (await poll('dbg_bwmode', v => v === 5, 4000)) === 5, await call('dbg_bwmode'));
  T('tank level 1, hull whole', (await call('dbg_tanklvl')) === 1 && (await call('dbg_tankhull')) > 0,
    'lvl=' + (await call('dbg_tanklvl')) + ' hull=' + (await call('dbg_tankhull')));
  const ty0 = await call('dbg_tanky');
  await hold('ArrowUp', 1500);
  const ty1 = await call('dbg_tanky');
  T('the tank drives north', ty1 < ty0 - 80, ty0 + ' -> ' + ty1);
  // steering is not firing: holding Up (north) must NOT shoot — the
  // shared input fill folds K_UP into sel_held for the platformer's
  // jump, and tank mode is explicitly carved out of that fold
  T('no shells in flight after pure steering', (await poll('dbg_tankshells', v => v === 0, 2500)) === 0, await call('dbg_tankshells'));
  await hold('ArrowUp', 1200);
  T('still no auto-fire while driving north', (await call('dbg_tankshells')) === 0, await call('dbg_tankshells'));
  await page.keyboard.down('Space'); await sleep(450);
  const sh = await call('dbg_tankshells');
  await page.keyboard.up('Space');
  T('the fire button does fire the cannon', sh > 0, 'shells=' + sh);
  T('tank still alive after firing', (await call('dbg_tankhull')) > 0, await call('dbg_tankhull'));
  await call('dbg_bwexit'); await sleep(300);
  T('quitting the tank returns you to the arcade', (await call('dbg_inarcade')) === 1 && (await call('dbg_bwmode')) === -1,
    'arc=' + (await call('dbg_inarcade')) + ' mode=' + (await call('dbg_bwmode')));
  const ry = await call('dbg_py');
  T('...right back at the tank cabinet', Math.abs(ry - (-11890)) < 130, 'py=' + ry);

  // cabinet 1: ENTRANCES still launches from its (re-spaced) box
  await call('dbg_tp', -256, -12108); await sleep(200);
  T('standing at the entrances cabinet', (await call('dbg_cabinet')) === 0, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('entrances cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('ENTRANCES launched (bw_mode 2)', (await poll('dbg_bwmode', v => v === 2, 4000)) === 2, await call('dbg_bwmode'));
  await call('dbg_bwexit'); await sleep(300);
  T('back in the arcade after Entrances', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));

  // cabinet 7: WAR! — v1.27 raised it 70px (ARC_ROWY[6] 254 -> 184)
  // and the room's south clamp followed (ARC_CY+328 -> ARC_CY+258).
  // Stand point derivation: (ARC_SX[6], ARC_CY + ARC_ROWY[6] + 58)
  // = (0, -12000+184+58) = (0,-11758); the clamp sits at -11742.
  await call('dbg_tp', 0, -11758); await sleep(200);
  T('standing at the WAR! cabinet', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  // the clamp lives in arcade_collide, which runs on the movement
  // path only (players.c) — so prove it the player's way: stand at
  // the cabinet and WALK south into the room's edge.
  await page.keyboard.down('ArrowDown'); await sleep(700); await page.keyboard.up('ArrowDown'); await sleep(150);
  T('the south clamp followed the cabinet up (y clamps to ARC_CY+258 = -11742)', (await call('dbg_py')) === -11742, await call('dbg_py'));

  // out the west door, back to the mall proper
  await call('dbg_tp', -330, -11976); await sleep(200);
  await key('KeyE', 250);
  T('left the arcade to the mall', (await poll('dbg_inarcade', v => v === 0, 4000)) === 0 && (await call('dbg_mall')) === 1,
    'arc=' + (await call('dbg_inarcade')) + ' mall=' + (await call('dbg_mall')));

  // v1.25 E2E regression: a save written INSIDE the Arcade must not
  // strand the player outside it after a reload. The load resumes at
  // the Mall centre (Daniel's rule), but the save's ar= flag used to
  // survive the load — the reloaded player stood in the Mall still
  // flagged in_arcade, and the east door (which requires !in_arcade)
  // refused entry until they left by the exit mat. Found by the
  // feat-7 battery; the load now clears in_arcade with the rest of
  // the resume placement.
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('back inside the arcade for the save test', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await key('KeyP', 400);
  await key('ArrowDown', 200); await key('ArrowDown', 200);   // SAVE + QUIT row
  await key('Space', 500);
  T('save+quit from inside the arcade reaches the title', (await poll('dbg_state', v => v === 0, 5000)) === 0, await call('dbg_state'));
  await key('Space', 500); await key('Enter', 700);           // CONTINUE, slot 1
  T('continued after the arcade save', (await poll('dbg_state', v => v === 1, 8000)) === 1, await call('dbg_state'));
  T('reload resumes at the mall centre', Math.abs(await call('dbg_px')) <= 8 && Math.abs((await call('dbg_py')) + 6000) <= 8 && (await call('dbg_mall')) === 1,
    'px=' + (await call('dbg_px')) + ' py=' + (await call('dbg_py')));
  T('...and is NOT flagged in_arcade anymore', (await call('dbg_inarcade')) === 0, await call('dbg_inarcade'));
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  T('the east door lets the reloaded player straight back in', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));

  console.log('arcadetest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
