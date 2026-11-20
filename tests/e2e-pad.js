// e2e-pad.js — END-TO-END USABILITY PASS, gamepad phase (v1.25).
// Two fake standard-mapping pads (the bowlpad/warpadtest harness).
// Everything is driven on the pads: title + CO-OP toggle, walking
// (stick), station panels (B = interact, A = select), a buddy
// adoption, the Bug Bowl throw, the Arcade door, WAR! (map launch,
// stick-flown cursor, Core capture, command bar, pad pause -> the
// cursor-driven SAVE & QUIT pick), and finally the P2 split-screen
// drop-in. One disclosed assist: the fresh-slot NAME field is a text
// field — if the pad cannot confirm it, one keyboard Enter confirms
// the prefilled name (every pad suite boots the same way). dbg_tp
// stages position at the WAR! cabinet only; entry is the real
// B + panel + A. dbg_feed stages carcass weight (bowlpad's own).
// NOTE: in co-op, G.state stays in GS_PLAY while station panels live
// in per-player views — dbg_store1 is P1's in-store flag either way.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}
const PADSCRIPT = () => {
  window.__pads = [0, 1].map(i => {
    const buttons = [];
    for (let k = 0; k < 18; k++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + i, index: i, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0], timestamp: 0 };
  });
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  window.__axes = (pi, x, y) => { window.__pads[pi].axes[0] = x; window.__pads[pi].axes[1] = y; };
};
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT);
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const btn = async (pi, bi, hold = 200, wait = 200) => { await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]); await sleep(hold); await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]); await sleep(wait); };
  const axes = (pi, x, y) => page.evaluate(([p, a, b]) => window.__axes(p, a, b), [pi, x, y]);
  const A = 0, B = 1, BACK = 8, RB = 5, DDOWN = 13;
  // open a panel with B: press, then give the state a long window to
  // be read before pressing again (a second B inside an open panel
  // is an interact and can close it)
  async function openPanel(pred, ms = 12000) {
    const t0 = Date.now();
    while (Date.now() - t0 < ms) {
      if (await pred()) return true;
      await btn(0, B, 200, 300);
      await sleep(700);
    }
    return await pred();
  }
  async function padWalkTo(tx, ty, tol = 14, timeout = 25000, pi = 0) {
    // bang-bang steering: the pad's digital movement cuts out below
    // the 0.45 axis threshold, so proportional axes stall just short
    // of the target — full deflection per axis until inside tol
    const fx = pi === 0 ? 'dbg_px' : 'dbg_p2x', fy = pi === 0 ? 'dbg_py' : 'dbg_p2y';
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call(fx), y = await call(fy);
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) { await axes(pi, 0, 0); return true; }
      await axes(pi, Math.abs(x - tx) <= tol ? 0 : Math.sign(tx - x), Math.abs(y - ty) <= tol ? 0 : Math.sign(ty - y));
      await sleep(110);
    }
    await axes(pi, 0, 0);
    return false;
  }
  // the WAR camera + cursor screen position, derived the warpadtest
  // way (a unit's arena pos vs its rendered pos gives the camera)
  async function curScreen() {
    return page.evaluate(() => {
      const T = window.__TSG;
      const cur = T.dbg_warcur();
      if (cur < 0) return null;
      const cx = Math.floor(cur / 10000), cy = cur % 10000;
      const derive = (axy, apos) => {
        if (axy < 0 || apos < 0) return null;
        const ax = Math.floor(axy / 10000), ay = axy % 10000;
        const sx = apos % 10000, sy = Math.floor(apos / 10000);
        if (sx < 2 || sy < 2) return null;
        const camx = ax - (sx - 320), camy = ay - (sy - 180 + 23);
        return { arena: [cx, cy], cam: [camx, camy], screen: [320 + (cx - camx), 180 + (cy - camy - 23)] };
      };
      for (let i = 0; i < 80; i++) { const r = derive(T.dbg_warunitxy(i), T.dbg_warunitpos(i)); if (r) return r; }
      const tn = T.dbg_warterrain();
      for (let t = 0; t < tn; t++) { const r = derive(T.dbg_warfeatxy(t), T.dbg_warfeatpos(t)); if (r) return r; }
      return { arena: [cx, cy], cam: null, screen: null };
    });
  }
  // fly P1's WAR cursor to an ARENA point on the stick (warpadtest's flyTo)
  async function flyToArena(tx, ty, tol = 9, maxMs = 12000) {
    const t0 = Date.now();
    while (Date.now() - t0 < maxMs) {
      const cur = await call('dbg_warcur');
      if (cur < 0) { await axes(0, 0.6, 0); await sleep(150); continue; }
      const cx = Math.floor(cur / 10000), cy = cur % 10000;
      const dx = tx - cx, dy = ty - cy;
      if (Math.abs(dx) <= tol && Math.abs(dy) <= tol) { await axes(0, 0, 0); return true; }
      const cl = v => Math.max(-1, Math.min(1, v / 55));
      await axes(0, cl(dx), cl(dy));
      await sleep(110);
    }
    await axes(0, 0, 0);
    return false;
  }

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  T('boots to the title with pads connected', (await call('dbg_state')) === 0 && (await call('dbg_pads')) >= 2, await call('dbg_pads'));

  // enable 2P CO-OP for later: last menu item when two pads are connected
  {
    const n = await call('dbg_menun');           // items; cycle is n+1 (name row)
    const coopIdx = n - 1;
    let cur = await call('dbg_menusel');
    const steps = ((coopIdx - cur) % (n + 1) + n + 1) % (n + 1);
    for (let i = 0; i < steps; i++) await btn(0, DDOWN, 150, 180);
    await btn(0, A, 200, 300);
    T('2P CO-OP toggled on from the pad', (await call('dbg_coop')) === 1, 'coop=' + (await call('dbg_coop')) + ' sel=' + (await call('dbg_menusel')));
    cur = await call('dbg_menusel');
    const back = ((0 - cur) % (n + 1) + n + 1) % (n + 1);
    for (let i = 0; i < back; i++) await btn(0, DDOWN, 150, 180);
  }
  await btn(0, A, 200, 350);   // START
  await btn(0, A, 200, 400);   // slot 1
  if ((await call('dbg_state')) === 4) {
    await btn(0, A, 200, 400);   // try to confirm the name on pad
    if ((await call('dbg_state')) === 4) {
      await page.keyboard.press('Enter');   // disclosed assist: text field confirm
      await sleep(400);
      console.log('NOTE name entry confirmed with one keyboard Enter (text field)');
    }
  }
  T('run started from the pad', (await poll('dbg_state', v => v === 1, 8000)) === 1, await call('dbg_state'));

  // P1 on foot to the hut, into the Mall, bowl a ball — all pad
  T('P1 walked to the hut door on the stick', await padWalkTo(0, 80, 13));
  await btn(0, B, 200, 350);
  T('P1 entered the Mall with pad B (interact)', (await poll('dbg_mall', v => v === 1, 6000)) === 1, await call('dbg_mall'));
  for (let i = 0; i < 3; i++) await call('dbg_feed');   // carcass staging (bowlpad's own)
  T('P1 walked to the Bug Bowl counter on the stick', await padWalkTo(-180, -5868, 14));
  T('bowling view opens on pad B', await openPanel(async () => (await call('dbg_store1')) === 1 && (await call('dbg_station')) === 4), await call('dbg_station'));
  { const f0 = await call('dbg_bowl_frame');
    await btn(0, A, 180, 300); await btn(0, A, 180, 400); await btn(0, A, 180, 350); await btn(0, A, 180, 350);
    const moved = await poll('dbg_bowl_frame', v => v !== f0, 12000);
    T('a ball was thrown on pad A', moved !== f0 || (await call('dbg_bowl_pins')) < 10, 'frame=' + f0 + ' -> ' + (await call('dbg_bowl_frame'))); }
  await btn(0, BACK, 200, 350);
  T('bowling view closes on pad Back', (await poll('dbg_store1', v => v === 0, 5000)) === 0, await call('dbg_store1'));

  // adopt a buddy on pad — the cabinets refuse without one
  // ("YOU NEED A BANZAI BUDDY FIRST" is the game's own rule)
  for (let i = 0; i < 8 && (await call('dbg_carried')) < 800; i++) await call('dbg_feed');
  T('P1 walked to the Banzai Buddies counter on the stick', await padWalkTo(315, -6148, 14));
  T('buddies panel opens on pad B', await openPanel(async () => (await call('dbg_store1')) === 1 && (await call('dbg_station')) === 3), await call('dbg_station'));
  for (let i = 0; i < 4; i++) await btn(0, DDOWN, 150, 200);   // row 4: PET ROCK
  await btn(0, A, 200, 350);
  T('buddies: species picked on pad A', (await call('dbg_budstage')) === 1, await call('dbg_budstage'));
  await btn(0, A, 200, 350);
  await btn(0, A, 200, 400);
  T('buddies: PET ROCK adopted through the pad panel', (await poll('dbg_buddy', v => v !== 0, 8000)) === 5, await call('dbg_buddy'));
  await btn(0, BACK, 200, 350);
  T('buddies panel closes on pad Back', (await poll('dbg_store1', v => v === 0, 5000)) === 0, await call('dbg_store1'));

  // the Arcade door + WAR! on pad (solo view: the pause panel's
  // geometry is the full-screen one)
  T('P1 walked to the arcade door on the stick', await padWalkTo(350, -5976, 14));
  await btn(0, B, 200, 350);
  T('entered the Arcade on pad B', (await poll('dbg_inarcade', v => v === 1, 6000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11688); await sleep(250);   // position staging only
  T('standing at the WAR! cabinet', (await call('dbg_cabinet')) === 6, await call('dbg_cabinet'));
  T('WAR! cabinet panel opens on pad B', await openPanel(async () => (await call('dbg_arcgate')) === 1), await call('dbg_arcgate'));
  await btn(0, A, 200, 400);
  T('WAR! launches to the map on pad A', (await poll('dbg_warstate', v => v === 0, 6000)) === 0 && (await call('dbg_bwmode')) === 8,
    'warstate=' + (await call('dbg_warstate')));
  await btn(0, A, 200, 500);   // confirm region stop 0
  T('pad A launched the battle', (await poll('dbg_warstate', v => v === 1, 6000)) === 1, await call('dbg_warstate'));
  await sleep(600);
  { // fly the cursor to the player's Core on the stick and capture it with A
    const s0 = await call('dbg_warstructxy', 0);
    const sx = Math.floor(s0 / 10000), sy = s0 % 10000;
    T('flew the cursor to the Core on the stick', await flyToArena(sx, sy));
    await sleep(200);
    await btn(0, A, 180, 350);
    T('the Core was captured on stick + A', (await call('dbg_warselstruct')) === 0, 'selstruct=' + (await call('dbg_warselstruct')));
    const hl0 = await call('dbg_warbarhl');
    await btn(0, RB, 180, 300);
    T('RB walks the command bar', (await call('dbg_warbarhl')) !== hl0, hl0 + ' -> ' + (await call('dbg_warbarhl')));
  }
  // the pad pause is cursor-driven (war_pad_pause -> war_pause_tap).
  // Wake semantics: with the cursor off, the first stick input in
  // the pause wakes it AT THE CAMERA CENTRE — which is also the
  // panel's centre. So: take the cursor off with a mouse tap on
  // empty ground (the mouse takes the field back), open the pause,
  // wake the cursor, read where it woke, and fly +54px of screen
  // (1:1) down onto the QUIT BATTLE button's centre.
  { const box = await page.locator('canvas').boundingBox();
    await page.mouse.click(box.x + 60 / 640 * box.width, box.y + 60 / 360 * box.height);
    await sleep(300);
    T('the mouse tap handed the field back (pad cursor off)', (await call('dbg_warcur')) === -1, await call('dbg_warcur')); }
  await btn(0, BACK, 200, 400);   // the battle's pause verb is Esc/Back (v1.17), not Start
  T('WAR! pause opens on pad Back', (await call('dbg_warpause')) === 1, await call('dbg_warpause'));
  await axes(0, 0.3, 0); await sleep(70); await axes(0, 0, 0); await sleep(250);   // wake at the camera centre
  { const woke = await call('dbg_warcur');
    T('the pad cursor woke in the pause', woke >= 0, woke);
    const wx = Math.floor(woke / 10000), wy = woke % 10000;
    T('flew the woken cursor onto QUIT BATTLE', await flyToArena(wx, wy + 54));
    await sleep(150);
    await btn(0, A, 180, 500);
    T('pad cursor picked QUIT BATTLE, back at the map', (await poll('dbg_warstate', v => v === 0, 5000)) === 0, 'warstate=' + (await call('dbg_warstate'))); }
  await btn(0, BACK, 200, 400);
  T('pad Back leaves the map to the Arcade', (await poll('dbg_bwmode', v => v === -1, 6000)) === -1 && (await call('dbg_inarcade')) === 1,
    'mode=' + (await call('dbg_bwmode')));

  // out to the overworld for the co-op leg
  await call('dbg_tp', -330, -11976); await sleep(200);
  await btn(0, B, 200, 350);
  T('left the Arcade to the Mall on pad B', (await poll('dbg_inarcade', v => v === 0, 5000)) === 0 && (await call('dbg_mall')) === 1);
  T('P1 walked to the exit mat on the stick', await padWalkTo(0, -5810, 12));
  await btn(0, B, 200, 350);
  T('P1 left the Mall on pad B', (await poll('dbg_mall', v => v === 0, 5000)) === 0, await call('dbg_mall'));

  // P2 drop-in on the second pad + split-screen
  await btn(1, A, 200, 500);
  T('P2 dropped in from pad 2', (await poll('dbg_p2', v => v >= 0, 6000)) >= 0, await call('dbg_p2'));
  { const x0 = await call('dbg_p2x');
    await axes(1, 1, 0); await sleep(800); await axes(1, 0, 0);
    T('P2 walks on pad 2\'s stick', (await call('dbg_p2x')) > x0 + 8, x0 + ' -> ' + (await call('dbg_p2x'))); }
  T('P2 walked away east on pad 2', await padWalkTo(560, 40, 20, 20000, 1));
  T('split-screen engages as the players separate', (await poll('dbg_split', v => v === 1, 8000)) === 1, await call('dbg_split'));
  await page.screenshot({ path: 'tsg-v125-e2e-pad-split.png' });
  T('P2 walked back west', await padWalkTo(60, 40, 20, 20000, 1));

  console.log(`e2e-pad: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
