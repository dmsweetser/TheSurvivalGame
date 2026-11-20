// e2e-kb.js — END-TO-END USABILITY PASS, keyboard phase (v1.25).
// Every cabinet, mode and system entered the way a player enters it:
// on foot, through the real doors, panels and menus. Debug exports
// only READ state (position, mode, flags). Two disclosed stagings,
// both inventory/position only (the systems under test are entered
// and driven for real): dbg_feed tops up the hat stack to the pet
// rock's price (the same staging budtest uses), and dbg_tp places
// the player at each arcade cabinet's feet (the row is long; every
// cabinet ENTRY is the real Interact + panel + launch).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 170) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  async function walkTo(tx, ty, tol = 12, timeout = 40000) {
    // tail-aware pulses: the web shell's keyup latch stretches every
    // perceived hold by ~120ms, so a pulse keeps moving the player
    // after keyup — settle before re-reading the position, or the
    // loop overshoots every stand point (doors, station discs).
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(60);
      for (const k of keys) await page.keyboard.up(k);
      await sleep(190);
    }
    return false;
  }
  async function menuGoTo(idx) {
    const n = (await call('dbg_menun')) + 1;
    const cur = await call('dbg_menusel');
    const steps = ((idx - cur) % n + n) % n;
    for (let i = 0; i < steps; i++) await key('ArrowDown', 110, 130);
    return (await call('dbg_menusel')) === idx;
  }
  // leave a Banzai World mode through its own Start menu (P -> LEAVE)
  async function bwLeave() {
    await key('KeyP', 200, 300);
    if ((await call('dbg_bwmenu')) !== 1) return false;
    await key('ArrowDown', 120, 150); await key('ArrowDown', 120, 150);
    await key('Space', 250, 300);
    return (await poll('dbg_bwmode', v => v === -1, 6000)) === -1;
  }

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(400);
  T('boots to the title menu', (await call('dbg_state')) === 0);

  // ---------- title: HOW TO PLAY, SETTINGS (19 rows), ZEN, EDDIE ----------
  T('menu reaches HOW TO PLAY', await menuGoTo(1));
  await key('Space', 300, 300);
  T('help opens', (await call('dbg_state')) !== 0, await call('dbg_state'));
  await key('Space', 200, 250); await key('Space', 200, 250);   // page 2, page 3
  await key('Space', 200, 300);                                 // past the last page -> menu
  T('help pages through and returns to the title', (await poll('dbg_state', v => v === 0, 5000)) === 0, await call('dbg_state'));

  T('menu reaches SETTINGS', await menuGoTo(2));
  await key('Space', 300, 300);
  T('settings opens (state 6)', (await poll('dbg_state', v => v === 6, 5000)) === 6, await call('dbg_state'));
  const comfort0 = await call('dbg_comfort');
  for (let i = 0; i < 18; i++) await key('ArrowDown', 100, 110);   // walk all 19 rows
  T('settings: walked to the last of 19 rows, still in settings', (await call('dbg_state')) === 6);
  for (let i = 0; i < 18; i++) await key('ArrowUp', 100, 110);
  await key('ArrowLeft', 130, 150); await key('ArrowLeft', 130, 150);   // MUSIC VOLUME 10 -> 8
  T('settings: MUSIC VOLUME adjusts', (await call('dbg_comfort')) !== comfort0, await call('dbg_comfort'));
  await key('ArrowRight', 130, 150); await key('ArrowRight', 130, 150);
  T('settings: value restored', (await call('dbg_comfort')) === comfort0, await call('dbg_comfort'));
  await key('Escape', 250, 300);
  T('settings closes back to the title', (await poll('dbg_state', v => v === 0, 5000)) === 0);

  T('menu reaches ZEN MODE', await menuGoTo(4));
  await key('Space', 300, 300);
  T('ZEN toggles on', (await call('dbg_mzen')) === 1, await call('dbg_mzen'));
  T('menu reaches EDDIE MODE', await menuGoTo(5));
  await key('Space', 300, 300);
  T('EDDIE toggles on', (await call('dbg_meddie')) === 1, await call('dbg_meddie'));

  // ---------- start the run (slot picker + name entry are real) ----------
  T('menu reaches START', await menuGoTo(0));
  await key('Space', 400, 400);
  await key('Space', 400, 400);   // slot 1
  if ((await call('dbg_state')) === 4) {   // name entry for a fresh slot
    await page.keyboard.type('SCRAPPY', { delay: 60 });
    await key('Enter', 300, 400);
  }
  T('run started', (await poll('dbg_state', v => v === 1, 8000)) === 1, await call('dbg_state'));
  T('the run is Zen (records-safe)', (await call('dbg_zen')) === 1, await call('dbg_zen'));
  await page.screenshot({ path: 'tsg-v125-e2e-kb-zen-eddie.png' });

  // ---------- overworld on foot: fight, pause + CONTROLS ----------
  T('walked out to the field on foot', await walkTo(330, 330, 16));
  // roam a small circuit (spawns track a moving player) until a bug
  // is on top of us — one steering loop, bug check every step
  let engaged = false;
  {
    const wps = [[-330, 330], [-330, -60], [330, -60], [330, 330]];
    let wi = 0;
    const t0 = Date.now();
    while (Date.now() - t0 < 90000 && !engaged) {
      const px = await call('dbg_px'), py = await call('dbg_py');
      const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
      if ((await call('dbg_bugs')) > 0 && Math.abs(bx - px) < 420 && Math.abs(by - py) < 420) { engaged = true; break; }
      let [tx, ty] = wps[wi % wps.length];
      if (Math.abs(px - tx) <= 20 && Math.abs(py - ty) <= 20) { wi++; [tx, ty] = wps[wi % wps.length]; }
      const keys = [];
      if (px < tx - 14) keys.push('ArrowRight'); else if (px > tx + 14) keys.push('ArrowLeft');
      if (py < ty - 14) keys.push('ArrowDown'); else if (py > ty + 14) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(110);
      for (const k of keys) await page.keyboard.up(k);
    }
  }
  T('a bug closed in on the player', engaged, 'bugs=' + (await call('dbg_bugs')));
  let killed = false;
  for (let i = 0; i < 90 && !killed; i++) {
    const px = await call('dbg_px'), py = await call('dbg_py');
    const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
    const dx = bx - px, dy = by - py;
    const keys = [];
    if (Math.abs(dx) > 26) keys.push(dx > 0 ? 'ArrowRight' : 'ArrowLeft');
    if (Math.abs(dy) > 26) keys.push(dy > 0 ? 'ArrowDown' : 'ArrowUp');
    if (!keys.length) keys.push(Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp'));
    for (const k of keys) await page.keyboard.down(k);
    await page.keyboard.down('Space');
    await sleep(150);
    await page.keyboard.up('Space');
    for (const k of keys) await page.keyboard.up(k);
    if ((await call('dbg_kills')) > 0 || (await call('dbg_carried')) > 0) killed = true;
  }
  T('squashed a bug with the fire key (hat stack started)', killed, 'kills=' + (await call('dbg_kills')) + ' carried=' + (await call('dbg_carried')));

  await key('KeyP', 250, 300);
  T('pause opens in the overworld', (await poll('dbg_state', v => v === 5, 5000)) === 5, await call('dbg_state'));
  await key('ArrowDown', 130, 160);
  await key('Space', 250, 300);
  T('pause CONTROLS panel opens', (await poll('dbg_pausectl', v => v === 1, 5000)) === 1, await call('dbg_pausectl'));
  await key('Escape', 200, 250);
  T('controls panel backs out to the pause menu', (await call('dbg_state')) === 5 && (await call('dbg_pausectl')) === 0);
  await key('Escape', 200, 250);
  T('pause resumes the run', (await poll('dbg_state', v => v === 1, 5000)) === 1);

  // ---------- the Mall, on foot through the hut door ----------
  T('walked back to the hut door', await walkTo(0, 80, 12));
  await key('KeyE', 250, 300);
  T('entered the Bug Mall through the hut door', (await poll('dbg_mall', v => v === 1, 6000)) === 1, await call('dbg_mall'));
  // hat-stack staging for the adoption price (budtest's own staging);
  // 80 kg leaves a carcass for the bowl and the bank after the rock
  for (let i = 0; i < 8 && (await call('dbg_carried')) < 800; i++) await call('dbg_feed');

  const station = async (tx, ty, si, name, verb) => {
    T(`walked to ${name}`, await walkTo(tx, ty, 14));
    await key('KeyE', 250, 300);
    T(`${name} opens`, (await poll('dbg_station', v => v === si, 5000)) === si && (await call('dbg_state')) === 2,
      'station=' + (await call('dbg_station')) + ' state=' + (await call('dbg_state')));
    if (verb) await verb();
    // most panels close on Escape; the Photo Booth's STEP OUT already
    // closed its panel (state back to play) — an extra Escape there
    // would open the pause menu instead
    if ((await call('dbg_state')) === 2) await key('Escape', 250, 300);
    T(`${name} closes`, (await poll('dbg_state', v => v === 1, 5000)) === 1, await call('dbg_state'));
  };
  await station(-315, -6148, 0, 'THE COUNTER');
  await station(-105, -6148, 1, 'WHALES ONLY', async () => {
    await key('ArrowDown', 130, 160); await key('ArrowUp', 130, 160);   // browse the list, buy nothing
  });
  await station(315, -6148, 3, 'BANZAI BUDDIES', async () => {
    for (let i = 0; i < 4; i++) await key('ArrowDown', 130, 160);   // row 4: PET ROCK (60)
    await key('Space', 300, 350);
    T('buddies: species picked, name picker opens', (await call('dbg_budstage')) === 1, await call('dbg_budstage'));
    await key('Space', 300, 350);                                    // row 0: AEGNOR
    T('buddies: name picked, colour picker opens', (await call('dbg_budstage')) === 2, await call('dbg_budstage'));
    await key('Space', 300, 400);
    T('buddies: PET ROCK adopted through the panel', (await poll('dbg_buddy', v => v !== 0, 8000)) === 5, await call('dbg_buddy'));
  });
  await station(-180, -5868, 4, 'BUG BOWL', async () => {
    const f0 = await call('dbg_bowl_frame');
    await key('Space', 250, 250); await key('Space', 250, 350); await key('Space', 250, 300); await key('Space', 250, 300);
    const moved = await poll('dbg_bowl_frame', v => v !== f0, 12000);
    T('bowl: a ball was thrown through the aim/power phases', moved !== f0 || (await call('dbg_bowl_pins')) < 10,
      'frame=' + f0 + ' -> ' + (await call('dbg_bowl_frame')) + ' pins=' + (await call('dbg_bowl_pins')));
  });
  await station(105, -6148, 2, 'BUG BANK', async () => {
    if ((await call('dbg_carried')) > 0) {
      const b0 = await call('dbg_bank');
      await key('Space', 300, 400);   // row 0: DEPOSIT ALL
      T('bank: DEPOSIT ALL banked the hat stack', (await call('dbg_bank')) > b0, b0 + ' -> ' + (await call('dbg_bank')));
    } else console.log('NOTE bank deposit skipped: empty hat stack');
  });
  await station(340, -5868, 5, 'TROPHY CASE', async () => {
    await key('ArrowDown', 130, 160); await key('ArrowDown', 130, 160); await key('ArrowUp', 130, 160);
  });
  await station(-330, -5868, 6, 'CHALLENGE BOARD', async () => {
    T('challenge board has a live weekly target', (await call('dbg_wk')) >= 0, await call('dbg_wk'));
  });
  await station(-366, -5992, 7, 'BOUNTY BOARD');
  await station(-232, -5980, 8, 'PHOTO BOOTH', async () => {
    T('booth stage opens', (await poll('dbg_portstage', v => v === 5, 5000)) === 5, await call('dbg_portstage'));
    await key('ArrowDown', 130, 160); await key('ArrowDown', 130, 160);   // row 2: SNAP
    await key('Space', 400, 400);
    T('booth: a portrait was snapped', (await poll('dbg_porthas', v => v === 1, 6000)) === 1, await call('dbg_porthas'));
    await key('ArrowDown', 130, 160); await key('ArrowDown', 130, 160);   // row 4: STEP OUT
    await key('Space', 300, 350);
    T('booth: stepped out of the booth stage', (await call('dbg_portstage')) === 0, await call('dbg_portstage'));
  });

  // ---------- the Arcade and its seven cabinets ----------
  T('walked to the arcade door', await walkTo(350, -5976, 14));
  await key('KeyE', 250, 300);
  T('entered the Arcade through the east door', (await poll('dbg_inarcade', v => v === 1, 6000)) === 1, await call('dbg_inarcade'));

  const cabinet = async (tx, ty, cab, name, mode, verb) => {
    await call('dbg_tp', tx, ty); await sleep(250);   // position staging only; ENTRY is the real E + panel + launch
    T(`standing at the ${name} cabinet`, (await call('dbg_cabinet')) === cab, await call('dbg_cabinet'));
    await key('KeyE', 250, 300);
    T(`${name} cabinet panel opens`, (await poll('dbg_arcgate', v => v === 1, 5000)) === 1, await call('dbg_arcgate'));
    await key('Space', 300, 400);
    T(`${name} launches`, (await poll('dbg_bwmode', v => v === mode, 6000)) === mode, await call('dbg_bwmode'));
    if (verb) await verb();
    T(`${name}: left via its Start menu`, await bwLeave());
    T(`${name}: back in the Arcade`, (await call('dbg_inarcade')) === 1 && (await call('dbg_bwmode')) === -1);
  };
  await cabinet(-256, -12108, 0, 'ENTRANCES', 2, async () => {
    T('entrances: in room 1', (await call('dbg_entroom')) === 1, await call('dbg_entroom'));
    await page.keyboard.down('ArrowRight'); await sleep(1200); await page.keyboard.up('ArrowRight');
  });
  await cabinet(0, -12108, 1, 'SUPER BUDDY RUN', 3, async () => {
    const t0 = await call('dbg_bwx');
    await page.keyboard.down('ArrowRight'); await sleep(900); await page.keyboard.up('ArrowRight');
    await key('Space', 150, 200);   // jump
    T('run: the runner is moving', (await call('dbg_bwx')) !== t0, t0 + ' -> ' + (await call('dbg_bwx')));
  });
  await cabinet(256, -12108, 2, 'BUDDY CRAFT', 4, async () => {
    const x0 = await call('dbg_bwx');
    await page.keyboard.down('ArrowRight'); await sleep(800); await page.keyboard.up('ArrowRight');
    await key('Space', 150, 250);
    T('craft: the buddy roams the wilds', (await call('dbg_bwx')) !== x0, x0 + ' -> ' + (await call('dbg_bwx')));
  });
  await cabinet(-256, -11898, 3, 'TANK RESCUE', 5, async () => {
    const y0 = await call('dbg_tanky');
    await page.keyboard.down('ArrowUp'); await sleep(1100); await page.keyboard.up('ArrowUp');
    T('tank: it drives north', (await call('dbg_tanky')) < y0 - 40, y0 + ' -> ' + (await call('dbg_tanky')));
    await page.keyboard.down('Space'); await sleep(400); await page.keyboard.up('Space');
    T('tank: the cannon fires', (await poll('dbg_tankshells', v => v > 0, 3000)) > 0, await call('dbg_tankshells'));
  });
  await cabinet(0, -11898, 4, 'CASTLE', 6, async () => {
    T('castle: inside the castle', (await call('dbg_casroom')) >= 1, await call('dbg_casroom'));
    await page.keyboard.down('ArrowRight'); await sleep(700); await page.keyboard.up('ArrowRight');
  });
  await cabinet(256, -11898, 5, 'SUPER BUDDY CART', 7, async () => {
    T('cart: the waiting room comes first', (await call('dbg_crtstate')) === 3, await call('dbg_crtstate'));
    await key('Space', 300, 300);   // FIRE in the waiting room: race solo
    T('cart: racing after the countdown', (await poll('dbg_crtstate', v => v === 1, 9000)) === 1, await call('dbg_crtstate'));
    const p0 = await call('dbg_crtprog');
    await page.keyboard.down('Space'); await sleep(2500); await page.keyboard.up('Space');   // FIRE is the gas
    T('cart: the kart drives and gains ground', (await call('dbg_crtprog')) > p0 + 120, p0 + ' -> ' + (await call('dbg_crtprog')));
  });
  // WAR!: map -> 'S' skirmish panel -> LAUNCH -> battle -> pause SAVE & QUIT -> map -> Esc out
  await call('dbg_tp', 0, -11688); await sleep(250);
  T('standing at the WAR! cabinet', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  await key('KeyE', 250, 300);
  T('WAR! cabinet panel opens', (await poll('dbg_arcgate', v => v === 1, 5000)) === 1);
  await key('Space', 300, 400);
  T('WAR! launches to the world map', (await poll('dbg_warstate', v => v === 0, 6000)) === 0 && (await call('dbg_bwmode')) === 8,
    'warstate=' + (await call('dbg_warstate')) + ' mode=' + (await call('dbg_bwmode')));
  await key('KeyS', 200, 300);
  T('skirmish panel opens on the S key', (await poll('dbg_warstate', v => v === 4, 5000)) === 4, await call('dbg_warstate'));
  await key('ArrowDown', 130, 160); await key('ArrowDown', 130, 160);   // LAUNCH row
  await key('Space', 300, 500);
  T('skirmish battle launches', (await poll('dbg_warstate', v => v === 1, 6000)) === 1, await call('dbg_warstate'));
  T('the battle has an army and a purse', (await call('dbg_warunits')) > 0 && (await call('dbg_warbits')) > 0,
    'units=' + (await call('dbg_warunits')) + ' bits=' + (await call('dbg_warbits')));
  await key('Escape', 250, 350);   // Esc is the battle's pause verb (v1.17 design)
  T('WAR! pause opens on Esc', (await call('dbg_warpause')) === 1, await call('dbg_warpause'));
  // the pause's keyboard idiom is the weapon keys (1-3 pick a row,
  // war_pause_input) — Digit3 = SAVE & QUIT (campaign) / QUIT BATTLE
  await key('Digit3', 200, 500);
  T('WAR! pause SAVE & QUIT returns to the map', (await poll('dbg_warstate', v => v === 0, 6000)) === 0, await call('dbg_warstate'));
  await key('Escape', 250, 350);
  T('WAR!: Esc leaves the map back to the Arcade', (await poll('dbg_bwmode', v => v === -1, 6000)) === -1 && (await call('dbg_inarcade')) === 1,
    'mode=' + (await call('dbg_bwmode')));

  // out of the Arcade, then the Banzai gate: STORY, then FREE PLAY
  await call('dbg_tp', -330, -11976); await sleep(200);
  await key('KeyE', 250, 300);
  T('left the Arcade to the Mall', (await poll('dbg_inarcade', v => v === 0, 5000)) === 0 && (await call('dbg_mall')) === 1);
  T('walked to the Banzai gate', await walkTo(215, -5868, 14));
  const bits0 = await call('dbg_quest');
  await key('KeyE', 250, 350);
  await key('Space', 300, 400);   // row 0: STORY MODE
  T('STORY MODE launches from the gate panel', (await poll('dbg_bwmode', v => v === 1, 6000)) === 1, await call('dbg_bwmode'));
  {
    const NX = -2620, NY = -1440 + 26;   // Elder Patchina's standing spot (+ her doorstep offset)
    let talked = false;
    for (let i = 0; i < 60 && !talked; i++) {
      const x = await call('dbg_bwx'), y = await call('dbg_bwy');
      if (Math.abs(x - NX) <= 30 && Math.abs(y - NY) <= 34) {
        await key('KeyE', 250, 400);
        for (let j = 0; j < 5; j++) {   // walk the dialogue / accept the quest
          if ((await call('dbg_quest')) !== bits0) { talked = true; break; }
          await key('Space', 250, 350);
        }
        if ((await call('dbg_quest')) !== bits0) talked = true;
        break;
      }
      const keys = [];
      if (x < NX - 24) keys.push('ArrowRight'); else if (x > NX + 24) keys.push('ArrowLeft');
      if (y < NY - 24) keys.push('ArrowDown'); else if (y > NY + 24) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(110);
      for (const k of keys) await page.keyboard.up(k);
    }
    if (!talked) talked = (await poll('dbg_quest', v => v !== bits0, 4000)) !== bits0;   // the accept can land a beat after the last dialogue key
    T('story: walked to Patchina and took her quest (story bits moved)', talked, 'bits ' + bits0 + ' -> ' + (await call('dbg_quest')));
  }
  T('story: left via the Start menu', await bwLeave());
  await key('KeyE', 250, 350);
  await key('ArrowDown', 130, 160);
  await key('Space', 300, 400);   // row 1: FREE PLAY
  T('FREE PLAY launches from the gate panel', (await poll('dbg_bwmode', v => v === 0, 6000)) === 0, await call('dbg_bwmode'));
  { const x0 = await call('dbg_bwx');
    let moved = false;
    for (let i = 0; i < 10 && !moved; i++) {
      await page.keyboard.down('ArrowRight'); await sleep(350); await page.keyboard.up('ArrowRight');
      if ((await call('dbg_bwx')) !== x0 || (await call('dbg_bwbattle')) === 1) moved = true;
    }
    if ((await call('dbg_bwbattle')) === 1) console.log('NOTE free play: a hacker battle engaged during the roam (movement is battle-ruled)');
    T('free play: the buddy roams (or a battle engages)', moved, x0 + ' -> ' + (await call('dbg_bwx')) + ' battle=' + (await call('dbg_bwbattle'))); }
  T('free play: left via the Start menu', await bwLeave());
  T('back in the Mall after the gate trips', (await call('dbg_mall')) === 1 && (await call('dbg_bwmode')) === -1);

  // ---------- save + quit round trip ----------
  await key('KeyP', 250, 300);
  T('pause opens in the Mall', (await call('dbg_state')) === 5, await call('dbg_state'));
  await key('ArrowDown', 130, 160); await key('ArrowDown', 130, 160);
  await key('Space', 400, 400);   // SAVE + QUIT
  T('save+quit returns to the title', (await poll('dbg_state', v => v === 0, 6000)) === 0, await call('dbg_state'));

  console.log(`e2e-kb: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
