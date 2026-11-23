// warpadtest.js — v1.25 WAR! PAD FIXES (Daniel's playtest, Nov 19):
// wartest is green, and on a real pad the building-select CAPTURE
// still failed him and the cursor drifted off-centre. wartest's pad
// legs use the wake-at-centre trick with the stick RELEASED before
// A; Daniel's hands found the doors wartest never opens:
//
//  1. THE PHANTOM STEP — press A while still pushing the stick
//     toward the building (the natural mid-flight select) and the
//     capture opened on the WRONG button: the entry zeroed the
//     stick-edge state, so the still-held stick read as a fresh
//     deflection on the next frame and stepped the highlight once
//     for free. Proven deterministic pre-fix (barhl=1, 3/3).
//  2. THE DEAD D-PAD — the D-pad never moved the WAR! cursor at
//     all (it only fed the camera pan, which the edge-follow then
//     cancelled): dbg_warcur was bit-identical after 0.9s of DRight.
//     A D-pad player could not aim at a building, let alone capture
//     its bar. The D-pad now wakes, flies, selects and walks the
//     captured menu like the stick does.
//  3. THE DRIFTING CURSOR — the old camera model flew the cursor
//     inside a clamped camera while war_update's key pan (fed by
//     the pad's own stick through IN.keys) shoved the camera at
//     430 u/s against the cursor's 540: the difference piled up as
//     a screen offset, the edge-follow pinned the cursor 28u inside
//     the edge, and the rim clamp stranded it there. Daniel's model
//     replaces it: THE CURSOR IS THE CAMERA — centred always, the
//     view scrolls off the map at the rim and the off-map ground
//     (already drawn darker past the fence) compensates. The right
//     stick's pan became cursor flight like the left's.
//
// This suite drives REAL pad events only (fake pad through the
// shell's real poll, like skirmishpathtest) — closed-loop stick
// flight onto each building, A pressed with the stick HELD, D-pad
// flight and menu steps, and centred-camera reads at the rim. dbg
// exports arrange state (seed, bits, buildings) and OBSERVE only.
// Cursor screen position is derived from ONE synchronous evaluate
// reading a unit/feature's arena pos and screen pos together (a
// consistent camera; dbg_warunitpos/dbg_warfeatpos clamp negative
// screen coords to 0, so only references with sx,sy >= 2 are used).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function newPage(browser) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 }, hasTouch: true });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    const buttons = [];
    for (let i = 0; i < 18; i++) buttons.push({ pressed: false, touched: false, value: 0 });
    window.__FAKEPAD = { id: 'Fake Standard Pad', index: 0, connected: true, mapping: 'standard',
                         axes: [0, 0, 0, 0], buttons, timestamp: 0 };
    navigator.getGamepads = () => [window.__FAKEPAD, null, null, null];
  });
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  return page;
}
const call = (page, fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
async function poll(page, fn, want, timeout = 6000) {
  const t0 = Date.now();
  while (Date.now() - t0 < timeout) { const v = await call(page, fn); if (want(v)) return v; await sleep(120); }
  return call(page, fn);
}
async function key(page, k, ms = 150) { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); }
async function enterWarMap(page) {
  await key(page, 'Space', 400);
  if ((await call(page, 'dbg_state')) !== 1) await key(page, 'Enter', 400);
  if ((await poll(page, 'dbg_state', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_adopt', 0);
  await call(page, 'dbg_tp', 0, 80); await sleep(200);
  await key(page, 'KeyE', 300);
  if ((await poll(page, 'dbg_mall', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_tp', 350, -5976); await sleep(200);
  await key(page, 'KeyE', 250);
  if ((await poll(page, 'dbg_inarcade', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_tp', 0, -11758); await sleep(200);   // v1.27: WAR! stand point = (0, -12000+184+58) — the cabinet rose 70px (ARC_ROWY[6] 254 -> 184)
  if ((await call(page, 'dbg_cabinet')) !== 6) return false;
  await key(page, 'KeyE', 250);
  if ((await poll(page, 'dbg_arcgate', v => v === 1)) !== 1) return false;
  await key(page, 'Space', 300);
  if ((await poll(page, 'dbg_bwmode', v => v === 8)) !== 8) return false;
  if ((await poll(page, 'dbg_warstate', v => v === 0)) !== 0) return false;
  await sleep(700);   // entry grace
  return true;
}
const unxy = v => [Math.floor(v / 10000), v % 10000];
function padTools(page) {
  const padBtn = (i, down) => page.evaluate(([n, d]) => { const b = window.__FAKEPAD.buttons[n]; b.pressed = d; b.value = d ? 1 : 0; }, [i, down]);
  const padTap = async (i, holdMs = 130) => { await padBtn(i, true); await sleep(holdMs); await padBtn(i, false); await sleep(150); };
  const padAxes = (lx, ly, rx, ry) => page.evaluate(([a, b, c, d]) => {
    window.__FAKEPAD.axes[0] = a; window.__FAKEPAD.axes[1] = b; window.__FAKEPAD.axes[2] = c; window.__FAKEPAD.axes[3] = d;
  }, [lx, ly, rx, ry]);
  return { padBtn, padTap, padAxes };
}
async function flyTo(page, P, tx, ty, tol = 9, maxMs = 12000) {
  const t0 = Date.now();
  while (Date.now() - t0 < maxMs) {
    const cur = await call(page, 'dbg_warcur');
    if (cur < 0) { await P.padAxes(0.6, 0, 0, 0); await sleep(150); continue; }
    const [cx, cy] = unxy(cur);
    const dx = tx - cx, dy = ty - cy;
    if (Math.abs(dx) <= tol && Math.abs(dy) <= tol) { await P.padAxes(0, 0, 0, 0); return [cx, cy]; }
    const cl = v => Math.max(-1, Math.min(1, v / 55));
    await P.padAxes(cl(dx), cl(dy), 0, 0);
    await sleep(110);
  }
  await P.padAxes(0, 0, 0, 0);
  const cur = await call(page, 'dbg_warcur');
  return cur >= 0 ? unxy(cur) : [-1, -1];
}
async function deselect(page, P) {
  for (let i = 0; i < 3; i++) {
    if ((await call(page, 'dbg_warselstruct')) === -1 && (await call(page, 'dbg_warbarcap')) === 0
        && (await call(page, 'dbg_warsel')) === 0) break;
    await P.padTap(1);   // B peels / cancels
  }
  await sleep(200);
}
// fly onto a building, release the stick, press A. If a wandering
// unit eats the tap instead (a player would just tap again), B out
// and retry once — the assertion still demands the building's
// capture, at full strength.
async function selectByStick(page, P, sidx) {
  const [sx, sy] = unxy(await call(page, 'dbg_warstructxy', sidx));
  for (let attempt = 0; attempt < 2; attempt++) {
    await flyTo(page, P, sx, sy);
    await sleep(250);
    await P.padTap(0);
    await sleep(300);
    if ((await call(page, 'dbg_warselstruct')) === sidx) return true;
    await deselect(page, P);
  }
  return (await call(page, 'dbg_warselstruct')) === sidx;
}
// fly onto a building and press A with the stick STILL PUSHED toward
// it, then keep holding — the phantom-step regression posture
async function selectHeld(page, P, sidx) {
  const [sx, sy] = unxy(await call(page, 'dbg_warstructxy', sidx));
  await flyTo(page, P, sx, sy);
  const cur = unxy(await call(page, 'dbg_warcur'));
  let dx = sx - cur[0], dy = sy - cur[1];
  if (dx === 0 && dy === 0) dx = 1;   // a centred landing still gets a held push (east)
  const m = Math.max(Math.abs(dx), Math.abs(dy), 1);
  await P.padAxes(dx / m, dy / m, 0, 0);
  await P.padTap(0);   // A lands on the next frame, cursor still on the building, stick still pushed
  await sleep(400);   // still holding: the phantom step fired on the entry frame + 1
  const r = { sel: await call(page, 'dbg_warselstruct'), cap: await call(page, 'dbg_warbarcap'), hl: await call(page, 'dbg_warbarhl') };
  await P.padAxes(0, 0, 0, 0);
  return r;
}
// cursor screen pos + camera from one synchronous evaluate
async function curScreen(page) {
  return page.evaluate(() => {
    const T = window.__TSG;
    const cur = T.dbg_warcur();
    if (cur < 0) return null;
    const cx = Math.floor(cur / 10000), cy = cur % 10000;
    const derive = (axy, apos) => {
      if (axy < 0 || apos < 0) return null;
      const ax = Math.floor(axy / 10000), ay = axy % 10000;
      const sx = apos % 10000, sy = Math.floor(apos / 10000);
      if (sx < 2 || sy < 2) return null;   // the pos exports clamp negatives to 0
      const camx = ax - (sx - 320), camy = ay - (sy - 180 + 23);
      return { arena: [cx, cy], cam: [camx, camy], screen: [320 + (cx - camx), 180 + (cy - camy - 23)] };
    };
    for (let i = 0; i < 80; i++) { const r = derive(T.dbg_warunitxy(i), T.dbg_warunitpos(i)); if (r) return r; }
    const tn = T.dbg_warterrain();
    for (let t = 0; t < tn; t++) { const r = derive(T.dbg_warfeatxy(t), T.dbg_warfeatpos(t)); if (r) return r; }
    return { arena: [cx, cy], cam: null, screen: null };
  });
}
async function realClick(page, fbx, fby) {
  const box = await page.locator('canvas').boundingBox();
  await page.mouse.click(box.x + fbx / 640 * box.width, box.y + fby / 360 * box.height);
  await sleep(250);
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ================= CAMPAIGN =================
  {
    const page = await newPage(browser);
    const P = padTools(page);
    ok(await enterWarMap(page), 'campaign: reached the WAR! map on the real path');
    await call(page, 'dbg_warseed', 11);
    await P.padTap(0);   // A confirms region stop 0
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'campaign: pad A launched the battle');
    await sleep(600);
    await call(page, 'dbg_waraddbits', 5000);
    const pb = await call(page, 'dbg_warbuildat', 1);
    const zp = await call(page, 'dbg_warbuildat', 2);
    ok(pb > 0 && zp > 0, 'campaign: Patch Bay + Zapper built (arrangement)', pb + '/' + zp);

    // -- capture by stick flight, stick released before A (the
    //    route that already worked — it must keep working)
    for (const [name, idx] of [['Core', 0], ['Patch Bay', pb], ['Zapper', zp]]) {
      await deselect(page, P);
      ok(await selectByStick(page, P, idx), `capture: stick flight selected the ${name}`);
      ok((await call(page, 'dbg_warbarcap')) === 1, `capture: the ${name}'s bar CAPTURED the pad`);
      ok((await call(page, 'dbg_warbarhl')) === 0, `capture: the ${name}'s highlight opened on slot 0`);
    }

    // -- capture with the stick HELD (the phantom-step regression)
    for (const [name, idx] of [['Core', 0], ['Patch Bay', pb]]) {
      await deselect(page, P);
      const r = await selectHeld(page, P, idx);
      ok(r.sel === idx && r.cap === 1, `held-stick: A mid-push selected + captured the ${name}`, JSON.stringify(r));
      ok(r.hl === 0, `held-stick: NO phantom step — the ${name}'s highlight stayed on slot 0`, 'hl=' + r.hl);
    }
    await page.screenshot({ path: __dirname + '/tsg-v125-fwarpad-capture.png' });

    // -- the D-pad: wake, flight, selection, menu steps
    await deselect(page, P);
    await realClick(page, 630, 10);   // a real mouse tap retires the pad cursor (last device wins)
    ok((await poll(page, 'dbg_warcur', v => v === -1)) === -1, 'dpad: a real tap retired the cursor');
    await P.padBtn(15, true); await sleep(350);   // DRight: wakes + flies
    const woken = await call(page, 'dbg_warcur');
    await sleep(750);
    const flown = await call(page, 'dbg_warcur');
    await P.padBtn(15, false); await sleep(200);
    ok(woken >= 0 && flown > woken + 250 * 10000, 'dpad: DRight woke the cursor and FLEW it east', woken + ' -> ' + flown);
    // closed-loop D-pad flight onto the Core
    const [hx, hy] = unxy(await call(page, 'dbg_warstructxy', 0));
    let onTarget = false;
    for (let n = 0; n < 60 && !onTarget; n++) {
      const cur = await call(page, 'dbg_warcur');
      if (cur < 0) { await P.padBtn(15, true); await sleep(100); await P.padBtn(15, false); continue; }
      const [cx, cy] = unxy(cur);
      const dx = hx - cx, dy = hy - cy;
      if (Math.abs(dx) <= 20 && Math.abs(dy) <= 20) { onTarget = true; break; }
      const btn = Math.abs(dx) >= Math.abs(dy) ? (dx > 0 ? 15 : 14) : (dy > 0 ? 13 : 12);
      const holdMs = Math.max(Math.abs(dx), Math.abs(dy)) > 70 ? 110 : 50;
      await P.padBtn(btn, true); await sleep(holdMs); await P.padBtn(btn, false); await sleep(60);
    }
    ok(onTarget, 'dpad: closed-loop D-pad flight landed on the Core');
    await P.padTap(0);
    await sleep(300);
    ok((await call(page, 'dbg_warselstruct')) === 0 && (await call(page, 'dbg_warbarcap')) === 1,
       'dpad: A on the Core selected it and CAPTURED the bar');
    ok((await call(page, 'dbg_warbarhl')) === 0, 'dpad: the highlight opened on slot 0');
    await P.padTap(15);   // DRight edge walks the captured menu
    ok((await poll(page, 'dbg_warbarhl', v => v === 1)) === 1, 'dpad: DRight stepped the captured highlight to slot 1');
    await P.padTap(14);   // DLeft walks it back
    ok((await poll(page, 'dbg_warbarhl', v => v === 0)) === 0, 'dpad: DLeft stepped it back to slot 0');
    await P.padTap(1);    // B cancels out of the selection
    ok((await poll(page, 'dbg_warbarcap', v => v === 0)) === 0 && (await call(page, 'dbg_warselstruct')) === -1,
       'dpad: B ended the capture and cleared the selection');

    // -- the centred camera: rim, corners, mid-field
    for (const [name, tx, ty] of [['west edge', 0, 600], ['NW corner', 0, 0], ['NE corner', 1600, 0], ['SE corner', 1600, 1200], ['mid-field', 800, 600]]) {
      await flyTo(page, P, tx, ty);
      await sleep(250);
      let cs = null;
      for (let n = 0; n < 10 && !(cs && cs.screen); n++) { cs = await curScreen(page); if (!(cs && cs.screen)) await sleep(250); }
      ok(!!(cs && cs.screen) && Math.abs(cs.screen[0] - 320) <= 3 && Math.abs(cs.screen[1] - 157) <= 3,
         `centred: cursor sits at the field centre at the ${name}`, JSON.stringify(cs));
      if (name === 'NW corner') await page.screenshot({ path: __dirname + '/tsg-v125-fwarpad-corner.png' });
      if (name === 'SE corner') await page.screenshot({ path: __dirname + '/tsg-v125-fwarpad-secorner.png' });
    }
    // -- the right stick flies the cursor now (the pan is gone)
    await flyTo(page, P, 700, 600);
    const before = await call(page, 'dbg_warcur');
    await P.padAxes(0, 0, 1, 0); await sleep(700); await P.padAxes(0, 0, 0, 0);
    await sleep(200);
    const after = await call(page, 'dbg_warcur');
    ok(after > before + 250 * 10000, 'right stick: it flies the cursor east (no separate pan)', before + ' -> ' + after);
    const cs = await curScreen(page);
    ok(!!(cs && cs.screen) && Math.abs(cs.screen[0] - 320) <= 3 && Math.abs(cs.screen[1] - 157) <= 3,
       'right stick: the cursor is still centred after the flight', JSON.stringify(cs));
    await page.close();
  }

  // ================= SKIRMISH =================
  {
    const page = await newPage(browser);
    const P = padTools(page);
    ok(await enterWarMap(page), 'skirmish: reached the WAR! map on the real path');
    await call(page, 'dbg_warseed', 11);
    await P.padTap(12);          // DUp wraps the selection to the SKIRMISH stop
    await P.padTap(0);           // A opens the setup panel
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, 'skirmish: setup panel opened by pad');
    await P.padTap(13); await P.padTap(13);
    await P.padTap(0);           // LAUNCH
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1 && (await call(page, 'dbg_warskirmish')) === 1,
       'skirmish: battle launched by pad');
    await sleep(600);
    await call(page, 'dbg_waraddbits', 5000);
    const pb = await call(page, 'dbg_warbuildat', 1);
    ok(await selectByStick(page, P, 0), 'skirmish capture: stick flight selected the Core');
    ok((await call(page, 'dbg_warbarcap')) === 1 && (await call(page, 'dbg_warbarhl')) === 0,
       'skirmish capture: the bar captured on slot 0');
    if (pb > 0) {
      await deselect(page, P);
      const r = await selectHeld(page, P, pb);
      ok(r.sel === pb && r.cap === 1 && r.hl === 0, 'skirmish held-stick: Patch Bay captured with NO phantom step', JSON.stringify(r));
    }
    await page.close();
  }

  await browser.close();
  console.log(`WARPadTEST: ${pass} passed, ${fail} failed`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('WARPadTEST ERROR', e.message); process.exit(1); });
