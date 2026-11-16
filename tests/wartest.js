// wartest.js — WAR! (Nov 15): the seventh arcade cabinet, a tap-first
// RTS. World map -> battle for region 1: harvest loop fills the purse,
// the command bar trains through a real tap flow, buildings go up,
// double-tap selects every unit of a type, a zapper tower kills a bug,
// the hive falls, the region is conquered — and the conquest survives
// a save/Continue round trip (the Buddy's wr key).
// dbg packings (see survival.c): dbg_warunits(side,type) counts alive
// units (side 0 buddies / 1 bugs; types 0 harvester, 1 melee, 2 ranged,
// 3 crawler, 4 spitter, 5 scout buggy, 6 siege tank; -1 = any).
// dbg_warunitxy / dbg_warstructxy pack arena-local coords as
// x*10000+y; dbg_warunitpos packs on-screen fb coords the same way.
// Struct 0 is always the Compiler Core; the hive of a fresh battle is
// struct 1. Round 2 (Nov 16) adds: dbg_warplace (armed build kind),
// dbg_warterrain (feature count), dbg_warfog(x,y) (0 black / 1 explored
// / 2 visible), dbg_warmove(i,x,y) (teleport), dbg_warbuildxy (exact-
// spot build, walls grid-snapped, property bypassed), and touch_point
// drags for the pan checks. Struct kinds: 1 patch, 2 zapper, 3 wall,
// 5 assembly plant. Round 3 (Nov 16) adds the debris hooks:
// dbg_warfeatn() (features still standing), dbg_warfeat(t)
// (hp*100 + kind; kinds 0 tree / 1 rock / 2 terminal / 3 spool /
// 4 rubble), dbg_warfeatxy(t) and dbg_warfeatpos(t) (packed coords,
// same conventions as the unit ones). v1.17 "SUPPLY LINES" adds:
// dbg_warpause, dbg_warsnap, dbg_warregion, dbg_warbcat (-1 top
// level / 0 TRAIN / 1 BUILD / 2 DEFENSE), dbg_warnodes (count*100000
// + bits left on the field), dbg_warcarry (fullest harvester load),
// dbg_warqueue(struct) (training-queue depth), dbg_warrally(struct)
// (packed rally point, -1 = unset). The bar is two-level now: the
// bar row shows categories (slots at y=337), the open category's
// options live in the strip directly above it (slots at y=291).
const { chromium } = require('playwright-core');
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
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  // a fake standard-mapping gamepad (v1.16 pad checks): the REAL
  // shell poll reads it every frame, so buttons flow through EX.act
  // and the analog sticks through EX.pad_axes, exactly like hardware.
  // The test steers it by mutating window.__FAKEPAD.
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
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); };
  const hold = async (k, ms = 200) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(150); };
  const tap = async (x, y) => { await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [x, y]); await sleep(250); };
  // the fake pad: axes are (LX, LY, RX, RY) in -1..1 (+Y down);
  // buttons use the standard mapping (0 A, 1 B, 4 LB, 5 RB). A press
  // is HELD across many frames — this VM drops instant presses.
  const padAxes = async (lx, ly, rx, ry) => page.evaluate(([a, b, c, d]) => {
    window.__FAKEPAD.axes[0] = a; window.__FAKEPAD.axes[1] = b; window.__FAKEPAD.axes[2] = c; window.__FAKEPAD.axes[3] = d;
  }, [lx, ly, rx, ry]);
  const padBtn = async (i, down) => page.evaluate(([n, d]) => {
    const b = window.__FAKEPAD.buttons[n]; b.pressed = d; b.value = d ? 1 : 0;
  }, [i, down]);
  const padTap = async (i, holdMs = 120) => { await padBtn(i, true); await sleep(holdMs); await padBtn(i, false); await sleep(60); };
  async function poll(fn, args, want, timeout = 9000, step = 150) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }
  const unxy = v => [Math.floor(v / 10000), v % 10000];        // world packings: x*10000+y
  const unpos = v => [v % 10000, Math.floor(v / 10000)];       // screen packing: x + y*10000

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_adopt', 0);
  ok((await call('dbg_buddy')) !== 0, 'buddy adopted');

  // mall -> arcade -> the brand-new seventh cabinet
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', [], v => v === 1)) === 1, 'in the mall');
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  ok((await poll('dbg_inarcade', [], v => v === 1)) === 1, 'in the arcade');
  await call('dbg_tp', 256, -11898); await sleep(200);
  ok((await call('dbg_cabinet')) === 5, 'the CART cabinet is still cabinet 5', await call('dbg_cabinet'));
  await call('dbg_tp', 0, -11688); await sleep(200);
  ok((await call('dbg_cabinet')) === 6, 'standing at the WAR! cabinet (cabinet 6, third row)', await call('dbg_cabinet'));
  await key('KeyE', 250);
  ok((await poll('dbg_arcgate', [], v => v === 1)) === 1, 'WAR! cabinet panel opened');
  await key('Space', 300);
  ok((await poll('dbg_bwmode', [], v => v === 8)) === 8, 'WAR! launched (bw_mode 8)');
  ok((await call('dbg_warstate')) === 0, 'world map is the home state');
  ok((await call('dbg_warconq')) === 0, 'no regions conquered yet');
  await sleep(700);   // entry grace
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/war-map.png' });

  // launch region 1 by tapping its node (top node of the ring:
  // centre (320,188), region 0 at angle -90 -> (320, 73))
  await tap(320, 73);
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1, 'tapping region 1 launched its battle');
  ok((await call('dbg_warbits')) === 250, 'v1.17 check 1: starting purse is 250 bits', await call('dbg_warbits'));
  ok((await call('dbg_warnodes')) === 904800, 'v1.17 check 1: nine bit nodes totalling 4,800 bits on the field', await call('dbg_warnodes'));
  ok((await call('dbg_warunits', 0, 0)) === 1, 'one harvester at the start');
  ok((await call('dbg_warunits', 0, 1)) === 1, 'one melee buddy at the start');
  ok((await call('dbg_warunits', 0, 2)) === 0, 'no ranged buddies yet');
  ok((await call('dbg_warhives')) === 1, 'region 1 has exactly one hive');
  ok((await call('dbg_warstructs', 0)) === 1, 'the Compiler Core stands');
  await cv.screenshot({ path: __dirname + '/war-battle.png' });

  // the real tap flow: tap the Core (camera opens on it: world
  // (280,600) -> screen (320,157)), then — v1.17's categorized bar —
  // tap TRAIN in the bar and HARVESTER in the strip above it
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'tapping the Core selects it');
  ok((await call('dbg_warbcat')) === -1, 'v1.17 check 10: the Core bar opens at the category level');
  await tap(48, 337);   // bar slot 0: the TRAIN category
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.17 check 10: TRAIN opened its options in the strip above the bar');
  await cv.screenshot({ path: __dirname + '/war-bar.png' });
  const bitsBeforeTrain = await call('dbg_warbits');
  await tap(48, 291);   // strip slot 0: HARVESTER, 80 bits
  // read the purse at once: the charge is instant, and the window is
  // too short for more than one +25 delivery to land inside it
  ok((await call('dbg_warbits')) <= bitsBeforeTrain - 55, 'training charged the purse (80, less at most one delivery)', await call('dbg_warbits'));
  ok((await poll('dbg_warunits', [0, 0], v => v === 2, 14000)) === 2, 'the bar trained a second harvester');
  // check 10, second half: DEFENSE opens the strip too, and the GATE
  // sits in it at 35 bits
  await tap(228, 337);   // bar slot 2: the DEFENSE category
  ok((await poll('dbg_warbcat', [], v => v === 2, 4000)) === 2, 'v1.17 check 10: DEFENSE opened above the bar');
  await tap(228, 291);   // strip slot 2: GATE
  ok((await poll('dbg_warplace', [], v => v === 6, 4000)) === 6, 'v1.17 check 10: GATE is in DEFENSE (35 bits) and arms for placement');
  await tap(48, 337);   // while placing, the bar is [CANCEL]: stand it down
  ok((await poll('dbg_warplace', [], v => v === -1, 4000)) === -1, 'v1.17 check 10: CANCEL stood the gate placement down');

  // v1.17 (addendum 3): the Core is a PRODUCER — tapping empty
  // ground with it selected plants its rally flag instead of
  // deselecting (the plain deselect case lives in check 19, on a
  // Zapper, which is not a producer)
  await call('dbg_warcam', 500, 300); await sleep(200);
  await tap(320, 157);   // world (500,300): empty ground
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'rally: the Core STAYS selected after a field tap');
  ok((await call('dbg_warrally', 0)) === 5000300, 'rally: the field tap set the Core rally point', await call('dbg_warrally', 0));

  // the harvest loop pays: two harvesters now work the near nodes
  const bits0 = await call('dbg_warbits');
  ok((await poll('dbg_warbits', [], v => v >= bits0 + 20, 45000, 500)) >= bits0 + 20,
     'harvesters looped node -> Core and the purse grew', 'bits stuck at ' + (await call('dbg_warbits')));

  // buildings via the dbg placement hook (same validity + charging
  // code path as tap placement)
  await call('dbg_waraddbits', 400);
  const pb = await call('dbg_warbuildat', 1);
  ok(pb > 0 && (await call('dbg_warstructs', 1)) === 1, 'Patch Bay built', 'idx=' + pb);
  const zp = await call('dbg_warbuildat', 2);
  ok(zp > 0 && (await call('dbg_warstructs', 2)) === 1, 'Zapper Tower built', 'idx=' + zp);
  const fw = await call('dbg_warbuildat', 3);
  ok(fw > 0 && (await call('dbg_warstructs', 3)) === 1, 'Firewall built', 'idx=' + fw);

  // unit production via the dbg trainer (same cost + spawn path)
  ok((await call('dbg_wartrain', 1)) === 2, 'a second melee buddy trained');
  ok((await call('dbg_wartrain', 2)) === 1, 'a ranged bolt buddy trained');

  // tap-select a melee buddy for real: centre the camera on it and
  // tap its exact screen position
  if ((await call('dbg_warunits', 0, 1)) < 2) { await call('dbg_waraddbits', 200); await call('dbg_wartrain', 1); }
  let mi = await call('dbg_warfind', 1);
  ok(mi >= 0, 'found a melee buddy to select', mi);
  let [mx, my] = unxy(await call('dbg_warunitxy', mi));
  await call('dbg_warcam', mx, my); await sleep(250);
  let mp = await call('dbg_warunitpos', mi);
  let [msx, msy] = unpos(mp);
  await tap(msx, msy);
  if ((await call('dbg_warsel')) !== 1) {   // it may have stepped off the tap: one retry on its fresh position
    mp = await call('dbg_warunitpos', mi); [msx, msy] = unpos(mp);
    await tap(msx, msy);
  }
  ok((await call('dbg_warsel')) === 1, 'a tap selected exactly one unit', await call('dbg_warsel'));

  // move order: tap open ground south of it and watch it go
  const before = await call('dbg_warunitxy', mi);
  mp = await call('dbg_warunitpos', mi); [msx, msy] = unpos(mp);
  await tap(msx, Math.min(msy + 95, 300));
  const moved = await poll('dbg_warunitxy', [mi], v => {
    if (v < 0) return false;
    const [ax, ay] = unxy(before), [bx, by] = unxy(v);
    return Math.abs(bx - ax) + Math.abs(by - ay) > 40;
  }, 7000, 200);
  ok(moved !== before && moved >= 0, 'the move order marched it off', before + ' -> ' + moved);

  // double-tap: two quick taps on the same melee buddy select them all
  mi = await call('dbg_warfind', 1);
  [mx, my] = unxy(await call('dbg_warunitxy', mi));
  await call('dbg_warcam', mx, my); await sleep(250);
  mp = await call('dbg_warunitpos', mi); [msx, msy] = unpos(mp);
  await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [msx, msy]);
  await sleep(150);
  mp = await call('dbg_warunitpos', mi); [msx, msy] = unpos(mp);
  await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [msx, msy]);
  await sleep(300);
  const meleeN = await call('dbg_warunits', 0, 1);
  ok((await call('dbg_warsel')) === meleeN && meleeN >= 2,
     'double-tap selected ALL melee buddies', 'sel=' + (await call('dbg_warsel')) + ' melee=' + meleeN);

  // the zapper earns its keep: a crawler dropped beside it dies
  const bugs0 = await call('dbg_warunits', 1, -1);
  const [zx, zy] = unxy(await call('dbg_warstructxy', zp));
  ok((await call('dbg_warspawnbug', zx + 60, zy)) === bugs0 + 1, 'a crawler spawned beside the zapper');
  ok((await poll('dbg_warunits', [1, -1], v => v <= bugs0, 45000, 400)) <= bugs0,
     'the zapper (and the guard) killed the crawler', 'bugs=' + (await call('dbg_warunits', 1, -1)));
  ok((await call('dbg_warstructs', 2)) === 1, 'the zapper still stands');

  // mid-fight screenshot for the eyeball check
  await call('dbg_warcam', 700, 600); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war-fight.png' });

  // victory: raze the hive (struct 1) and the region falls
  await call('dbg_warhurt', 1, 1, 99999);
  ok((await poll('dbg_warstate', [], v => v === 2, 5000)) === 2, 'hive down: victory state');
  ok((await poll('dbg_warstate', [], v => v === 0, 8000)) === 0, 'victory returns to the world map');
  ok((await call('dbg_warconq')) === 1, 'region 1 is conquered (war_prog = 1)');

  // ================= round 2 (Nov 16, Daniel's playtest) ==========
  // a fresh region-1 battle for the new systems: fog, terrain, the
  // grab-the-world pan, the property rule, vehicles, triple-click.
  ok((await call('dbg_warenter', 0)) === 1, 'fresh battle for the round-2 checks');
  await sleep(600);

  // fog of war: base visible, far corners black, exploration permanent
  ok((await poll('dbg_warfog', [280, 600], v => v === 2, 4000)) === 2, 'fog: the Core clearing is visible');
  ok((await call('dbg_warfog', 1560, 60)) === 0, 'fog: the far corner starts unexplored');
  ok((await call('dbg_warfog', 60, 1140)) === 0, 'fog: the south-west corner starts unexplored');
  ok((await call('dbg_warterrain')) > 30, 'terrain scattered across the arena', await call('dbg_warterrain'));
  let scout1 = await call('dbg_warfind', 1);
  await call('dbg_warmove', scout1, 1500, 100);
  ok((await poll('dbg_warfog', [1560, 60], v => v === 2, 5000)) === 2, 'fog: a scouted corner becomes visible');
  await call('dbg_warmove', scout1, 336, 548);
  ok((await poll('dbg_warfog', [1560, 60], v => v === 1, 5000)) === 1, 'fog: explored NEVER rescinds (dims to explored, not black)');
  ok((await call('dbg_warfog', 60, 1140)) === 0, 'fog: the unvisited corner is still black');
  await call('dbg_warcam', 700, 450); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war2-fog.png' });
  await call('dbg_warcam', 430, 430); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war2-terrain.png' });

  // grab-the-world pan: drag (+100,+40) and the world follows the finger
  await call('dbg_warcam', 280, 600); await sleep(250);
  const feat = await call('dbg_warfind', 1);
  const p0 = unpos(await call('dbg_warunitpos', feat));
  await page.evaluate(() => window.__TSG.touch_point(7, 300, 150, 0));
  await sleep(130);
  for (const [dx, dy] of [[325, 160], [350, 170], [375, 180], [400, 190]]) {
    await page.evaluate(([x, y]) => window.__TSG.touch_point(7, x, y, 1), [dx, dy]);
    await sleep(130);
  }
  await page.evaluate(() => window.__TSG.touch_point(7, 400, 190, 2));
  await sleep(300);
  const p1 = unpos(await call('dbg_warunitpos', feat));
  ok(Math.abs((p1[0] - p0[0]) - 100) <= 6 && Math.abs((p1[1] - p0[1]) - 40) <= 6,
     'drag pan tracks the finger 1:1 (grab the world)', `feature moved ${p1[0] - p0[0]},${p1[1] - p0[1]} for a +100,+40 drag`);

  // the property rule through the REAL tap flow: the Core's bar arms
  // a Zapper; a tap way outside your land is refused (and you stay in
  // placement); a tap inside the Core's zone builds. Round 3: zones
  // grew (Core 34+76 = 110) and only ADJACENCY is required — the new
  // build's footprint must touch the property (centre within zone +
  // its own radius: 110 + 14 = 124 for a Zapper off the Core).
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'property: tapping the Core selects it');
  await tap(228, 337);   // bar slot 2: the DEFENSE category
  ok((await poll('dbg_warbcat', [], v => v === 2, 4000)) === 2, 'property: DEFENSE opened');
  await tap(48, 291);   // strip slot 0: ZAPPER
  ok((await poll('dbg_warplace', [], v => v === 2, 4000)) === 2, 'property: the bar armed Zapper placement');
  await tap(600, 60);    // world (560,503): ~296u from the Core — beyond even the 124u adjacency reach
  await sleep(300);
  ok((await call('dbg_warstructs', 2)) === 0 && (await call('dbg_warplace')) === 2,
     'property: a build outside your land is refused, placement kept');
  await tap(386, 157);   // world (346,600): 66u from the Core — inside its 110u zone, clear of its hull
  ok((await poll('dbg_warstructs', [2], v => v === 1, 4000)) === 1 && (await call('dbg_warplace')) === -1,
     'property: a build on your own land goes up');

  // the round-3 boundary case, exact: a Zapper centred 118u from the
  // Core is OUTSIDE every zone (Core 110, the first Zapper's 90) but
  // its footprint TOUCHES the Core's zone edge (118 <= 110 + 14), so
  // adjacency alone must let it build. Pick the first of eight ring
  // spots that also clears the first Zapper's zone and all terrain.
  const tnB = await call('dbg_warterrain');
  const featsB = [];
  for (let t = 0; t < tnB; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) featsB.push(unxy(v)); }
  const ringB = [[398, 600], [363, 683], [280, 718], [197, 683], [162, 600], [197, 517], [280, 482], [363, 517]];
  let spotB = null;
  for (const [cx, cy] of ringB) {
    const dCore = Math.hypot(cx - 280, cy - 600), dZap = Math.hypot(cx - 346, cy - 600);
    if (!(dCore > 110 && dCore <= 124 && dZap > 90)) continue;
    if (featsB.some(([fx, fy]) => Math.hypot(fx - cx, fy - cy) < 40)) continue;
    spotB = [cx, cy]; break;
  }
  ok(!!spotB, 'property: an exact boundary spot exists (centre outside every zone, footprint touching)');
  if (spotB) {
    await call('dbg_waraddbits', 200);
    await tap(320, 157);
    ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'property: the Core re-selected');
    await tap(228, 337);   // the DEFENSE category again
    ok((await poll('dbg_warbcat', [], v => v === 2, 4000)) === 2, 'property: DEFENSE re-opened');
    await tap(48, 291);   // strip slot 0: ZAPPER
    ok((await poll('dbg_warplace', [], v => v === 2, 4000)) === 2, 'property: Zapper placement re-armed');
    await tap(320 + (spotB[0] - 280), 157 + (spotB[1] - 600));
    ok((await poll('dbg_warstructs', [2], v => v === 2, 4000)) === 2 && (await call('dbg_warplace')) === -1,
       'property: a build whose footprint only TOUCHES your land goes up (adjacency, not containment)',
       'structs(2)=' + (await call('dbg_warstructs', 2)) + ' at ' + spotB.join(','));
  }

  // vehicles: the Assembly Plant, a Scout Buggy and a Siege Tank
  await call('dbg_waraddbits', 2000);
  const asm = await call('dbg_warbuildat', 5);
  ok(asm > 0 && (await call('dbg_warstructs', 5)) === 1, 'Assembly Plant built', 'idx=' + asm);
  const armyBefore = await call('dbg_warunits', 0, -1);
  let bitsV = await call('dbg_warbits');
  ok((await call('dbg_wartrain', 5)) === 1, 'a Scout Buggy rolled out');
  ok((await call('dbg_warbits')) <= bitsV - 70, 'the scout charged ~90 bits', await call('dbg_warbits'));
  bitsV = await call('dbg_warbits');
  ok((await call('dbg_wartrain', 6)) === 1, 'a Siege Tank rolled out');
  ok((await call('dbg_warbits')) <= bitsV - 140, 'the siege tank charged ~160 bits', await call('dbg_warbits'));
  ok((await call('dbg_warunits', 0, -1)) === armyBefore + 2, 'vehicles count in the army total');
  const pb2 = await call('dbg_warbuildat', 1);
  ok(pb2 > 0, 'Patch Bay built (for the triple-click ranks)');
  await call('dbg_wartrain', 1);
  await call('dbg_wartrain', 2);
  // park the two vehicles on open ground south of the base so the
  // fleet shot shows them clear of the buildings
  const scI = await call('dbg_warfind', 5), sgI = await call('dbg_warfind', 6);
  if (scI >= 0) await call('dbg_warmove', scI, 190, 800);
  if (sgI >= 0) await call('dbg_warmove', sgI, 340, 820);
  await call('dbg_warcam', 280, 690); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war2-fleet.png' });

  // triple-click: three quick taps on a melee buddy select EVERY
  // combat unit — melee, ranged and both vehicles — never harvesters
  const combatN = (await call('dbg_warunits', 0, 1)) + (await call('dbg_warunits', 0, 2))
                + (await call('dbg_warunits', 0, 5)) + (await call('dbg_warunits', 0, 6));
  const mi3 = await call('dbg_warfind', 1);
  const [m3x, m3y] = unxy(await call('dbg_warunitxy', mi3));
  await call('dbg_warcam', m3x, m3y); await sleep(250);
  for (let k = 0; k < 3; k++) {
    const pp = unpos(await call('dbg_warunitpos', mi3));
    await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), pp);
    await sleep(150);
  }
  await sleep(300);
  ok(combatN >= 5 && (await call('dbg_warsel')) === combatN,
     'triple-click selected ALL combat units (harvesters excluded)',
     'sel=' + (await call('dbg_warsel')) + ' combat=' + combatN + ' army=' + (await call('dbg_warunits', 0, -1)));

  // the siege gun cracks structures: ordered onto the hive, its shots
  // (double damage vs structures) bite chunks out of it
  const sg = await call('dbg_warfind', 6);
  await call('dbg_warmove', sg, 1190, 600);   // 140u west of the hive: inside its 125+30 range
  ok((await call('dbg_wartapunit', sg)) === 1, 'siege tank selected for the order');
  await call('dbg_warcam', 1330, 600); await sleep(250);
  const hiveHp0 = await call('dbg_warhurt', 1, 1, 0);
  await tap(320, 157);   // the hive, dead centre: attack order
  ok((await poll('dbg_warhurt', [1, 1, 0], v => v >= 0 && v <= hiveHp0 - 50, 20000, 300)) <= hiveHp0 - 50,
     'the siege tank is shelling the hive (x2 vs structures)', 'hive hp stuck at ' + (await call('dbg_warhurt', 1, 1, 0)));

  // firewalls: grid-snapped segments join into one barrier, and a bug
  // cannot walk through the line
  ok((await call('dbg_warenter', 0)) === 1, 'fresh battle for the firewall checks');
  await call('dbg_waraddbits', 500);
  const w1 = await call('dbg_warbuildxy', 3, 500, 400);
  const w2 = await call('dbg_warbuildxy', 3, 500, 424);
  const w3 = await call('dbg_warbuildxy', 3, 500, 448);
  const w4 = await call('dbg_warbuildxy', 3, 524, 448);
  ok(w1 >= 0 && w2 >= 0 && w3 >= 0 && w4 >= 0 && (await call('dbg_warstructs', 3)) === 4,
     'four firewall segments placed', [w1, w2, w3, w4].join(','));
  const snap = v => Math.round(v / 24) * 24;
  const [w1x, w1y] = unxy(await call('dbg_warstructxy', w1));
  const [w2x, w2y] = unxy(await call('dbg_warstructxy', w2));
  const [w3x, w3y] = unxy(await call('dbg_warstructxy', w3));
  const [w4x, w4y] = unxy(await call('dbg_warstructxy', w4));
  ok(w1x === snap(500) && w2x === w1x && w3x === w1x && w2y - w1y === 24 && w3y - w2y === 24
     && w4y === w3y && w4x - w3x === 24,
     'segments snapped to the 24u grid, shoulder to shoulder',
     `${w1x},${w1y} ${w2x},${w2y} ${w3x},${w3y} ${w4x},${w4y}`);
  await call('dbg_warcam', w1x, w2y); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war2-wall.png' });
  await call('dbg_warspawnbug', 640, 432);
  let bugIdx = -1, minBugX = 1e9;
  for (let s = 0; s < 10; s++) {
    await sleep(500);
    for (let i = 0; i < 80; i++) {
      const v = await call('dbg_warunitxy', i);
      if (v < 0) continue;
      const [ux, uy] = unxy(v);
      if (ux > 560 && ux < 700 && uy > 380 && uy < 490) { bugIdx = i; if (ux < minBugX) minBugX = ux; }
    }
  }
  ok(bugIdx >= 0 && minBugX > 490, 'the firewall line held: the crawler never crossed',
     'bugIdx=' + bugIdx + ' minX=' + minBugX);

  // ================= round 3 (Nov 16): destructible debris ========
  // terrain features have hp now: a melee buddy ordered onto one
  // through the REAL tap flow smashes it, the ground frees up, and
  // the siege gun gets NO x2 structure bonus against a rock.
  const tn3 = await call('dbg_warterrain');
  const alive0 = await call('dbg_warfeatn');
  ok(alive0 === tn3 && tn3 > 30, 'debris: every feature starts standing', alive0 + '/' + tn3);
  const featHp = async t => { const f = await call('dbg_warfeat', t); return f < 0 ? -1 : Math.floor(f / 100); };
  // a low-hp feature (tree 35 / rubble 25) in the western half, well
  // clear of the firewall line and its crawler (510,432)
  let featT = -1, featXY = null;
  for (let t = 0; t < tn3; t++) {
    const f = await call('dbg_warfeat', t);
    if (f < 0) continue;
    const kind = f % 100;
    if (kind !== 0 && kind !== 4) continue;
    const xy = unxy(await call('dbg_warfeatxy', t));
    if (xy[0] > 700 || Math.hypot(xy[0] - 510, xy[1] - 432) < 220) continue;
    featT = t; featXY = xy; break;
  }
  ok(featT >= 0, 'debris: found a tree/rubble feature to fell', 't=' + featT);
  if (featT >= 0) {
    const miD = await call('dbg_warfind', 1);
    await call('dbg_warmove', miD, featXY[0] + 30, featXY[1]);
    ok((await call('dbg_wartapunit', miD)) === 1, 'debris: melee buddy selected');
    await call('dbg_warcam', featXY[0], featXY[1]); await sleep(250);
    const hpStart = await featHp(featT);
    let hpMid = hpStart;
    for (let attempt = 0; attempt < 2 && hpMid >= hpStart; attempt++) {   // one re-order if the first tap got eaten
      const fp = unpos(await call('dbg_warfeatpos', featT));
      await tap(fp[0], fp[1]);   // tap the feature itself: attack order
      const pv = await poll('dbg_warfeat', [featT], v => v >= 0 && Math.floor(v / 100) < hpStart, 5000, 200);
      hpMid = pv >= 0 ? Math.floor(pv / 100) : hpStart;
    }
    ok(hpMid < hpStart && hpMid > 0, 'debris: the melee buddy is smashing the feature', hpStart + ' -> ' + hpMid);
    await cv.screenshot({ path: __dirname + '/war3-debris.png' });
    ok((await poll('dbg_warfeat', [featT], v => v >= 0 && Math.floor(v / 100) === 0, 20000, 250)) >= 0
       && (await featHp(featT)) === 0, 'debris: the feature is destroyed');
    ok((await call('dbg_warfeatn')) === alive0 - 1, 'debris: the standing count dropped by one',
       (await call('dbg_warfeatn')) + ' vs ' + alive0);
    // the ground is free now: the buddy can stand where it stood
    await call('dbg_warmove', miD, featXY[0], featXY[1]);
    await sleep(700);
    const [ux2, uy2] = unxy(await call('dbg_warunitxy', miD));
    ok(Math.hypot(ux2 - featXY[0], uy2 - featXY[1]) < 8, 'debris: the buddy stands on the cleared ground (no push-out)',
       ux2 + ',' + uy2 + ' vs ' + featXY.join(','));
  }

  // the siege gun's x2 is for structures only: one shell in a rock
  // (90 hp) must do the plain 26, never 52
  await call('dbg_waraddbits', 1000);
  const asm3 = await call('dbg_warbuildat', 5);
  ok(asm3 > 0, 'debris: Assembly Plant built for the siege check', 'idx=' + asm3);
  ok((await call('dbg_wartrain', 6)) === 1, 'debris: a Siege Tank rolled out');
  let rockT = -1, rockXY = null;
  for (let t = 0; t < tn3; t++) {
    const f = await call('dbg_warfeat', t);
    if (f < 0 || f % 100 !== 1 || Math.floor(f / 100) <= 0) continue;
    const xy = unxy(await call('dbg_warfeatxy', t));
    if (xy[0] > 700 || Math.hypot(xy[0] - 510, xy[1] - 432) < 220) continue;
    rockT = t; rockXY = xy; break;
  }
  ok(rockT >= 0, 'debris: found a rock for the siege check', 't=' + rockT);
  if (rockT >= 0) {
    const sgD = await call('dbg_warfind', 6);
    await call('dbg_warmove', sgD, rockXY[0] + 100, rockXY[1]);
    ok((await call('dbg_wartapunit', sgD)) === 1, 'debris: siege tank selected');
    await call('dbg_warcam', rockXY[0], rockXY[1]); await sleep(250);
    const rp = unpos(await call('dbg_warfeatpos', rockT));
    await tap(rp[0], rp[1]);   // tap the rock: attack order
    const pv2 = await poll('dbg_warfeat', [rockT], v => v >= 0 && Math.floor(v / 100) < 90, 15000, 200);
    const rockHp = pv2 >= 0 ? Math.floor(pv2 / 100) : -1;
    ok(rockHp === 90 - 26, 'debris: the siege shell did PLAIN damage (26, not the x2 structure rate)',
       'rock hp after one shell: ' + rockHp);
  }

  // ================= v1.16 (Nov 16): WAR! on a gamepad ============
  // The pad cursor is a finger routed through war_handle_tap: the
  // left stick flies it, A presses it (single / double / triple
  // musters fall out of the tap streak), LB/RB + A work the command
  // bar, B peels a layer back, the right stick pans screen-locked.
  ok((await call('dbg_warenter', 0)) === 1, 'pad: fresh battle for the gamepad checks');
  await sleep(700);   // entry grace
  ok((await call('dbg_warcur')) === -1, 'pad: no cursor before any pad input');

  // 1. the left stick summons the cursor and flies it east, then south
  await padAxes(1, 0, 0, 0); await sleep(450);
  const cur1 = await call('dbg_warcur');
  ok(cur1 >= 0, 'pad: left stick summoned the cursor', cur1);
  const [c1x, c1y] = unxy(Math.max(cur1, 0));
  ok(c1x > 300, 'pad: stick-right flew the cursor EAST of its spawn', c1x + ',' + c1y + ' (spawn ~280,623)');
  await padAxes(0, 1, 0, 0); await sleep(450);
  const [c2x, c2y] = unxy(await call('dbg_warcur'));
  ok(c2y > c1y + 40, 'pad: stick-down flew the cursor SOUTH', c1y + ' -> ' + c2y);
  await padAxes(0, 0, 0, 0); await sleep(150);

  // 2. A presses the field at the cursor: select, double-muster,
  // triple-muster — the tap streak ladder, harvesters excluded
  await call('dbg_waraddbits', 400);
  const pbP = await call('dbg_warbuildat', 1);
  ok(pbP > 0, 'pad: Patch Bay built (a second melee for the musters)', 'idx=' + pbP);
  await call('dbg_wartrain', 1);
  ok((await call('dbg_warunits', 0, 1)) === 2, 'pad: two melee buddies on the field');
  const [ccx, ccy] = unxy(await call('dbg_warcur'));
  const meleeA = await call('dbg_warfind', 1);
  const harvI = await call('dbg_warfind', 0);
  await call('dbg_warmove', meleeA, ccx, ccy);   // park melee A right under the cursor
  await call('dbg_warmove', harvI, 200, 200);    // the harvester works far away — it must not wander under the cursor mid-streak
  // melee B: the other friendly near the base (not A, not the
  // harvester, not a hive guard) — park it beside A, on screen
  let meleeB = -1;
  for (let i = 0; i < 80; i++) {
    if (i === meleeA || i === harvI) continue;
    const v = await call('dbg_warunitxy', i);
    if (v < 0) continue;
    const [ux, uy] = unxy(v);
    if (Math.hypot(ux - 280, uy - 600) < 300) { meleeB = i; break; }
  }
  ok(meleeB >= 0, 'pad: found the second melee for the musters', meleeB);
  if (meleeB >= 0) await call('dbg_warmove', meleeB, ccx + 120, ccy);
  await padTap(0);   // A: single press at the cursor
  ok((await poll('dbg_warsel', [], v => v === 1, 4000)) === 1, 'pad: A at a unit selects exactly it', await call('dbg_warsel'));
  await padTap(0, 110);   // A again, inside the streak window
  ok((await poll('dbg_warsel', [], v => v === 2, 4000)) === 2, 'pad: double-A mustered ALL melee on screen', await call('dbg_warsel'));
  await padTap(0, 110);   // and a third: every combat unit
  ok((await poll('dbg_warsel', [], v => v === 2, 4000)) === 2 && (await call('dbg_warunits', 0, 0)) === 1,
     'pad: triple-A mustered all COMBAT units — the harvester stays out', 'sel=' + (await call('dbg_warsel')));

  // 3. A on empty ground issues a move order
  const mBefore = await call('dbg_warunitxy', meleeA);
  await padAxes(1, 0, 0, 0); await sleep(400); await padAxes(0, 0, 0, 0); await sleep(150);
  await padTap(0);
  const mMoved = await poll('dbg_warunitxy', [meleeA], v => {
    if (v < 0) return false;
    const [ax, ay] = unxy(mBefore), [bx, by] = unxy(v);
    return bx - ax > 40;
  }, 7000, 200);
  ok(mMoved >= 0 && mMoved !== mBefore, 'pad: A on open ground marched the selection EAST', mBefore + ' -> ' + mMoved);

  // 6 (early, while the camera is known): the right stick pans and
  // the cursor keeps its SCREEN position
  const scrOf = async () => {
    const [ux, uy] = unxy(await call('dbg_warunitxy', meleeA));
    const [sx, sy] = unpos(await call('dbg_warunitpos', meleeA));
    const camx = ux - (sx - 320), camy = uy - (sy - 180 + 23);
    const [qx, qy] = unxy(await call('dbg_warcur'));
    return [320 + (qx - camx), 180 + (qy - camy - 23), camx];
  };
  const scr0 = await scrOf();
  await padAxes(0, 0, 1, 0); await sleep(500); await padAxes(0, 0, 0, 0); await sleep(200);
  const scr1 = await scrOf();
  ok(Math.abs(scr1[0] - scr0[0]) <= 8 && Math.abs(scr1[1] - scr0[1]) <= 8 && scr1[2] > scr0[2] + 60,
     'pad: right stick panned the camera, cursor SCREEN-locked',
     `screen ${scr0[0]},${scr0[1]} -> ${scr1[0]},${scr1[1]}, camx ${scr0[2]} -> ${scr1[2]}`);

  // 4. the command bar by pad: select the Core with A, RB lights a
  // button, A trains through it. Getting there takes two wakes: the
  // first B press only summons the cursor (a waking press is spent
  // waking), the second B peels the unit selection away; then a tap
  // retires the cursor so A's wake respawns it at the camera centre,
  // which is where the Core sits. ORDER MATTERS: the cursor must be
  // OFF before dbg_warcam teleports the camera — a live cursor's
  // edge-follow quite correctly drags the camera after it (that is
  // the follow doing its job; real play never teleports the camera).
  await tap(630, 10);   // a real tap retires the cursor (last device wins; empty field up there)
  ok((await poll('dbg_warcur', [], v => v === -1, 4000)) === -1, 'pad: a real tap retired the cursor');
  await padTap(1);   // B: wakes the cursor (spent waking)
  await padTap(1);   // B again: peels the unit selection away
  ok((await poll('dbg_warsel', [], v => v === 0, 4000)) === 0, 'pad: B cleared the unit selection');
  await tap(630, 10);   // retire the cursor again BEFORE moving the camera
  ok((await poll('dbg_warcur', [], v => v === -1, 4000)) === -1, 'pad: cursor retired before the camera move');
  await call('dbg_warcam', 280, 600); await sleep(250);
  await padTap(0);   // A wakes the cursor at the camera's centre (nothing selected now)
  ok((await poll('dbg_warcur', [], v => v === 2800623, 4000)) === 2800623, 'pad: A summoned the cursor at the camera centre (280,623)',
     await call('dbg_warcur'));
  await padTap(0);   // A again: the tap itself — the Core is at the camera's centre
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'pad: A at the camera centre selected the Core');
  ok((await call('dbg_warsel')) === 0, 'pad: selecting the Core cleared the unit selection');
  await padTap(5);   // RB: light the first bar button — v1.17: the TRAIN category
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'pad: RB lit bar button 0', await call('dbg_warbarhl'));
  await padTap(0);   // A opens the category
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'pad: A on TRAIN opened the category row');
  await padTap(5);   // RB: light the first OPTION (HARVESTER)
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'pad: RB lit option 0 (HARVESTER)');
  const bitsBar = await call('dbg_warbits');
  await padTap(0);   // A presses the lit option
  ok((await poll('dbg_warbits', [], v => v === bitsBar - 80, 4000)) === bitsBar - 80,
     'pad: A on the lit button trained (80 bits charged)', bitsBar + ' -> ' + (await call('dbg_warbits')));
  ok((await poll('dbg_warunits', [0, 0], v => v === 2, 14000)) === 2, 'pad: the bar-trained harvester rolled out');

  // 5. B cancels a pending placement — nothing built, nothing charged.
  // v1.17 drive: B peels the open TRAIN category shut, RB walks the
  // category row to DEFENSE, A opens it, RB walks its options to
  // FIREWALL, A arms it — then B cancels as ever.
  await padTap(1);   // B: peel the open category shut
  ok((await poll('dbg_warbcat', [], v => v === -1, 4000)) === -1, 'pad: B peeled the open category shut');
  await padTap(5); await padTap(5); await padTap(5);   // RB x3: TRAIN -> BUILD -> DEFENSE
  ok((await poll('dbg_warbarhl', [], v => v === 2, 4000)) === 2, 'pad: RB walked the highlight to DEFENSE (slot 2)', await call('dbg_warbarhl'));
  await padTap(0);   // A opens DEFENSE
  ok((await poll('dbg_warbcat', [], v => v === 2, 4000)) === 2, 'pad: A opened the DEFENSE category');
  await padTap(5); await padTap(5);   // RB x2: ZAPPER -> FIREWALL
  ok((await poll('dbg_warbarhl', [], v => v === 1, 4000)) === 1, 'pad: RB walked the options to FIREWALL (slot 1)', await call('dbg_warbarhl'));
  await padTap(0);
  ok((await poll('dbg_warplace', [], v => v === 3, 4000)) === 3, 'pad: A on FIREWALL armed its placement');
  const bitsPlace = await call('dbg_warbits'), wallsPlace = await call('dbg_warstructs', 3);
  await padTap(1);   // B: cancel
  ok((await poll('dbg_warplace', [], v => v === -1, 4000)) === -1
     && (await call('dbg_warbits')) === bitsPlace && (await call('dbg_warstructs', 3)) === wallsPlace,
     'pad: B cancelled the placement — no wall, no charge',
     'place=' + (await call('dbg_warplace')) + ' bits=' + (await call('dbg_warbits')) + ' walls=' + (await call('dbg_warstructs', 3)));

  // hand the field back to the keyboard for the suite's tail
  await padAxes(0, 0, 0, 0);
  await key('ArrowLeft', 200);

  // ================= v1.17 "SUPPLY LINES" (Nov 16) ================
  // resources, pathfinding, gates, queues, rally points. (Check 1
  // rode the first battle; check 10 rode the bar flows; the pause /
  // snapshot checks 6-9 run in the tail, on the region-2 battle.)
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17: fresh battle for the supply-lines checks');
  await sleep(600);

  // check 2: a harvester load can reach the new 25-bit carry
  ok((await poll('dbg_warcarry', [], v => v >= 25, 60000, 400)) >= 25,
     'v1.17 check 2: a harvester scooped a full 25-bit load (WAR_CARRY)', 'carry=' + (await call('dbg_warcarry')));

  // check 3: debris pays its salvage bounty for PLAYER damage. The
  // economy is isolated first (the harvester dies rich, so no
  // emergency grant can fire): after that, the only bits that can
  // move are the bounty itself. (Bug-side debris damage paying
  // nothing is NOT asserted: bugs never target debris and their
  // stray projectiles are the only side-1 route into
  // war_hurt_terrain — not drivable deterministically. The bounty
  // branch is side-gated in one place; see the build report.)
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 3: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 500);
  const harvK = await call('dbg_warfind', 0);
  await call('dbg_warhurt', 0, harvK, 99999);
  ok((await call('dbg_warunits', 0, 0)) === 0, 'v1.17 check 3: harvester down (economy isolated, no grant at 750 bits)');
  const tn17 = await call('dbg_warterrain');
  let treeT = -1, treeXY = null, treeKind = -1;
  for (let t = 0; t < tn17; t++) {
    const f = await call('dbg_warfeat', t);
    if (f < 0) continue;
    const kind = f % 100;
    if (kind !== 0 && kind !== 4) continue;   // tree (10 bits) or rubble (15)
    const xy = unxy(await call('dbg_warfeatxy', t));
    if (xy[0] > 700) continue;
    treeT = t; treeXY = xy; treeKind = kind; break;
  }
  ok(treeT >= 0, 'v1.17 check 3: found a tree/rubble feature to fell', 't=' + treeT);
  if (treeT >= 0) {
    const bounty = treeKind === 0 ? 10 : 15;
    const miT = await call('dbg_warfind', 1);
    await call('dbg_warmove', miT, treeXY[0] + 30, treeXY[1]);
    ok((await call('dbg_wartapunit', miT)) === 1, 'v1.17 check 3: melee selected');
    await call('dbg_warcam', treeXY[0], treeXY[1]); await sleep(250);
    const bitsBounty0 = await call('dbg_warbits');
    const fpT = unpos(await call('dbg_warfeatpos', treeT));
    await tap(fpT[0], fpT[1]);   // the attack order on the feature itself
    ok((await poll('dbg_warfeat', [treeT], v => v >= 0 && Math.floor(v / 100) === 0, 30000, 250)) >= 0,
       'v1.17 check 3: the melee felled the feature');
    ok((await call('dbg_warbits')) === bitsBounty0 + bounty,
       'v1.17 check 3: the salvage bounty landed exactly', bitsBounty0 + ' -> ' + (await call('dbg_warbits')) + ' (bounty ' + bounty + ')');
  }

  // check 4: a harvester paths AROUND a firewall line (through its
  // gap) and still delivers — straight steering used to pin forever
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 4: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 2000);
  let wallN4 = 0;
  for (let y = 408; y <= 792; y += 24) { if (y === 600) continue; if ((await call('dbg_warbuildxy', 3, 360, y)) >= 0) wallN4++; }
  ok(wallN4 >= 14, 'v1.17 check 4: the wall line went up west of the nodes (gap at y=600)', 'segments=' + wallN4);
  const bitsPath0 = await call('dbg_warbits');
  const harvP = await call('dbg_warfind', 0);
  let crossed4 = false;
  const tPath0 = Date.now();
  while (Date.now() - tPath0 < 90000) {
    const v = await call('dbg_warunitxy', harvP);
    if (v >= 0 && Math.floor(v / 10000) > 380) crossed4 = true;
    if (crossed4 && (await call('dbg_warbits')) >= bitsPath0 + 25) break;
    await sleep(250);
  }
  ok(crossed4, 'v1.17 check 4: the harvester crossed the wall line (x>380)');
  ok((await call('dbg_warbits')) >= bitsPath0 + 25, 'v1.17 check 4: ...and delivered bits through the gap', 'bits=' + (await call('dbg_warbits')));

  // check 5: a bug paths around a firewall line to reach its target.
  // Bugs CHEW structures they target, so the bait is a unit (units
  // outrank structures in bug targeting): the starting melee, pinned
  // west of the line by the harness (re-teleported while the bug
  // routes) so the crossing measured is the bug's alone. The bait
  // stands at (640,430), up-line: the detour around the wall's north
  // end runs ~190u from a mid-line bait — outside the 170u bug
  // aggro radius, so a bug routed from (640,500) legitimately loses
  // its target at the top of the detour and falls back. From
  // (640,430) the whole route stays inside aggro.
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 5: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 2000);
  let wallN5 = 0;
  for (let y = 384; y <= 816; y += 24) { if (y === 600) continue; if ((await call('dbg_warbuildxy', 3, 700, y)) >= 0) wallN5++; }
  ok(wallN5 >= 16, 'v1.17 check 5: the wall line went up (gap at y=600)', 'segments=' + wallN5);
  const miBait = await call('dbg_warfind', 1);
  await call('dbg_warmove', miBait, 640, 430);
  const harvBait = await call('dbg_warfind', 0);
  await call('dbg_warmove', harvBait, 200, 1100);   // park the harvester far west — east-side nodes would make IT the bug's nearest unit target
  await call('dbg_warspawnbug', 790, 470);
  let bugI = -1;
  for (let i = 0; i < 80; i++) {
    await call('dbg_warmove', miBait, 640, 430);   // keep the bait glued down while we look — unpinned, it charges the bug and kills it east of the line
    const v = await call('dbg_warunitxy', i);
    if (v < 0) continue;
    const [ux, uy] = unxy(v);
    if (ux > 715 && Math.abs(uy - 470) < 60) { bugI = i; break; }
  }
  ok(bugI >= 0, 'v1.17 check 5: the spawned bug is tracked', 'idx=' + bugI);
  let crossed5 = false;
  if (bugI >= 0) {
    const tBug0 = Date.now();
    while (Date.now() - tBug0 < 45000) {
      // pin at a ~120ms cadence, not 300ms: the bait charges the
      // bug between pins (~85u/s), and a 300ms pin cycle lets it
      // drift ~30u — when the pin then teleports it back, the bug's
      // pursuit path reads the jump as a stale target and drops
      // mid-route (the bug lunged to the wall's end three times and
      // fell back). ~120ms keeps the bait within ~15u of its post.
      await call('dbg_warmove', miBait, 640, 430);   // pin the bait west of the line
      const v = await call('dbg_warunitxy', bugI);
      if (v >= 0 && Math.floor(v / 10000) <= 686) { crossed5 = true; break; }
      await sleep(120);
    }
  }
  ok(crossed5, 'v1.17 check 5: the bug crossed the line (gap or end — around, not through)',
     'bug xy at timeout: ' + (bugI >= 0 ? await call('dbg_warunitxy', bugI) : 'untracked'));
  ok((await call('dbg_warstructs', 3)) === wallN5, 'v1.17 check 5: every wall segment still stands',
     'walls=' + (await call('dbg_warstructs', 3)) + ' of ' + wallN5);

  // check 11: the gate — yours walk through, bugs chew it
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 11: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 1500);
  const harvG = await call('dbg_warfind', 0);
  await call('dbg_warmove', harvG, 1500, 100);   // park the workers far away
  const miG = await call('dbg_warfind', 1);
  await call('dbg_warmove', miG, 200, 1100);
  for (const y of [504, 528, 552, 576, 624, 648, 672, 696]) await call('dbg_warbuildxy', 3, 500, y);
  const gateI = await call('dbg_warbuildxy', 6, 500, 600);
  ok(gateI >= 0 && (await call('dbg_warstructs', 6)) === 1, 'v1.17 check 11: the gate went up in the wall line', 'idx=' + gateI);
  if (gateI >= 0) {
    const gateHp0 = await call('dbg_warhurt', 1, gateI, 0);
    await call('dbg_warspawnbug', 570, 600);
    let bugG = -1;
    for (let i = 0; i < 80; i++) {
      const v = await call('dbg_warunitxy', i);
      if (v < 0) continue;
      const [ux, uy] = unxy(v);
      if (ux > 540 && Math.abs(uy - 600) < 40) { bugG = i; break; }
    }
    let bugMinX = 1e9, gateHp1 = gateHp0;
    const tGate0 = Date.now();
    while (Date.now() - tGate0 < 20000) {
      gateHp1 = await call('dbg_warhurt', 1, gateI, 0);
      if (bugG >= 0) { const v = await call('dbg_warunitxy', bugG); if (v >= 0) bugMinX = Math.min(bugMinX, Math.floor(v / 10000)); }
      if (gateHp1 < gateHp0) break;
      await sleep(400);
    }
    ok(gateHp1 < gateHp0, 'v1.17 check 11: the bug is chewing the gate (it cannot pass)', gateHp0 + ' -> ' + gateHp1);
    ok(bugMinX >= 520, 'v1.17 check 11: the bug never crossed the gate line', 'minX=' + bugMinX);
    await call('dbg_warmove', miG, 440, 600);
    ok((await call('dbg_wartapunit', miG)) === 1, 'v1.17 check 11: melee selected');
    await call('dbg_warcam', 504, 600); await sleep(250);
    await tap(440, 157);   // world (624,600): east of the line
    const crossV = await poll('dbg_warunitxy', [miG], v => v >= 0 && Math.floor(v / 10000) > 545, 20000, 200);
    ok(crossV >= 0 && Math.floor(crossV / 10000) > 545, 'v1.17 check 11: the melee crossed THROUGH the gate', 'xy=' + crossV);
  }

  // check 12: a harvester whose only door is its own gate still
  // completes the harvest loop (pathfinding treats the gate as a
  // door for side 0 — and war_separate never stops it there)
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 12: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 4000);
  // neither the line's x nor its gate spot is fixed: region terrain
  // is deterministic and dense along any one line, so scan the
  // debris field and put the line + gate where the clearance is
  // best (clearance >= 62 also covers the gate's wall neighbours:
  // debris refusal reaches ~37u, and a neighbour sits 24u away)
  const tn12 = await call('dbg_warterrain');
  const feats12 = [];
  for (let t = 0; t < tn12; t++) feats12.push(unxy(await call('dbg_warfeatxy', t)));
  let lineX = -1, gateY = -1, bestClear = -1;
  for (const lx of [816, 864, 912, 960]) {
    for (let y = 408; y <= 792; y += 24) {
      let clear = 1e9;
      for (const [fx, fy] of feats12) if (Math.abs(fx - lx) < 60) clear = Math.min(clear, Math.abs(fy - y));
      if (clear > bestClear) { bestClear = clear; lineX = lx; gateY = y; }
    }
  }
  ok(lineX > 0 && bestClear >= 62, 'v1.17 check 12: a clear line + gate spot exists', 'lineX=' + lineX + ' gateY=' + gateY + ' clearance=' + bestClear);
  const gateQ = await call('dbg_warbuildxy', 6, lineX, gateY);
  ok(gateQ >= 0, 'v1.17 check 12: the gate went up at (' + lineX + ',' + gateY + ')', 'idx=' + gateQ);
  let wallNQ = 0, gateNeighbours = 0;
  for (let y = 48; y <= 1152; y += 24) {
    if (y === gateY) continue;
    if ((await call('dbg_warbuildxy', 3, lineX, y)) >= 0) { wallNQ++; if (Math.abs(y - gateY) === 24) gateNeighbours++; }
  }
  ok(wallNQ >= 22 && gateNeighbours === 2, 'v1.17 check 12: the wall line went up around the gate (neighbours solid)',
     'segments=' + wallNQ + ' neighbours=' + gateNeighbours);
  const harvQ = await call('dbg_warfind', 0);
  await call('dbg_warmove', harvQ, lineX - 62, gateY);
  ok((await call('dbg_wartapunit', harvQ)) === 1, 'v1.17 check 12: harvester selected');
  await call('dbg_warcam', lineX, gateY); await sleep(250);
  await tap(408, 157);   // world (lineX+88, gateY): east of the line — a manual move order
  let throughGate = false, arrivedEast = false;
  const tQ0 = Date.now();
  while (Date.now() - tQ0 < 30000) {
    const v = await call('dbg_warunitxy', harvQ);
    if (v >= 0) {
      const [ux, uy] = unxy(v);
      if (Math.abs(ux - lineX) < 16 && Math.abs(uy - gateY) < 22) throughGate = true;
      if (ux > lineX + 48) arrivedEast = true;
    }
    if (throughGate && arrivedEast) break;
    await sleep(120);
  }
  ok(throughGate, 'v1.17 check 12: the harvester crossed through its own GATE');
  ok(arrivedEast, 'v1.17 check 12: ...and reached the east side');
  const bitsQ0 = await call('dbg_warbits');
  ok((await poll('dbg_warbits', [], v => v >= bitsQ0 + 25, 90000, 500)) >= bitsQ0 + 25,
     'v1.17 check 12: it harvested the east node and delivered back through the gate', 'bits=' + (await call('dbg_warbits')));

  // check 13: a gate survives the snapshot round trip
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 13: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 500);
  const gateS = await call('dbg_warbuildxy', 6, 600, 300);
  ok(gateS >= 0 && (await call('dbg_warstructs', 6)) === 1, 'v1.17 check 13: gate built', 'idx=' + gateS);
  if (gateS >= 0) {
    const gateHpS = await call('dbg_warhurt', 1, gateS, 0);
    await key('Escape', 300);
    ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'check 13: pause menu up');
    await tap(320, 234);   // SAVE & QUIT
    ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0 && (await call('dbg_warsnap')) === 1, 'v1.17 check 13: battle parked with the gate in it');
    await key('Escape', 300);
    ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'check 13: left WAR!');
    await call('dbg_bwentm', 8); await sleep(500);
    ok((await poll('dbg_warstate', [], v => v === 1, 4000)) === 1, 'v1.17 check 13: battle restored');
    ok((await call('dbg_warstructs', 6)) === 1 && (await call('dbg_warhurt', 1, gateS, 0)) === gateHpS,
       'v1.17 check 13: the gate is back — type 6, same slot, same hp',
       'structs(6)=' + (await call('dbg_warstructs', 6)) + ' hp=' + (await call('dbg_warhurt', 1, gateS, 0)) + ' vs ' + gateHpS);
  }

  // check 14: queue stacking (addendum 2) — pay per unit up front,
  // they roll out in order with no further input
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 14: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 1000);
  const pbQ = await call('dbg_warbuildat', 1);
  ok(pbQ > 0, 'v1.17 check 14: Patch Bay built', 'idx=' + pbQ);
  const harvQ14 = await call('dbg_warfind', 0);
  await call('dbg_warhurt', 0, harvQ14, 99999);   // isolate the purse (rich: no emergency grant)
  const [pbx, pby] = unxy(await call('dbg_warstructxy', pbQ));
  await call('dbg_warcam', pbx, pby); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== pbQ) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === pbQ, 4000)) === pbQ, 'v1.17 check 14: Patch Bay selected');
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.17 check 14: its TRAIN menu starts open (single category)');
  const bitsQ14 = await call('dbg_warbits');
  await tap(48, 291);   // strip slot 0: WRENCH (melee, 50 bits)
  const costOne = bitsQ14 - (await call('dbg_warbits'));
  ok(costOne === 50 && (await call('dbg_warqueue', pbQ)) === 1, 'v1.17 check 14: first melee queued and charged at once', 'cost=' + costOne);
  await tap(48, 291); await tap(48, 291);
  ok((await call('dbg_warqueue', pbQ)) === 3, 'v1.17 check 14: the queue stacked to 3');
  ok((await call('dbg_warbits')) === bitsQ14 - 3 * costOne, 'v1.17 check 14: charged 3x up front', bitsQ14 + ' -> ' + (await call('dbg_warbits')));
  const meleeQ0 = await call('dbg_warunits', 0, 1);
  ok((await poll('dbg_warunits', [0, 1], v => v === meleeQ0 + 3, 60000, 500)) === meleeQ0 + 3,
     'v1.17 check 14: all three rolled out on their own', 'melee=' + (await call('dbg_warunits', 0, 1)));
  ok((await call('dbg_warqueue', pbQ)) === 0, 'v1.17 check 14: the queue drained');

  // check 15: pay-to-queue — short bits refuse, changing nothing
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 15: fresh battle');
  await sleep(400);
  const harvQ15 = await call('dbg_warfind', 0);
  await call('dbg_warhurt', 0, harvQ15, 99999);   // isolate the purse (250 bits: no grant)
  const pbQ15 = await call('dbg_warbuildat', 1);   // 250 - 150 = 100 bits left
  ok(pbQ15 > 0, 'v1.17 check 15: Patch Bay built', 'idx=' + pbQ15);
  const [qx15, qy15] = unxy(await call('dbg_warstructxy', pbQ15));
  await call('dbg_warcam', qx15, qy15); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== pbQ15) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === pbQ15, 4000)) === pbQ15, 'v1.17 check 15: Patch Bay selected');
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.17 check 15: TRAIN open');
  await tap(48, 291); await tap(48, 291);   // two melees: 2 x 50 = the whole 100
  ok((await call('dbg_warqueue', pbQ15)) === 2 && (await call('dbg_warbits')) === 0,
     'v1.17 check 15: two queued, purse empty', 'q=' + (await call('dbg_warqueue', pbQ15)) + ' bits=' + (await call('dbg_warbits')));
  await tap(48, 291);   // a third costs 50: refused
  ok((await call('dbg_warqueue', pbQ15)) === 2 && (await call('dbg_warbits')) === 0,
     'v1.17 check 15: pay-to-queue refused it — queue and purse untouched');

  // check 16: the cap — nine queue, the tenth is refused free, and
  // the nine still complete (siege tanks: their 9s train time makes
  // the fill race-free)
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 16: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 3000);
  const harvQ16 = await call('dbg_warfind', 0);
  await call('dbg_warhurt', 0, harvQ16, 99999);
  const asmQ = await call('dbg_warbuildat', 5);
  ok(asmQ > 0, 'v1.17 check 16: Assembly Plant built', 'idx=' + asmQ);
  const [qx16, qy16] = unxy(await call('dbg_warstructxy', asmQ));
  await call('dbg_warcam', qx16, qy16); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== asmQ) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === asmQ, 4000)) === asmQ, 'v1.17 check 16: Assembly Plant selected');
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.17 check 16: TRAIN open');
  for (let k = 0; k < 9; k++) await tap(138, 291);   // strip slot 1: SIEGE TANK
  ok((await poll('dbg_warqueue', [asmQ], v => v === 9, 6000)) === 9, 'v1.17 check 16: the queue filled to 9');
  const bitsQ16 = await call('dbg_warbits');
  await tap(138, 291);   // the tenth: refused, charges nothing
  ok((await call('dbg_warqueue', asmQ)) === 9 && (await call('dbg_warbits')) === bitsQ16,
     'v1.17 check 16: the 10th activation was refused free of charge',
     'q=' + (await call('dbg_warqueue', asmQ)) + ' bits=' + (await call('dbg_warbits')) + ' vs ' + bitsQ16);
  ok((await poll('dbg_warunits', [0, 6], v => v === 9, 180000, 1000)) === 9,
     'v1.17 check 16: all nine siege tanks still completed', 'siege=' + (await call('dbg_warunits', 0, 6)));

  // check 17: rally flow — a field tap with the Patch Bay selected
  // plants its flag; the next unit it trains walks there
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 17: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 500);
  const pbR = await call('dbg_warbuildat', 1);
  ok(pbR > 0, 'v1.17 check 17: Patch Bay built', 'idx=' + pbR);
  const [rx, ry] = unxy(await call('dbg_warstructxy', pbR));
  await call('dbg_warcam', rx, ry); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== pbR) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === pbR, 4000)) === pbR, 'v1.17 check 17: Patch Bay selected');
  const rdx = rx >= 280 ? 120 : -120;   // rally AWAY from the Core, so the tap can't land on it
  await tap(320 + rdx, 157);
  const rallyV = await call('dbg_warrally', pbR);
  ok(rallyV >= 0 && Math.abs(Math.floor(rallyV / 10000) - (rx + rdx)) <= 1 && Math.abs((rallyV % 10000) - ry) <= 1,
     'v1.17 check 17: the rally point is where the field was tapped', 'rally=' + rallyV + ' want ~' + (rx + rdx) + ',' + ry);
  ok((await call('dbg_warselstruct')) === pbR, 'v1.17 check 17: the building stayed selected');
  const idxBefore17 = new Set();
  for (let i = 0; i < 80; i++) if ((await call('dbg_warunitxy', i)) >= 0) idxBefore17.add(i);
  await tap(48, 291);   // strip slot 0: WRENCH
  let newI17 = -1;
  const tS17 = Date.now();
  while (Date.now() - tS17 < 15000 && newI17 < 0) {
    for (let i = 0; i < 80; i++) if (!idxBefore17.has(i) && (await call('dbg_warunitxy', i)) >= 0) { newI17 = i; break; }
    if (newI17 < 0) await sleep(200);
  }
  ok(newI17 >= 0, 'v1.17 check 17: the trained melee rolled out', 'idx=' + newI17);
  if (newI17 >= 0) {
    const reached = await poll('dbg_warunitxy', [newI17], v => {
      if (v < 0) return false;
      const [ux, uy] = unxy(v);
      return Math.hypot(ux - (rx + rdx), uy - ry) < 30;
    }, 25000, 200);
    ok(reached >= 0, 'v1.17 check 17: it walked to the rally point', 'xy=' + reached);
  }

  // check 18: the rally rides the snapshot
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 18: fresh battle');
  await sleep(400);
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);   // select the Core
  if ((await call('dbg_warselstruct')) !== 0) { await sleep(600); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'v1.17 check 18: Core selected');
  await tap(420, 200);   // world (380,643): the rally tap
  const rally18 = await call('dbg_warrally', 0);
  ok(rally18 >= 0, 'v1.17 check 18: rally set on the Core', 'rally=' + rally18);
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'check 18: pause menu up');
  await tap(320, 234);   // SAVE & QUIT
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'v1.17 check 18: battle parked');
  await key('Escape', 300);
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'check 18: left WAR!');
  await call('dbg_bwentm', 8); await sleep(500);
  ok((await poll('dbg_warstate', [], v => v === 1, 4000)) === 1, 'v1.17 check 18: battle restored');
  ok((await call('dbg_warrally', 0)) === rally18, 'v1.17 check 18: the rally survived the round trip', rally18 + ' -> ' + (await call('dbg_warrally', 0)));
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== 0) { await sleep(600); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'v1.17 check 18: Core re-selected after restore');
  await tap(48, 337);   // TRAIN
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.17 check 18: TRAIN open');
  const [r18x, r18y] = unxy(Math.max(rally18, 0));
  const idxBefore18 = new Set();
  for (let i = 0; i < 80; i++) if ((await call('dbg_warunitxy', i)) >= 0) idxBefore18.add(i);
  await tap(48, 291);   // HARVESTER
  let newH18 = -1;
  const tH18 = Date.now();
  while (Date.now() - tH18 < 15000 && newH18 < 0) {
    for (let i = 0; i < 80; i++) if (!idxBefore18.has(i) && (await call('dbg_warunitxy', i)) >= 0) { newH18 = i; break; }
    if (newH18 < 0) await sleep(200);
  }
  ok(newH18 >= 0, 'v1.17 check 18: the post-restore harvester rolled out', 'idx=' + newH18);
  if (newH18 >= 0) {
    const reached = await poll('dbg_warunitxy', [newH18], v => {
      if (v < 0) return false;
      const [ux, uy] = unxy(v);
      return Math.hypot(ux - r18x, uy - r18y) < 30;
    }, 20000, 150);
    ok(reached >= 0, 'v1.17 check 18: it walked to the restored rally point', 'xy=' + reached);
  }

  // check 19: tap meaning is unchanged around producers
  ok((await call('dbg_warenter', 0)) === 1, 'v1.17 check 19: fresh battle');
  await sleep(400);
  await call('dbg_waraddbits', 800);
  const pbS = await call('dbg_warbuildat', 1);
  const zpS = await call('dbg_warbuildat', 2);
  ok(pbS > 0 && zpS > 0, 'v1.17 check 19: Patch Bay and Zapper built', pbS + ',' + zpS);
  const [sbx, sby] = unxy(await call('dbg_warstructxy', pbS));
  await call('dbg_warcam', sbx, sby); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== pbS) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === pbS, 4000)) === pbS, 'v1.17 check 19: Patch Bay selected');
  const miS = await call('dbg_warfind', 1);
  const [smx, smy] = unxy(await call('dbg_warunitxy', miS));
  await call('dbg_warcam', smx, smy); await sleep(250);
  let mpS = unpos(await call('dbg_warunitpos', miS));
  await tap(mpS[0], mpS[1]);
  if ((await call('dbg_warsel')) !== 1) { mpS = unpos(await call('dbg_warunitpos', miS)); await tap(mpS[0], mpS[1]); }
  ok((await call('dbg_warsel')) === 1 && (await call('dbg_warselstruct')) === -1,
     'v1.17 check 19: with a producer selected, tapping a friendly unit still selects the unit');
  await call('dbg_warcam', sbx, sby); await sleep(250);
  await tap(320, 157);
  if ((await call('dbg_warselstruct')) !== pbS) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === pbS, 4000)) === pbS, 'v1.17 check 19: back to the Patch Bay');
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);   // the Core
  if ((await call('dbg_warselstruct')) !== 0) { await sleep(600); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'v1.17 check 19: tapping the Core switched the selection to it');
  const [szx, szy] = unxy(await call('dbg_warstructxy', zpS));
  await call('dbg_warcam', szx, szy); await sleep(250);
  await tap(320, 157);   // select the Zapper
  if ((await call('dbg_warselstruct')) !== zpS) { await sleep(500); await tap(320, 157); }
  ok((await poll('dbg_warselstruct', [], v => v === zpS, 4000)) === zpS, 'v1.17 check 19: Zapper selected');
  await tap(320, 60);   // empty field north of it
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'v1.17 check 19: with a Zapper selected, a field tap still just deselects');

  // region 2 is unlocked now
  ok((await call('dbg_warenter', 1)) === 1, 'region 2 battle launches');
  ok((await call('dbg_warhives')) === 1, 'region 2 also fields one hive');

  // v1.17 check 6: Esc in a battle opens the PAUSE MENU — the battle
  // freezes behind it; RESUME picks it up mid-march
  // the march target must be clear ground: a tap ON debris is an
  // attack order (the buddy stands and smashes — it would look
  // frozen even after RESUME), so scan region 2's debris first.
  // NOTE the sleep after the scan: ~70 back-to-back evaluate calls
  // starve the game's frame loop, so in GAME time the battle is
  // still inside its 0.35s entry grace when the scan returns — and
  // a tap inside grace is eaten. Real sleeps let game time flow.
  const tnP = await call('dbg_warterrain');
  const featPts = [];
  for (let t = 0; t < tnP; t++) featPts.push(unxy(await call('dbg_warfeatxy', t)));
  let marchTo = null;
  for (const cand of [[560, 600], [500, 720], [450, 520], [400, 640], [560, 520], [480, 680]]) {   // all inside the 640x360 tap frame
    if (featPts.every(([fx, fy]) => Math.hypot(fx - cand[0], fy - cand[1]) > 45)) { marchTo = cand; break; }
  }
  ok(!!marchTo, 'pause: found a debris-free march target', marchTo && marchTo.join(','));
  await sleep(700);   // let the entry grace expire in game time
  const miP = await call('dbg_warfind', 1);
  ok((await call('dbg_wartapunit', miP)) === 1, 'pause: melee selected for a march');
  const startP = await call('dbg_warunitxy', miP);
  await tap(320 + (marchTo[0] - 280), 157 + (marchTo[1] - 600));   // order it across the field
  await sleep(300);
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1 && (await call('dbg_warstate')) === 1,
     'v1.17 check 6: Esc opened the pause menu (the battle did NOT abandon)');
  const frozenAt = await call('dbg_warunitxy', miP);
  ok(frozenAt !== startP, 'v1.17 check 6: the march was live when the menu opened', startP + ' -> ' + frozenAt);
  await sleep(1300);
  ok((await call('dbg_warunitxy', miP)) === frozenAt, 'v1.17 check 6: the battle is FROZEN behind the menu',
     frozenAt + ' -> ' + (await call('dbg_warunitxy', miP)));
  await tap(320, 146);   // RESUME (panel button 1)
  ok((await poll('dbg_warpause', [], v => v === 0, 4000)) === 0, 'v1.17 check 6: RESUME closed the menu');
  ok((await poll('dbg_warunitxy', [miP], v => v >= 0 && v !== frozenAt, 8000)) !== frozenAt, 'v1.17 check 6: the march resumed');

  // v1.17 check 7: SAVE & QUIT parks the battle; re-entering the
  // cabinet restores it — same region, purse, army and nodes. The
  // readings are taken with the menu OPEN (battle frozen), so
  // nothing can drift between the reading and the snapshot.
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'pause menu up for SAVE & QUIT');
  const bitsSnap = await call('dbg_warbits'), unitsSnap = await call('dbg_warunits', 0, -1), nodesSnap = await call('dbg_warnodes');
  await tap(320, 234);   // SAVE & QUIT (panel button 3)
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'v1.17 check 7: SAVE & QUIT returned to the map');
  ok((await call('dbg_warsnap')) === 1, 'v1.17 check 7: a battle snapshot is parked');
  await key('Escape', 300);   // out of the mode entirely
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'Esc on the map left WAR!');
  // the mode was last re-entered via dbg_bwentm (mall_enter first),
  // so Esc's return floor is the mall, not the arcade — a harness
  // artefact, not game behaviour; either floor proves the exit
  ok((await call('dbg_inarcade')) === 1 || (await call('dbg_mall')) === 1, 'back on the mall/arcade floor after leaving WAR!');
  await call('dbg_bwentm', 8); await sleep(500);
  ok((await poll('dbg_warstate', [], v => v === 1, 4000)) === 1, 'v1.17 check 7: re-entering WAR! restored the battle');
  ok((await call('dbg_warregion')) === 1, 'v1.17 check 7: the restored battle is region 2');
  ok((await call('dbg_warbits')) === bitsSnap, 'v1.17 check 7: the purse survived exactly', bitsSnap + ' -> ' + (await call('dbg_warbits')));
  ok((await call('dbg_warunits', 0, -1)) === unitsSnap, 'v1.17 check 7: the army survived', unitsSnap + ' -> ' + (await call('dbg_warunits', 0, -1)));
  ok((await call('dbg_warnodes')) === nodesSnap, 'v1.17 check 7: the nodes are as they were left', nodesSnap + ' -> ' + (await call('dbg_warnodes')));
  ok((await call('dbg_warsnap')) === 0, 'v1.17 check 7: the snapshot was consumed by the restore');
  ok((await call('dbg_warfog', 280, 600)) === 2, 'v1.17 check 7: fog restored around the Core');

  // v1.17 check 8: RESTART BATTLE — same region, fresh field, and
  // no snapshot of the old fight survives
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'pause menu up for RESTART');
  await tap(320, 190);   // RESTART BATTLE (panel button 2)
  ok((await poll('dbg_warpause', [], v => v === 0, 4000)) === 0 && (await call('dbg_warstate')) === 1, 'v1.17 check 8: RESTART relaunched the battle');
  ok((await call('dbg_warbits')) === 250 && (await call('dbg_warunits', 0, -1)) === 2 && (await call('dbg_warnodes')) === 904800,
     'v1.17 check 8: fresh start — 250 bits, the two starting units, full nodes',
     'bits=' + (await call('dbg_warbits')) + ' units=' + (await call('dbg_warunits', 0, -1)) + ' nodes=' + (await call('dbg_warnodes')));
  ok((await call('dbg_warregion')) === 1, 'v1.17 check 8: still region 2');
  // "no snapshot of the OLD fight survives" can't be read off
  // dbg_warsnap directly: the game autosaves every 5s and a live
  // battle re-serializes into the snapshot by design. Prove it
  // functionally instead — park this fresh battle and re-enter:
  // what comes back must be the FRESH fight (nodes untouched),
  // not the drained region-2 battle check 7 restored.
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'check 8: pause menu up to park the fresh battle');
  await tap(320, 234);   // SAVE & QUIT
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'check 8: fresh battle parked');
  await key('Escape', 300);
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'check 8: left WAR!');
  await call('dbg_bwentm', 8); await sleep(500);
  ok((await poll('dbg_warstate', [], v => v === 1, 4000)) === 1 && (await call('dbg_warnodes')) === 904800,
     'v1.17 check 8: re-entering restores the FRESH battle — the old parked fight is gone',
     'nodes=' + (await call('dbg_warnodes')));

  // park THIS battle, so the save-file round trip below has one to
  // restore (v1.17 check 9)
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'pause menu up to park the battle');
  await tap(320, 234);   // SAVE & QUIT
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'battle parked for the Continue round trip');
  await key('Escape', 300);
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'Esc on the map left WAR!');

  // persistence: save + quit, Continue — the conquest must survive,
  // and (v1.17 check 9) the parked battle rides the save FILE: after
  // a full quit + Continue, entering WAR! restores it
  await hold('KeyP', 500);
  await hold('ArrowDown', 250);
  await hold('ArrowDown', 250);        // rows: CONTINUE / CONTROLS / SAVE + QUIT
  await hold('Space', 700);            // SAVE + QUIT TO TITLE
  await hold('Space', 600);            // CONTINUE row
  await hold('Enter', 900);            // slot 1
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await call('dbg_bwentm', 8); await sleep(400);
  ok((await call('dbg_warstate')) === 1, 'v1.17 check 9: after Continue, the parked battle was restored from the save file');
  ok((await call('dbg_warconq')) === 1, 'after Continue: region 1 is STILL conquered');
  ok((await call('dbg_warregion')) === 1, 'v1.17 check 9: the restored battle is region 2 (unlocked, and still in progress)');

  console.log(`\nwartest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
