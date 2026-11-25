// warcoop.js — v1.25 FEATURE 13: WAR! LOCAL CO-OP COMMAND.
// One battle, one army, one purse, two commanders. The battle lives
// in the OWNER's BwSim (whoever launched it); the other local player
// joins as GUEST with full command — their own cursor, camera,
// selection and bar capture, over the shared field. This suite
// drives BOTH pads with real pad events only (two fake pads through
// the shell's real poll, the cartcoop idiom); dbg exports arrange
// state (seed, bits, one building) and OBSERVE only.
//
// Legs: 1 join-at-entry + the guest camera starts on the owner's;
// 2 P2 cursor parity (wake, closed-loop flight, centred camera at a
// corner and mid-field, held-stick capture on slot 0 with no phantom
// step, D-pad menu step, B peel); 3 the shared purse (P2 trains, P1
// builds — both spends read identically from both sides); 4 both
// commanders select the SAME building with independent highlights,
// and P2's peel leaves P1's selection standing; 5 pause rudeness in
// both directions (only the pauser drives or resumes); 6 the guest
// drops out through the BW Start-menu LEAVE and the battle fights
// on solo; 7 the owner SAVE & QUITs — both land on their own maps,
// the battle parks, and P1 resumes it solo once P2 has left WAR!
// (a commander standing at the map would be pulled in by the same
// rule as a fresh launch — leg 8 proves that rule); 8 skirmish
// pull-in from a partner waiting at the map; 9 symmetry on a fresh
// page: P2 launches and OWNS, P1 guests and commands with pad 1
// while P2 commands with pad 2.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
const unxy = v => [Math.floor(v / 10000), v % 10000];

