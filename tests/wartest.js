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
// same conventions as the unit ones).
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
  ok((await call('dbg_warbits')) === 150, 'starting purse is 150 bits', await call('dbg_warbits'));
  ok((await call('dbg_warunits', 0, 0)) === 1, 'one harvester at the start');
  ok((await call('dbg_warunits', 0, 1)) === 1, 'one melee buddy at the start');
  ok((await call('dbg_warunits', 0, 2)) === 0, 'no ranged buddies yet');
  ok((await call('dbg_warhives')) === 1, 'region 1 has exactly one hive');
  ok((await call('dbg_warstructs', 0)) === 1, 'the Compiler Core stands');
  await cv.screenshot({ path: __dirname + '/war-battle.png' });

  // the real tap flow: tap the Core (camera opens on it: world
  // (280,600) -> screen (320,157)), then tap HARVESTER in its bar
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'tapping the Core selects it');
  await cv.screenshot({ path: __dirname + '/war-bar.png' });
  const bitsBeforeTrain = await call('dbg_warbits');
  await tap(48, 337);   // bar slot 0: HARVESTER, 80 bits
  ok((await poll('dbg_warunits', [0, 0], v => v === 2, 14000)) === 2, 'the bar trained a second harvester');
  ok((await call('dbg_warbits')) <= bitsBeforeTrain - 60, 'training charged the purse', await call('dbg_warbits'));

  // deselect by tapping empty ground (recentre first)
  await call('dbg_warcam', 500, 300); await sleep(200);
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'tapping empty ground deselects the building');

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
  await tap(228, 337);   // bar slot 2: ZAPPER
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
    await tap(228, 337);   // arm the Zapper again
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
  await padTap(5);   // RB: light the first bar button (HARVESTER)
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'pad: RB lit bar button 0', await call('dbg_warbarhl'));
  const bitsBar = await call('dbg_warbits');
  await padTap(0);   // A presses the lit button
  ok((await poll('dbg_warbits', [], v => v === bitsBar - 80, 4000)) === bitsBar - 80,
     'pad: A on the lit button trained (80 bits charged)', bitsBar + ' -> ' + (await call('dbg_warbits')));
  ok((await poll('dbg_warunits', [0, 0], v => v === 2, 14000)) === 2, 'pad: the bar-trained harvester rolled out');

  // 5. B cancels a pending placement — nothing built, nothing charged
  await padTap(5); await padTap(5); await padTap(5); await padTap(5);   // RB x4: HARVESTER -> PATCH -> ZAPPER -> FIREWALL
  ok((await poll('dbg_warbarhl', [], v => v === 3, 4000)) === 3, 'pad: RB walked the highlight to FIREWALL (slot 3)', await call('dbg_warbarhl'));
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

  // region 2 is unlocked now
  ok((await call('dbg_warenter', 1)) === 1, 'region 2 battle launches');
  ok((await call('dbg_warhives')) === 1, 'region 2 also fields one hive');
  await key('Escape', 300);   // abandon the battle back to the map
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'Esc abandoned the battle to the map');
  await key('Escape', 300);   // and out of the mode entirely
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'Esc on the map left WAR!');
  ok((await call('dbg_inarcade')) === 1, 'back in the arcade at the WAR! cabinet');

  // persistence: save + quit, Continue — the conquest must survive
  await hold('KeyP', 500);
  await hold('ArrowDown', 250);
  await hold('ArrowDown', 250);        // rows: CONTINUE / CONTROLS / SAVE + QUIT
  await hold('Space', 700);            // SAVE + QUIT TO TITLE
  await hold('Space', 600);            // CONTINUE row
  await hold('Enter', 900);            // slot 1
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await call('dbg_bwentm', 8); await sleep(400);
  ok((await call('dbg_warstate')) === 0, 'after Continue: back on the WAR! world map');
  ok((await call('dbg_warconq')) === 1, 'after Continue: region 1 is STILL conquered');
  ok((await call('dbg_warenter', 1)) === 1, 'after Continue: region 2 is still unlocked');

  console.log(`\nwartest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