async function newPage(browser) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    function mkpad(index) {
      const buttons = [];
      for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
      return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    }
    window.__pads = [mkpad(0), mkpad(1)];
    navigator.getGamepads = () => window.__pads;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
    window.__axes = (pi, a) => { window.__pads[pi].axes = a; };
  });
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  return page;
}
const call = (page, fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
async function poll(page, fn, want, timeout = 8000, args = []) {
  const t0 = Date.now();
  while (Date.now() - t0 < timeout) { const v = await call(page, fn, ...args); if (want(v)) return v; await sleep(150); }
  return call(page, fn, ...args);
}
const key = async (page, k, ms = 150) => { await page.keyboard.down(k); await sleep(120); await page.keyboard.up(k); await sleep(ms); };
// pad tools, parameterized by pad index (0 = P1, 1 = P2)
function pads(page) {
  const btn = (pi, bi, down) => page.evaluate(([p, n, d]) => window.__btn(p, n, d), [pi, bi, down]);
  const tap = async (pi, bi, holdMs = 130) => { await btn(pi, bi, true); await sleep(holdMs); await btn(pi, bi, false); await sleep(150); };
  const axes = (pi, lx, ly, rx = 0, ry = 0) => page.evaluate(([p, a]) => window.__axes(p, a), [pi, [lx, ly, rx, ry]]);
  return { btn, tap, axes };
}
// closed-loop stick flight for either commander, on their own cursor reader
async function flyTo(page, P, pi, curFn, tx, ty, tol = 9, maxMs = 12000) {
  const t0 = Date.now();
  while (Date.now() - t0 < maxMs) {
    const cur = await call(page, curFn);
    if (cur < 0) { await P.axes(pi, 0.6, 0); await sleep(150); continue; }
    const [cx, cy] = unxy(cur);
    const dx = tx - cx, dy = ty - cy;
    if (Math.abs(dx) <= tol && Math.abs(dy) <= tol) { await P.axes(pi, 0, 0); return [cx, cy]; }
    const cl = v => Math.max(-1, Math.min(1, v / 55));
    await P.axes(pi, cl(dx), cl(dy));
    await sleep(110);
  }
  await P.axes(pi, 0, 0);
  const cur = await call(page, curFn);
  return cur >= 0 ? unxy(cur) : [-1, -1];
}
// walk a captured highlight to a slot with DRight taps, verify-and-
// retry (a press can fall between frames under load — the standing
// idiom; the assertion that follows still demands the exact slot)
async function stepTo(page, P, pi, hlFn, want) {
  for (let t = 0; t < 4; t++) {
    if ((await call(page, hlFn)) === want) return true;
    await P.tap(pi, 15);
  }
  return (await call(page, hlFn)) === want;
}
// P2 from their Arcade cabinet into WAR!, verify-and-retry on the
// panel pick (same idiom). v1.27: the WAR! cabinet stand point is
// (ARC_SX[6], ARC_CY + ARC_ROWY[6] + 58) = (0, -12000+184+58) =
// (0,-11758) — the cabinet rose 70px (ROWY 254 -> 184); every
// dbg_tp/dbg_tp2 to it in this suite uses the new point.
async function p2Cabinet(page, P) {
  for (let t = 0; t < 4; t++) {
    if ((await call(page, 'dbg_war2state')) !== -1) return true;
    await call(page, 'dbg_tp2', 0, -11758); await sleep(250);
    await P.tap(1, 1, 180); await sleep(350);
    await P.tap(1, 0, 180); await sleep(350);
  }
  return (await call(page, 'dbg_war2state')) !== -1;
}
async function deselect(page, P, pi, selFn, capFn) {
  for (let i = 0; i < 4; i++) {
    if ((await call(page, selFn)) === -1 && (await call(page, capFn)) === 0) break;
    await P.tap(pi, 1);   // B peels / cancels
  }
  await sleep(200);
}
// fly onto a building and press A with the stick STILL PUSHED toward
// it, then keep holding — the phantom-step regression posture
async function selectHeld(page, P, pi, curFn, selFn, capFn, hlFn, sidx, xyFn = 'dbg_warstructxy') {
  const [sx, sy] = unxy(await call(page, xyFn, sidx));
  await flyTo(page, P, pi, curFn, sx, sy);
  const cur = unxy(await call(page, curFn));
  let dx = sx - cur[0], dy = sy - cur[1];
  if (dx === 0 && dy === 0) dx = 1;
  const m = Math.max(Math.abs(dx), Math.abs(dy), 1);
  await P.axes(pi, dx / m, dy / m);
  await P.tap(pi, 0);
  await sleep(400);   // still holding: the phantom step fired on the entry frame + 1
  const r = { sel: await call(page, selFn), cap: await call(page, capFn), hl: await call(page, hlFn) };
  await P.axes(pi, 0, 0);
  return r;
}
async function startCoop(page, P) {
  await sleep(800);   // let the pad/keyboard flavour latch settle (cartcoop's rule)
  const mc = await call(page, 'dbg_menucount');
  const coopRow = mc - 1;
  for (let i = 0; i < coopRow; i++) await key(page, 'ArrowDown', 160);
  for (let t = 0; t < 4 && (await call(page, 'dbg_coop')) !== 1; t++) await key(page, 'Space', 300);
  if ((await call(page, 'dbg_coop')) !== 1) return false;
  for (let i = 0; i < coopRow; i++) await key(page, 'ArrowUp', 160);
  for (let t = 0; t < 4 && (await call(page, 'dbg_state')) !== 1; t++) { await key(page, 'Space', 400); if ((await call(page, 'dbg_state')) !== 1) await key(page, 'Enter', 400); }
  if ((await call(page, 'dbg_state')) !== 1) return false;
  await P.tap(1, 0, 160);
  if ((await poll(page, 'dbg_p2', v => v > 0, 6000)) <= 0) return false;
  await call(page, 'dbg_adopt', 0);
  await call(page, 'dbg_adopt2', 0);
  await sleep(400);
  return true;
}
// overworld -> mall -> arcade -> WAR! cabinet, per player, on their own side
async function enterWar(page, P, pi) {
  if (pi === 0) {
    await call(page, 'dbg_tp', 0, 80); await key(page, 'KeyE', 250); await sleep(300);
    await call(page, 'dbg_tp', 350, -5976); await key(page, 'KeyE', 250);
    if ((await poll(page, 'dbg_inarcade', v => v === 1, 5000)) !== 1) return false;
    await call(page, 'dbg_tp', 0, -11758); await sleep(250);
    if ((await call(page, 'dbg_cabinet')) !== 6) return false;
    await key(page, 'KeyE', 250);
    if ((await poll(page, 'dbg_arcgate', v => v === 1, 5000)) !== 1) return false;
    await key(page, 'Space', 300);
    return (await poll(page, 'dbg_bwmode', v => v === 8, 5000)) === 8;
  }
  await call(page, 'dbg_tp2', 0, 80); await P.tap(1, 1, 180); await sleep(400);
  await call(page, 'dbg_tp2', 350, -5976); await P.tap(1, 1, 180); await sleep(400);
  await call(page, 'dbg_tp2', 0, -11758); await sleep(250);
  await P.tap(1, 1, 180); await sleep(400);   // P2's own cabinet panel
  await P.tap(1, 0, 180);                      // pad A picks PLAY on it
  return true;   // the caller polls the mode-specific reader
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ================= P1 OWNS, P2 GUESTS =================
  {
    const page = await newPage(browser);
    const P = pads(page);
    ok(await startCoop(page, P), 'setup: co-op run started, P2 dropped in');
    const p2slot = await call(page, 'dbg_p2');

    // P1 walks the real path and launches a campaign battle
    ok(await enterWar(page, P, 0), 'P1 reached the WAR! cabinet');
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'P1 stands at the WAR! map');
    await call(page, 'dbg_warseed', 11);
    await P.tap(0, 0);   // pad A confirms region stop 0
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'P1 launched the battle (owner)');
    await sleep(700);   // entry grace
    ok((await call(page, 'dbg_warduo')) === -1, 'no link yet: P2 is not at WAR!');
    // park P1's camera somewhere known; the joining guest starts there
    await call(page, 'dbg_warcam', 500, 400);
    await sleep(250);

    // ---- leg 1: join at entry
    ok(await enterWar(page, P, 1), 'P2 walked their own path to the WAR! cabinet');
    ok((await poll(page, 'dbg_warlink2', v => v === 1)) === 1, 'leg1: P2 joined as GUEST of P1\'s battle', await call(page, 'dbg_warlink2'));
    ok((await call(page, 'dbg_warduo')) === p2slot, 'leg1: P1\'s sim names P2 as co-commander', await call(page, 'dbg_warduo'));
    ok((await call(page, 'dbg_war2state')) === 1, 'leg1: P2 sees the live battle, not a map of their own');
    ok((await call(page, 'dbg_war2cam')) === 500 * 10000 + 400, 'leg1: the guest camera starts on the owner\'s camera', await call(page, 'dbg_war2cam'));
    ok((await call(page, 'dbg_war2cur')) === -1, 'leg1: the guest cursor starts off — it wakes on P2\'s own stick');

    // ---- leg 2: P2 cursor parity
    const [corex, corey] = unxy(await call(page, 'dbg_warstructxy', 0));
    const landed = await flyTo(page, P, 1, 'dbg_war2cur', corex, corey);
    ok(Math.abs(landed[0] - corex) <= 12 && Math.abs(landed[1] - corey) <= 12, 'leg2: pad 2 stick flight landed P2\'s cursor on the Core', landed.join(','));
    { // the centred rule, in P2's own view: camera == cursor
      const cam = unxy(await call(page, 'dbg_war2cam')), cur = unxy(await call(page, 'dbg_war2cur'));
      ok(Math.abs(cam[0] - cur[0]) <= 3 && Math.abs(cam[1] - cur[1]) <= 3, 'leg2: P2\'s cursor is the centre of P2\'s view (cam == cur)', cam.join(',') + ' vs ' + cur.join(','));
    }
    await flyTo(page, P, 1, 'dbg_war2cur', 0, 0);
    { const cam = unxy(await call(page, 'dbg_war2cam')), cur = unxy(await call(page, 'dbg_war2cur'));
      ok(Math.abs(cam[0] - cur[0]) <= 3 && Math.abs(cam[1] - cur[1]) <= 3 && cur[0] <= 12 && cur[1] <= 12,
         'leg2: centred at the NW corner too — the view scrolls off-map, the cursor stays centred', cam.join(',') + ' vs ' + cur.join(','));
    }
    const r2 = await selectHeld(page, P, 1, 'dbg_war2cur', 'dbg_war2selstruct', 'dbg_war2barcap', 'dbg_war2barhl', 0, 'dbg_war2structxy');
    ok(r2.sel === 0 && r2.cap === 1, 'leg2: held-stick A selected the Core and CAPTURED P2\'s bar', JSON.stringify(r2));
    ok(r2.hl === 0, 'leg2: NO phantom step on pad 2 — the highlight stayed on slot 0', 'hl=' + r2.hl);
    ok(await stepTo(page, P, 1, 'dbg_war2barhl', 1), 'leg2: pad 2 D-pad stepped the captured highlight to slot 1');
    await P.tap(1, 1);    // B peels out of the selection
    ok((await poll(page, 'dbg_war2barcap', v => v === 0)) === 0 && (await call(page, 'dbg_war2selstruct')) === -1,
       'leg2: pad 2 B ended the capture and cleared P2\'s selection');
    // both cursors live at once, for the screenshot
    await flyTo(page, P, 1, 'dbg_war2cur', 1050, 350);
    await flyTo(page, P, 0, 'dbg_warcur', 500, 800);
    await sleep(300);
    await page.screenshot({ path: __dirname + '/tsg-v125-f13-split.png' });

    // ---- leg 3: the shared purse
    await call(page, 'dbg_waraddbits', 5000);
    const pb = await call(page, 'dbg_warbuildat', 1);
    ok(pb > 0, 'leg3: a Patch Bay stands (arrangement)', 'idx=' + pb);
    // economy isolation (the wartest idiom): dead harvesters deliver
    // nothing, so the purse is static and every spend reads exactly
    for (let k = 0; k < 6; k++) { const h = await call(page, 'dbg_warfind', 0); if (h < 0) break; await call(page, 'dbg_warhurt', 0, h, 99999); }
    await sleep(300);
    const bits0 = await call(page, 'dbg_warbits');
    ok((await call(page, 'dbg_war2bits')) === bits0, 'leg3: the guest reads the same purse as the owner', bits0 + '');
    // P2 trains a WRENCH at the Patch Bay (single category, slot 0)
    const [pbx, pby] = unxy(await call(page, 'dbg_warstructxy', pb));
    await flyTo(page, P, 1, 'dbg_war2cur', pbx, pby);
    await P.tap(1, 0); await sleep(300);
    ok((await call(page, 'dbg_war2selstruct')) === pb && (await call(page, 'dbg_war2barcap')) === 1, 'leg3: P2 selected the Patch Bay, bar captured');
    await P.tap(1, 0);   // A on slot 0 queues the WRENCH (50 bits)
    ok((await poll(page, 'dbg_warqueue', v => v === 1, 6000, [pb])) === 1, 'leg3: P2\'s train order queued at the shared Patch Bay');
    ok((await call(page, 'dbg_warbits')) === bits0 - 50, 'leg3: the owner\'s purse paid for P2\'s order', bits0 + ' -> ' + (await call(page, 'dbg_warbits')));
    ok((await call(page, 'dbg_war2bits')) === bits0 - 50, 'leg3: ...and the guest\'s read of the purse agrees');
    await deselect(page, P, 1, 'dbg_war2selstruct', 'dbg_war2barcap');
    // P1 builds a second Patch Bay beside the Core, on real input
    const structs1 = await call(page, 'dbg_warstructs', 1);
    await flyTo(page, P, 0, 'dbg_warcur', corex, corey);
    await P.tap(0, 0); await sleep(300);
    ok((await call(page, 'dbg_warselstruct')) === 0 && (await call(page, 'dbg_warbarcap')) === 1, 'leg3: P1 selected the Core, bar captured');
    ok(await stepTo(page, P, 0, 'dbg_warbarhl', 1), 'leg3: P1 walked the highlight to BUILD');
    await P.tap(0, 0);    // open BUILD (slot 0 = PATCH BAY)
    await P.tap(0, 0);    // arm the placement
    ok((await poll(page, 'dbg_warplace', v => v >= 0)) >= 0, 'leg3: P1 armed a Patch Bay placement');
    let placed = false;
    for (const [ox, oy] of [[95, 0], [95, -80], [-95, 0], [0, 110]]) {
      await flyTo(page, P, 0, 'dbg_warcur', corex + ox, corey + oy);
      await P.tap(0, 0); await sleep(300);
      if ((await call(page, 'dbg_warstructs', 1)) === structs1 + 1) { placed = true; break; }
    }
    ok(placed, 'leg3: P1 placed the building on the shared field');
    ok((await call(page, 'dbg_war2bits')) === bits0 - 200, 'leg3: the guest\'s purse read dropped by the build cost too', await call(page, 'dbg_war2bits'));
    await deselect(page, P, 0, 'dbg_warselstruct', 'dbg_warbarcap');

    // ---- leg 4: the same building, two commanders
    await flyTo(page, P, 0, 'dbg_warcur', corex, corey);
    await P.tap(0, 0); await sleep(300);
    ok((await call(page, 'dbg_warselstruct')) === 0 && await stepTo(page, P, 0, 'dbg_warbarhl', 1), 'leg4: P1 holds the Core, highlight on slot 1');
    const r4 = await selectHeld(page, P, 1, 'dbg_war2cur', 'dbg_war2selstruct', 'dbg_war2barcap', 'dbg_war2barhl', 0, 'dbg_war2structxy');
    ok(r4.sel === 0 && r4.cap === 1 && r4.hl === 0, 'leg4: P2 selected the SAME Core — their own capture, highlight on slot 0', JSON.stringify(r4));
    ok((await call(page, 'dbg_warbarhl')) === 1, 'leg4: P1\'s highlight never moved');
    await page.screenshot({ path: __dirname + '/tsg-v125-f13-dual.png' });
    await P.tap(1, 1);   // P2 peels out
    ok((await poll(page, 'dbg_war2selstruct', v => v === -1)) === -1, 'leg4: P2 peeled out of the selection');
    ok((await call(page, 'dbg_warselstruct')) === 0 && (await call(page, 'dbg_warbarcap')) === 1, 'leg4: P1\'s selection and capture stand untouched');
    await deselect(page, P, 0, 'dbg_warselstruct', 'dbg_warbarcap');

    // ---- leg 5: pause rudeness, both directions
    await flyTo(page, P, 1, 'dbg_war2cur', 600, 500);
    const curBefore = await call(page, 'dbg_war2cur');
    await key(page, 'Escape', 250);
    ok((await poll(page, 'dbg_warpause', v => v === 1)) === 1, 'leg5: P1 opened the shared pause');
    ok((await call(page, 'dbg_war2pause')) === 1, 'leg5: the guest\'s side reads the same pause');
    await P.axes(1, 1, 0); await sleep(700); await P.axes(1, 0, 0); await sleep(200);
    ok((await call(page, 'dbg_war2cur')) === curBefore, 'leg5: P2\'s stick cannot command through P1\'s pause', curBefore + ' -> ' + (await call(page, 'dbg_war2cur')));
    await P.tap(1, 8);   // P2's Back does not resume someone else's pause
    await sleep(300);
    ok((await call(page, 'dbg_warpause')) === 1, 'leg5: P2 cannot resume P1\'s pause either');
    await key(page, 'Escape', 250);
    ok((await poll(page, 'dbg_warpause', v => v === 0)) === 0, 'leg5: the pauser resumes it');
    await P.tap(1, 8);   // P2's Back opens the pause this time
    ok((await poll(page, 'dbg_warpause', v => v === 1)) === 1, 'leg5: P2 opened the shared pause from pad 2');
    await key(page, 'Escape', 250);
    await sleep(300);
    ok((await call(page, 'dbg_warpause')) === 1, 'leg5: P1 cannot resume P2\'s pause');
    await key(page, 'Digit1', 200);   // P1's RESUME key is eaten, not queued
    await sleep(300);
    ok((await call(page, 'dbg_warpause')) === 1, 'leg5: P1\'s menu keys do not drive P2\'s pause');
    await P.tap(1, 8);
    ok((await poll(page, 'dbg_warpause', v => v === 0)) === 0, 'leg5: P2 resumes their own pause');
    await sleep(400);
    ok((await call(page, 'dbg_warpause')) === 0 && (await call(page, 'dbg_warstate')) === 1, 'leg5: nothing leaked into the resume frame — the battle simply fights on');

    // ---- leg 6: the guest drops out mid-battle
    await P.tap(1, 9);   // Start: P2's BW menu (RESUME / CONTROLS / LEAVE)
    await sleep(400);
    await P.tap(1, 13); await P.tap(1, 13);   // walk to LEAVE
    await P.tap(1, 0);                        // A picks it: bw_exit
    ok((await poll(page, 'dbg_warlink2', v => v === 0, 6000)) === 0, 'leg6: the link dissolved when the guest left WAR!');
    ok((await call(page, 'dbg_warduo')) === -1, 'leg6: the owner fights solo now');
    ok((await call(page, 'dbg_warstate')) === 1, 'leg6: the battle itself never blinked');
    ok((await call(page, 'dbg_bwstate2')) === 0, 'leg6: P2 is back at their Arcade cabinet');

    // ---- leg 7: owner SAVE & QUIT; solo resume
    // P2 re-joins first (entry join), so the SAVE & QUIT has a guest to release
    ok(await p2Cabinet(page, P), 'leg7: P2 back at the WAR! cabinet');
    ok((await poll(page, 'dbg_warlink2', v => v === 1, 6000)) === 1, 'leg7: P2 re-joined the battle at the cabinet');
    await key(page, 'Escape', 250);
    ok((await poll(page, 'dbg_warpause', v => v === 1)) === 1, 'leg7: P1 opened the pause');
    await key(page, 'Digit3', 250);   // SAVE & QUIT
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'leg7: P1 is back on their own map');
    ok((await call(page, 'dbg_warsnap')) === 1, 'leg7: the battle parked');
    ok((await poll(page, 'dbg_warlink2', v => v === 0, 6000)) === 0, 'leg7: the guest was released by the save');
    ok((await call(page, 'dbg_war2state')) === 0, 'leg7: P2 stands at their own map, battle-less');
    // a commander standing at the map would be pulled into a resume
    // (leg 8's rule) — so P2 steps out of WAR! first
    await P.tap(1, 8);   // Back on the map leaves the mode
    ok((await poll(page, 'dbg_bwstate2', v => v === 0, 6000)) === 0, 'leg7: P2 stepped out of WAR!');
    await key(page, 'Space', 300);   // P1 relaunches: the parked battle restores
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'leg7: P1 resumed the parked battle');
    ok((await call(page, 'dbg_warduo')) === -1, 'leg7: ...solo, with nobody to pull in');
    // park it again for leg 8
    await key(page, 'Escape', 250);
    await key(page, 'Digit3', 250);
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'leg7: parked again, P1 on the map');

    // ---- leg 8: skirmish pull-in
    ok(await p2Cabinet(page, P), 'leg8: P2 back at the WAR! cabinet');
    ok((await poll(page, 'dbg_war2state', v => v === 0, 6000)) === 0, 'leg8: P2 waits at their own WAR! map (no live battle to join)');
    ok((await call(page, 'dbg_warlink2')) === 0, 'leg8: no link while P1 is only parked');
    // open the setup panel, verify-and-retry (an eaten DUp would
    // otherwise launch a region with the following A)
    let panel = false;
    for (let t = 0; t < 3 && !panel; t++) {
      await P.tap(0, 12);        // DUp wraps P1's map selection to the SKIRMISH stop
      await P.tap(0, 0);         // A opens the setup panel
      panel = (await poll(page, 'dbg_warstate', v => v === 4, 2500)) === 4;
    }
    ok(panel, 'leg8: the skirmish setup panel opened');
    await P.tap(0, 13); await P.tap(0, 13); await P.tap(0, 13);   // down to LAUNCH (row 3 since v1.33's CLANS row joined the panel)
    await P.tap(0, 0);
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1 && (await call(page, 'dbg_warskirmish')) === 1, 'leg8: P1 launched the skirmish');
    ok((await poll(page, 'dbg_warlink2', v => v === 1, 6000)) === 1, 'leg8: the launch PULLED P2 in as guest');
    ok((await call(page, 'dbg_war2state')) === 1, 'leg8: P2 is in the skirmish, not at their map');
    await key(page, 'Escape', 250);
    await key(page, 'Digit3', 250);   // skirmish QUIT
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'leg8: the skirmish is abandoned');
    ok((await poll(page, 'dbg_warlink2', v => v === 0, 6000)) === 0, 'leg8: quitting released the guest too');
    ok((await call(page, 'dbg_war2state')) === 0, 'leg8: P2 is back at their own map');
    await page.close();
  }

  // ================= P2 OWNS, P1 GUESTS (symmetry) =================
  {
    const page = await newPage(browser);
    const P = pads(page);
    ok(await startCoop(page, P), 'sym: co-op run started, P2 dropped in');
    // P2 walks the path and launches first — P2 OWNS this battle
    ok(await enterWar(page, P, 1), 'sym: P2 reached the WAR! cabinet');
    ok((await poll(page, 'dbg_war2state', v => v === 0, 6000)) === 0, 'sym: P2 stands at the WAR! map');
    await sleep(800);   // entry grace, then pad 2 A launches region stop 0 (verify-and-retry)
    let launched = false;
    for (let t = 0; t < 3 && !launched; t++) {
      await P.tap(1, 0);
      launched = (await poll(page, 'dbg_war2state', v => v === 1, 2500)) === 1;
    }
    ok(launched, 'sym: P2 launched the battle (owner)');
    await sleep(700);
    // economy isolation on the owner's side, so the purse reads exact
    for (let k = 0; k < 6; k++) { const h = await call(page, 'dbg_war2find', 0); if (h < 0) break; await call(page, 'dbg_war2hurt', 0, h, 99999); }
    await sleep(300);
    ok((await call(page, 'dbg_warduo2')) === -1, 'sym: no guest yet');
    // P1 enters WAR! and joins as the guest
    ok(await enterWar(page, P, 0), 'sym: P1 reached the WAR! cabinet');
    ok((await poll(page, 'dbg_warduo2', v => v === 0, 6000)) === 0, 'sym: P1 joined as P2\'s guest', await call(page, 'dbg_warduo2'));
    // P1 commands with pad 1: held-stick capture of the Core
    const r9 = await selectHeld(page, P, 0, 'dbg_warcur', 'dbg_warselstruct', 'dbg_warbarcap', 'dbg_warbarhl', 0, 'dbg_war2structxy');
    ok(r9.sel === 0 && r9.cap === 1 && r9.hl === 0, 'sym: the P1 guest captured the Core on slot 0, no phantom step', JSON.stringify(r9));
    const bits9 = await call(page, 'dbg_war2bits');
    await P.tap(0, 0);   // open TRAIN
    await P.tap(0, 0);   // train a HARVESTER (80 bits from the shared purse)
    ok((await poll(page, 'dbg_war2bits', v => v === bits9 - 80, 6000)) === bits9 - 80, 'sym: P1\'s train order charged the owner\'s purse', bits9 + ' -> ' + (await call(page, 'dbg_war2bits')));
    // P2 commands with pad 2, same battle, same building
    const [corex, corey] = unxy(await call(page, 'dbg_war2structxy', 0));
    await flyTo(page, P, 1, 'dbg_war2cur', corex, corey);
    await P.tap(1, 0); await sleep(300);
    ok((await call(page, 'dbg_war2selstruct')) === 0 && (await call(page, 'dbg_war2barcap')) === 1, 'sym: the P2 owner holds the same Core in their own view');
    ok(await stepTo(page, P, 1, 'dbg_war2barhl', 1) && (await call(page, 'dbg_warbarhl')) === 0, 'sym: the owner\'s highlight steps while the guest\'s stays on slot 0 — two views, one battle');
    await page.screenshot({ path: __dirname + '/tsg-v125-f13-sym.png' });
    await page.close();
  }

  await browser.close();
  console.log(`WARCOOP: ${pass} passed, ${fail} failed`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('WARCOOP ERROR', e.message); process.exit(1); });
