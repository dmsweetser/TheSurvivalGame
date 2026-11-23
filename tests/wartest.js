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
// v1.18 "SHIFTING GROUND" adds dbg_warbarriers (live barrier
// segments) and dbg_warhivexy (first live hive, packed): battles
// are RANDOMIZED now (per-battle seed — banded hives along the far
// side, re-rolled debris, indestructible barrier lines), so every
// position a check needs is read live or scanned for, never
// assumed. Checks 20-24 (the tail) cover the SAVE & QUIT map
// resume and the randomization itself.
// SEED PIN (Nov 17, Daniel: keep the coverage, kill the dice):
// dbg_warseed(v) pins the per-battle seed, so every battle below
// lays out the SAME field and the layout-sensitive family (the
// v1.17 wall/gate/rally/snapshot checks) stops racing the roll.
// Check 22 (novelty) releases the pin for its two battles and the
// pin is re-applied right after it.
// v1.21 (Nov 17) adds dbg_warhbits (the first living hive's purse,
// -1 = no hive) and unit type 8, the BUG HARVESTER: the hive now
// runs a real economy — a per-hive purse (150 on campaign), two
// bug harvesters working the same nodes, waves BOUGHT at the tick
// (crawler 20 / spitter 35, harvester replacements at 40 first,
// guard refills only from surplus). Check 33 soaks it; check 34
// covers the same version's DEMOLISH sell-back (half the build
// cost, floored, two-step arm + confirm; never on the Core).
// v1.27 widens the field roll again: bit nodes jitter off the
// table (±90 x / ±70 y, amounts ±20% — war_jitter_nodes), terrain
// scatters 44..80 features under a per-battle kind-mix profile,
// ~30% of barrier lines run HORIZONTAL, hive x rolls 220 wide and
// garrisons roll 2-3 guards. Consequences for this suite: the
// node-total assertions read the count exactly and the total
// inside its ±20% band (nodesInBand below — the nominal totals
// are the table's 8,200 campaign / 4,550 SMALL); checks 12 and 23
// identify VERTICAL barrier lines from the segment field instead
// of assuming every line is vertical; check 30's move-order
// target is scanned clear of the field instead of assumed to be
// the (430,430) node's clearing. The WAR! cabinet also rose 70px
// in the Arcade: its stand point is (0, -12000+184+58) = (0,-11758).
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
  // v1.27: dbg_warnodes packs count*100000 + total bits; node
  // amounts jitter ±20% each, so assert the count exactly and the
  // total inside [0.8, 1.2] x the table's nominal total
  const nodesInBand = (v, count, nominal) => Math.floor(v / 100000) === count
    && (v % 100000) >= Math.floor(nominal * 0.8) && (v % 100000) <= Math.ceil(nominal * 1.2);

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_adopt', 0);
  ok((await call('dbg_buddy')) !== 0, 'buddy adopted');

  // mall -> arcade -> the brand-new seventh cabinet
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', [], v => v === 1)) === 1, 'in the mall');
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  ok((await poll('dbg_inarcade', [], v => v === 1)) === 1, 'in the arcade');
  await call('dbg_tp', 256, -11898); await sleep(200);
  ok((await call('dbg_cabinet')) === 5, 'the CART cabinet is still cabinet 5', await call('dbg_cabinet'));
  await call('dbg_tp', 0, -11758); await sleep(200);   // v1.27: WAR! stand point = (ARC_SX[6], ARC_CY + ARC_ROWY[6] + 58) = (0, -12000+184+58) — the cabinet rose 70px (ROWY 254 -> 184)
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

  // pin the battle seed for the whole WAR! run (see the header):
  // every battle from here lays out the pinned field, check 22
  // excepted (it releases and re-pins around itself below)
  const WAR_PIN_SEED = 11;
  ok((await call('dbg_warseed', WAR_PIN_SEED)) === WAR_PIN_SEED, 'battle seed pinned for the WAR! checks', await call('dbg_warseed', WAR_PIN_SEED));

  // launch region 1 by tapping its node (top node of the ring:
  // centre (320,188), region 0 at angle -90 -> (320, 73))
  await tap(320, 73);
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1, 'tapping region 1 launched its battle');
  ok((await call('dbg_warbits')) === 300, 'v1.19 check 1: starting purse is 300 bits (was 250)', await call('dbg_warbits'));
  ok(nodesInBand(await call('dbg_warnodes'), 11, 8200), 'v1.19 check 1: eleven bit nodes on the field, total inside the v1.27 ±20% amount-jitter band of the nominal 8,200 (was nine / 4,800)', await call('dbg_warnodes'));
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
  // too short for more than one +30 delivery to land inside it
  // (v1.19: a delivery is a full 30-bit load now, so the bound is -50)
  ok((await call('dbg_warbits')) <= bitsBeforeTrain - 50, 'training charged the purse (80, less at most one delivery)', await call('dbg_warbits'));
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
  // v1.18: the rally point is scanned for, not fixed — random debris
  // on the tap point would swallow the tap (the debris branch eats
  // it before the rally branch), so pick a clear spot near (500,300)
  const tnR0 = await call('dbg_warterrain');
  const featsR0 = [];
  for (let t = 0; t < tnR0; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) featsR0.push(unxy(v)); }
  let rallyPt = null;
  for (const [cx, cy] of [[500, 300], [560, 300], [440, 300], [500, 380], [500, 220], [580, 380], [420, 220], [620, 300]]) {
    if (featsR0.some(([fx, fy]) => Math.hypot(fx - cx, fy - cy) < 32)) continue;
    rallyPt = [cx, cy]; break;
  }
  ok(!!rallyPt, 'rally: a clear field point exists near (500,300)');
  await call('dbg_warcam', rallyPt[0], rallyPt[1]); await sleep(200);
  await tap(320, 157);   // the scanned point: empty ground
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'rally: the Core STAYS selected after a field tap');
  ok((await call('dbg_warrally', 0)) === rallyPt[0] * 10000 + rallyPt[1], 'rally: the field tap set the Core rally point', await call('dbg_warrally', 0));

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
  const xp29 = await call('dbg_buddyxp');   // v1.19 check 29: the buddy's XP, captured at the brink of the FIRST conquest
  await call('dbg_warhurt', 1, 1, 99999);
  ok((await poll('dbg_warstate', [], v => v === 2, 5000)) === 2, 'hive down: victory state');
  ok((await poll('dbg_warstate', [], v => v === 0, 8000)) === 0, 'victory returns to the world map');
  ok((await call('dbg_warconq')) === 1, 'region 1 is conquered (war_prog = 1)');
  ok((await call('dbg_buddyxp')) === xp29 + 60, 'v1.19 check 29: the first conquest of region 1 paid the buddy +60 XP (60 + 10 x region 0)', 'xp ' + xp29 + ' -> ' + (await call('dbg_buddyxp')));

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
  // v1.18: the hive lands banded-random along the far side now —
  // read its live position and set up relative to IT
  const [hvx, hvy] = unxy(await call('dbg_warhivexy'));
  await call('dbg_warmove', sg, hvx - 140, hvy);   // 140u west of the hive: inside its 125+30 range
  ok((await call('dbg_wartapunit', sg)) === 1, 'siege tank selected for the order');
  await call('dbg_warcam', hvx, hvy); await sleep(250);
  const hiveHp0 = await call('dbg_warhurt', 1, 1, 0);
  await tap(320, 157);   // the hive, dead centre: attack order
  ok((await poll('dbg_warhurt', [1, 1, 0], v => v >= 0 && v <= hiveHp0 - 50, 20000, 300)) <= hiveHp0 - 50,
     'the siege tank is shelling the hive (x2 vs structures)', 'hive hp stuck at ' + (await call('dbg_warhurt', 1, 1, 0)));

  // firewalls: grid-snapped segments join into one barrier, and a bug
  // cannot walk through the line
  ok((await call('dbg_warenter', 0)) === 1, 'fresh battle for the firewall checks');
  await call('dbg_waraddbits', 500);
  // v1.27: the four spots are picked by LIVE clearance, not fixed at
  // (500,400) — the broadened field roll (denser debris, jittered
  // nodes) can now sit on the old spots, and war_place_space refuses
  // within rad+26 of a node and rad+trad+4 of a feature (use 40u for
  // both). The anchor stays on the game's snap grid (x a multiple of
  // 24, y = 400 mod 24) and every downstream constant translates
  // with it: bug spawn (ax+136, ay+32), scan box, the ax-14 crossing
  // line — the originals at anchor (504,400) are exactly these.
  const fwBlock = [];
  { const tn = await call('dbg_warterrain');
    for (let t = 0; t < tn; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) fwBlock.push(unxy(v)); }
    const nn = Math.floor((await call('dbg_warnodes')) / 100000);
    for (let n = 0; n < nn; n++) { const v = await call('dbg_warnodexy', n); if (v >= 0) fwBlock.push(unxy(v)); } }
  // war_place_space ALSO refuses near existing structures (the Core
  // and the hives stand in war_s from the first frame): a wall needs
  // rad + other_rad + 8, worst case 13+34+8 = 55 — structures are
  // scanned with a 58u margin, features/nodes with 40u.
  const fwStruct = [];
  for (let s = 0; s < 28; s++) { const v = await call('dbg_warstructxy', s); if (v >= 0) fwStruct.push(unxy(v)); }
  const fwClear = (x, y) => !fwBlock.some(([bx, by]) => Math.hypot(bx - x, by - y) < 40)
                        && !fwStruct.some(([bx, by]) => Math.hypot(bx - x, by - y) < 58);
  const fwSnap = v => Math.round(v / 24) * 24;   // clearance is judged at the SNAPPED spots — warbuildxy snaps before placing (400 lands on 408)
  let ax = -1, ay = -1;
  outer: for (const cx of [504, 480, 528, 456, 552, 432, 576, 408, 600, 384, 624]) {
    for (const cy of [400, 376, 424, 352, 448, 328, 472]) {
      const sy = fwSnap(cy);
      if (fwClear(cx, sy) && fwClear(cx, sy + 24) && fwClear(cx, sy + 48) && fwClear(cx + 24, sy + 48)) { ax = cx; ay = cy; break outer; }
    }
  }
  ok(ax >= 0, 'firewall checks: found a clear anchor for the wall', 'anchor=' + ax + ',' + ay);
  const w1 = await call('dbg_warbuildxy', 3, ax, ay);
  const w2 = await call('dbg_warbuildxy', 3, ax, ay + 24);
  const w3 = await call('dbg_warbuildxy', 3, ax, ay + 48);
  const w4 = await call('dbg_warbuildxy', 3, ax + 24, ay + 48);
  ok(w1 >= 0 && w2 >= 0 && w3 >= 0 && w4 >= 0 && (await call('dbg_warstructs', 3)) === 4,
     'four firewall segments placed', [w1, w2, w3, w4].join(','));
  const snap = v => Math.round(v / 24) * 24;
  const [w1x, w1y] = unxy(await call('dbg_warstructxy', w1));
  const [w2x, w2y] = unxy(await call('dbg_warstructxy', w2));
  const [w3x, w3y] = unxy(await call('dbg_warstructxy', w3));
  const [w4x, w4y] = unxy(await call('dbg_warstructxy', w4));
  ok(w1x === snap(ax) && w2x === w1x && w3x === w1x && w2y - w1y === 24 && w3y - w2y === 24
     && w4y === w3y && w4x - w3x === 24,
     'segments snapped to the 24u grid, shoulder to shoulder',
     `${w1x},${w1y} ${w2x},${w2y} ${w3x},${w3y} ${w4x},${w4y}`);
  await call('dbg_warcam', w1x, w2y); await sleep(400);
  await cv.screenshot({ path: __dirname + '/war2-wall.png' });
  await call('dbg_warspawnbug', ax + 136, ay + 32);
  let bugIdx = -1, minBugX = 1e9;
  for (let s = 0; s < 10; s++) {
    await sleep(500);
    for (let i = 0; i < 80; i++) {
      const v = await call('dbg_warunitxy', i);
      if (v < 0) continue;
      const [ux, uy] = unxy(v);
      if (ux > ax + 56 && ux < ax + 196 && uy > ay - 20 && uy < ay + 90) { bugIdx = i; if (ux < minBugX) minBugX = ux; }
    }
  }
  ok(bugIdx >= 0 && minBugX > ax - 14, 'the firewall line held: the crawler never crossed',
     'bugIdx=' + bugIdx + ' minX=' + minBugX);

  // ================= round 3 (Nov 16): destructible debris ========
  // terrain features have hp now: a melee buddy ordered onto one
  // through the REAL tap flow smashes it, the ground frees up, and
  // the siege gun gets NO x2 structure bonus against a rock.
  const tn3 = await call('dbg_warterrain');
  const alive0 = await call('dbg_warfeatn');
  ok(alive0 === tn3 && tn3 > 30, 'debris: every feature starts standing', alive0 + '/' + tn3);
  const featHp = async t => { const f = await call('dbg_warfeat', t); return f < 0 ? -1 : Math.floor(f / 100); };
  // low-hp features (tree 35 / rubble 25) in the western half, well
  // clear of the firewall line and its crawler (ax+6, ay+32 — the
  // v1.27 live anchor; 510,432 at the original anchor) — and
  // (v1.18) with >=55u of open ground around them: a tree penned in
  // by barrier segments can be impossible to stand next to, so the
  // check works down a CANDIDATE LIST and fells the first one the
  // melee can actually reach and damage
  const pts3 = [];
  for (let t = 0; t < tn3; t++) {
    const f = await call('dbg_warfeat', t);
    if (f < 0) continue;
    const xy = unxy(await call('dbg_warfeatxy', t));
    if (xy[0] >= 0) pts3.push([t, f % 100, xy]);
  }
  const cand3 = pts3.filter(([t, kind, xy]) =>
    (kind === 0 || kind === 4) && xy[0] <= 700 && Math.hypot(xy[0] - (ax + 6), xy[1] - (ay + 32)) >= 220 &&
    !pts3.some(([t2, , xy2]) => t2 !== t && Math.hypot(xy2[0] - xy[0], xy2[1] - xy[1]) < 55));
  ok(cand3.length > 0, 'debris: found a reachable tree/rubble feature to fell', 'candidates=' + cand3.length);
  let featT = -1, featXY = null, hpStart = -1, hpMid = -1;
  const miD = await call('dbg_warfind', 1);
  for (const [ct, , cxy] of cand3.slice(0, 3)) {
    await call('dbg_warmove', miD, cxy[0] + 30, cxy[1]);
    ok((await call('dbg_wartapunit', miD)) === 1, 'debris: melee buddy selected');
    await call('dbg_warcam', cxy[0], cxy[1]); await sleep(250);
    const hs = await featHp(ct);
    let hm = hs;
    for (let attempt = 0; attempt < 2 && hm >= hs; attempt++) {   // one re-order if the first tap got eaten
      const fp = unpos(await call('dbg_warfeatpos', ct));
      await tap(fp[0], fp[1]);   // tap the feature itself: attack order
      const pv = await poll('dbg_warfeat', [ct], v => v >= 0 && Math.floor(v / 100) < hs, 5000, 200);
      hm = pv >= 0 ? Math.floor(pv / 100) : hs;
    }
    if (hm < hs) { featT = ct; featXY = cxy; hpStart = hs; hpMid = hm; break; }   // this one is taking damage — fell it
  }
  ok(featT >= 0, 'debris: a candidate feature is taking damage', 't=' + featT);
  if (featT >= 0) {
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
  // bar, B peels a layer back, the right stick flies the cursor too
  // with the camera centred on it (v1.25; it panned screen-locked
  // until Daniel's Nov 19 centred-cursor directive).
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

  // 6 (early, while the camera is known): the right stick FLIES the
  // cursor and the camera centres on it. DERIVATION (Nov 19, 2026):
  // this check used to assert the v1.16 model — right stick pans the
  // camera, cursor keeps its SCREEN spot. Daniel's Nov 19 directive
  // supersedes that model outright ("the cursor to always be
  // centered... allow the view to go off screen"): the pan is what
  // stranded the cursor off-centre. The new assertions are STRONGER,
  // not weaker: the cursor's arena x must advance with the stick
  // (the old check never proved the cursor moved at all), its screen
  // spot must be the field centre (320,157) before AND after (the
  // old check allowed any constant offset), and the camera must sit
  // ON the cursor. scrOf reads a reference pair + the cursor in ONE
  // evaluate. The ruler is ANY unit or terrain feature whose screen
  // pos is honest: the pos exports clamp negative screen coords to
  // 0, and the mustered melee roams off and fights, so pinning the
  // ruler to it read garbage (screen y=185-194 for a cursor the
  // scanning version of this same flow reads at 320,157).
  const scrOf = async () => page.evaluate(() => {
    const T = window.__TSG;
    const cur = T.dbg_warcur();
    const qx = Math.floor(cur / 10000), qy = cur % 10000;
    const derive = (axy, apos) => {
      if (axy < 0 || apos < 0) return null;
      const ax = Math.floor(axy / 10000), ay = axy % 10000;
      const sx = apos % 10000, sy = Math.floor(apos / 10000);
      if (sx < 2 || sy < 2) return null;
      const camx = ax - (sx - 320), camy = ay - (sy - 180 + 23);
      return [320 + (qx - camx), 180 + (qy - camy - 23), camx, qx];
    };
    for (let i = 0; i < 80; i++) { const r = derive(T.dbg_warunitxy(i), T.dbg_warunitpos(i)); if (r) return r; }
    const tn = T.dbg_warterrain();
    for (let t = 0; t < tn; t++) { const r = derive(T.dbg_warfeatxy(t), T.dbg_warfeatpos(t)); if (r) return r; }
    return [-999, -999, -999, qx];
  });
  const scr0 = await scrOf();
  await padAxes(0, 0, 1, 0); await sleep(500); await padAxes(0, 0, 0, 0); await sleep(200);
  const scr1 = await scrOf();
  ok(Math.abs(scr0[0] - 320) <= 8 && Math.abs(scr0[1] - 157) <= 8 &&
     Math.abs(scr1[0] - 320) <= 8 && Math.abs(scr1[1] - 157) <= 8 &&
     scr1[3] > scr0[3] + 60 && Math.abs(scr1[2] - scr1[3]) <= 3,
     'pad: right stick flew the cursor, camera centred on it (v1.25 centred model)',
     `screen ${scr0[0]},${scr0[1]} -> ${scr1[0]},${scr1[1]}, curx ${scr0[3]} -> ${scr1[3]}, camx ${scr1[2]}`);

  // 4. the command bar by pad: select the Core with A — v1.20: the
  // selection captures the bar with the first button already lit —
  // and A works the menu through to a trained unit. Getting there takes two wakes: the
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
  // v1.22: the wake spawn IS the camera centre now (280,600). Before
  // the fix this asserted (280,623) — the old spawn rode WAR_BAR_H/2
  // below centre and only the Core's fat 42u tap disc absorbed it.
  ok((await poll('dbg_warcur', [], v => v === 2800600, 4000)) === 2800600, 'pad: A summoned the cursor at the camera centre (280,600)',
     await call('dbg_warcur'));
  await padTap(0);   // A again: the tap itself — the Core is at the camera's centre
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'pad: A at the camera centre selected the Core');
  ok((await call('dbg_warsel')) === 0, 'pad: selecting the Core cleared the unit selection');
  // v1.20: the pad selection CAPTURED the bar on the spot — the
  // highlight already sits on the first bar button (the TRAIN
  // category); no RB is needed to light it. (Pre-v1.20 this section
  // RB-lit the button from dark; capture supersedes that contract.)
  ok((await poll('dbg_warbarcap', [], v => v === 1, 4000)) === 1, 'pad: selecting the Core captured the bar (v1.20)');
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'pad: capture lit bar button 0 (TRAIN)', await call('dbg_warbarhl'));
  await padTap(0);   // A opens the category
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'pad: A on TRAIN opened the category row');
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'pad: the highlight returned to option 0 (HARVESTER)');
  const bitsBar = await call('dbg_warbits');
  await padTap(0);   // A presses the lit option
  ok((await poll('dbg_warbits', [], v => v === bitsBar - 80, 4000)) === bitsBar - 80,
     'pad: A on the lit button trained (80 bits charged)', bitsBar + ' -> ' + (await call('dbg_warbits')));
  ok((await poll('dbg_warunits', [0, 0], v => v === 2, 14000)) === 2, 'pad: the bar-trained harvester rolled out');

  // 5. B cancels a pending placement — nothing built, nothing charged.
  // v1.20 drive (capture still held from the TRAIN activation): B
  // peels the open TRAIN category shut but STAYS captured, with the
  // highlight on the category just closed; RB walks the category
  // row to DEFENSE, A opens it (highlight back on its options),
  // RB walks its options to FIREWALL, A arms it — arming hands the
  // field cursor back (capture ends) — then B cancels as ever.
  await padTap(1);   // B: peel the open category shut, still captured
  ok((await poll('dbg_warbcat', [], v => v === -1, 4000)) === -1, 'pad: B peeled the open category shut');
  ok((await call('dbg_warbarcap')) === 1 && (await call('dbg_warbarhl')) === 0,
     'pad: ...still captured, highlight on the closed TRAIN category',
     'cap=' + (await call('dbg_warbarcap')) + ' hl=' + (await call('dbg_warbarhl')));
  await padTap(5); await padTap(5);   // RB x2: TRAIN -> BUILD -> DEFENSE
  ok((await poll('dbg_warbarhl', [], v => v === 2, 4000)) === 2, 'pad: RB walked the highlight to DEFENSE (slot 2)', await call('dbg_warbarhl'));
  await padTap(0);   // A opens DEFENSE
  ok((await poll('dbg_warbcat', [], v => v === 2, 4000)) === 2, 'pad: A opened the DEFENSE category');
  await padTap(5);   // RB: ZAPPER -> FIREWALL (the highlight already sat on option 0)
  ok((await poll('dbg_warbarhl', [], v => v === 1, 4000)) === 1, 'pad: RB walked the options to FIREWALL (slot 1)', await call('dbg_warbarhl'));
  await padTap(0);
  ok((await poll('dbg_warplace', [], v => v === 3, 4000)) === 3, 'pad: A on FIREWALL armed its placement');
  ok((await call('dbg_warbarcap')) === 0, 'pad: arming the placement ended the capture (the field cursor has it now)');
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

  // check 2: a harvester load can reach the carry cap (25 in
  // v1.17, raised to 30 by v1.19's richer fields)
  ok((await poll('dbg_warcarry', [], v => v >= 30, 60000, 400)) >= 30,
     'v1.19 check 2: a harvester scooped a full 30-bit load (WAR_CARRY)', 'carry=' + (await call('dbg_warcarry')));

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
  ok((await call('dbg_warunits', 0, 0)) === 0, 'v1.17 check 3: harvester down (economy isolated, no grant at 800 bits)');
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
  // v1.27: the line's column is picked by LIVE clearance (check 5's
  // method, extended to nodes and structures): the broadened roll
  // can park debris or a jittered node on the old fixed x=360, and
  // war_place_space refuses within rad+26 of a node / rad+trad+4 of
  // a feature (40u scan margin covers both) and rad+other_rad+8 of
  // an existing structure — the Core and hives included (58u scan
  // margin). Westernmost column with >=16 of 17 spots clear wins,
  // so the line stays west of the nodes as the check intends; the
  // crossing line is lineX4+20 (380 at x=360).
  const block4 = [];
  { const tn = await call('dbg_warterrain');
    for (let t = 0; t < tn; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) block4.push(unxy(v)); }
    const nn = Math.floor((await call('dbg_warnodes')) / 100000);
    for (let n = 0; n < nn; n++) { const v = await call('dbg_warnodexy', n); if (v >= 0) block4.push(unxy(v)); } }
  // structures too (Core + hives, 58u margin — see the firewall scan)
  const struct4 = [];
  for (let s = 0; s < 28; s++) { const v = await call('dbg_warstructxy', s); if (v >= 0) struct4.push(unxy(v)); }
  const open4 = (lx, y) => !block4.some(([bx, by]) => Math.hypot(bx - lx, by - y) < 40)
                        && !struct4.some(([bx, by]) => Math.hypot(bx - lx, by - y) < 58);
  const cand4 = [360, 336, 384, 312, 408, 288, 432].map(lx => {
    let open = 0;
    for (let y = 408; y <= 792; y += 24) { if (y === 600) continue; if (open4(lx, y)) open++; }
    return [lx, open];
  });
  const clear4 = cand4.filter(([, open]) => open >= 16);
  const pick4 = clear4.length ? clear4.reduce((a, b) => (b[0] < a[0] ? b : a)) : cand4.reduce((a, b) => (b[1] > a[1] ? b : a));
  const lineX4 = pick4[0];
  let wallN4 = 0;
  for (let y = 408; y <= 792; y += 24) { if (y === 600) continue; if ((await call('dbg_warbuildxy', 3, lineX4, y)) >= 0) wallN4++; }
  ok(wallN4 >= 14, 'v1.17 check 4: the wall line went up west of the nodes (gap at y=600)', 'segments=' + wallN4 + ' at x=' + lineX4);
  const bitsPath0 = await call('dbg_warbits');
  const harvP = await call('dbg_warfind', 0);
  let crossed4 = false;
  const tPath0 = Date.now();
  while (Date.now() - tPath0 < 90000) {
    const v = await call('dbg_warunitxy', harvP);
    if (v >= 0 && Math.floor(v / 10000) > lineX4 + 20) crossed4 = true;
    if (crossed4 && (await call('dbg_warbits')) >= bitsPath0 + 25) break;
    await sleep(250);
  }
  ok(crossed4, 'v1.17 check 4: the harvester crossed the wall line (x>lineX+20)');
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
  // v1.18: the wall's column is picked by clearance, not fixed at
  // 700 — a random barrier line can stand near x=700 and refuse
  // half the segments, and a bug approaching from the east can get
  // POCKETED between the wall and a parallel barrier line (the A*
  // grid sees no free cell in a ~25u corridor). So the column
  // anchors ~200u WEST of the westernmost barrier line: the bait
  // (lineX-60), the wall and the bug's spawn (lineX+90) all stay
  // clear of every barrier, and the bug's detour around the wall's
  // ends never meets one. The bait/bug geometry translates with
  // the column, keeping the aggro math the comment above describes.
  const tn5 = await call('dbg_warterrain');
  const feats5 = [];
  let barWest5 = 1e9;
  for (let t = 0; t < tn5; t++) {
    const v = await call('dbg_warfeatxy', t); if (v < 0) continue;
    const xy = unxy(v); feats5.push(xy);
    const f = await call('dbg_warfeat', t);
    if (f >= 0 && f % 100 === 5 && xy[0] < barWest5) barWest5 = xy[0];
  }
  // v1.27: nodes block placement too (war_place_space refuses within
  // rad+26 of a node) and they jitter now — fold their live positions
  // into the same clearance scan (they are not barriers, so they stay
  // out of barWest5 above).
  { const nn = Math.floor((await call('dbg_warnodes')) / 100000);
    for (let n = 0; n < nn; n++) { const v = await call('dbg_warnodexy', n); if (v >= 0) feats5.push(unxy(v)); } }
  // structures too (Core + hives, 58u margin — see the firewall scan)
  const struct5 = [];
  for (let s = 0; s < 28; s++) { const v = await call('dbg_warstructxy', s); if (v >= 0) struct5.push(unxy(v)); }
  const baseX5 = Math.max(440, Math.min(720, barWest5 - 200));
  // v1.27 (cont.): the widened roll can close EVERY column of the
  // old candidate set. In the pinned field the barrier constraint
  // (lx+110 <= barWest5) leaves a single column, and it is blocked
  // twice over — a debris cluster on its mid-span and a jittered
  // node on its south end — so only 14 of the 18 spots build there
  // (probe-verified against war_place_space's exact rules; the
  // game is right, the search was too narrow). The search now also
  // slides the wall's SPAN (the 384..816 window, the gap still at
  // y=600) in 24u steps: (column, shift) pairs are scored by the
  // same live clearance model, preferring the most open, then the
  // column nearest the ~200-west anchor, then the unshifted span.
  // In the pinned field the winner is the anchor column with the
  // span slid 48 north (336..768): the node falls outside the span
  // and 16 of 18 build. The bait, the spawn and the crossing line
  // all key off lineX5 exactly as before, and the assertion is
  // unchanged: >=16 of the 18 spots must actually build, and the
  // bug must still cross gap-or-end, never through.
  let lineX5 = baseX5, shift5 = 0, bestOpen5 = -1, bestPref5 = 1e9;
  for (let lx = 336; lx <= 760 && lx + 110 <= barWest5; lx += 24) {
    for (const sh of [0, -24, 24, -48]) {
      let open = 0;
      for (let y = 384 + sh; y <= 816 + sh; y += 24) { if (y === 600) continue;
        if (!feats5.some(([fx, fy]) => Math.hypot(fx - lx, fy - y) < 40)
         && !struct5.some(([fx, fy]) => Math.hypot(fx - lx, fy - y) < 58)) open++; }
      const pref = Math.abs(lx - baseX5) * 100 + Math.abs(sh);
      if (open > bestOpen5 || (open === bestOpen5 && pref < bestPref5)) { bestOpen5 = open; bestPref5 = pref; lineX5 = lx; shift5 = sh; }
    }
  }
  let wallN5 = 0;
  for (let y = 384 + shift5; y <= 816 + shift5; y += 24) { if (y === 600) continue; if ((await call('dbg_warbuildxy', 3, lineX5, y)) >= 0) wallN5++; }
  ok(wallN5 >= 16, 'v1.17 check 5: the wall line went up (gap at y=600)', 'segments=' + wallN5 + ' at x=' + lineX5);
  const miBait = await call('dbg_warfind', 1);
  await call('dbg_warmove', miBait, lineX5 - 60, 430);
  const harvBait = await call('dbg_warfind', 0);
  await call('dbg_warmove', harvBait, 200, 1100);   // park the harvester far west — east-side nodes would make IT the bug's nearest unit target
  await call('dbg_warspawnbug', lineX5 + 90, 470);
  let bugI = -1;
  for (let i = 0; i < 80; i++) {
    await call('dbg_warmove', miBait, lineX5 - 60, 430);   // keep the bait glued down while we look — unpinned, it charges the bug and kills it east of the line
    const v = await call('dbg_warunitxy', i);
    if (v < 0) continue;
    const [ux, uy] = unxy(v);
    if (ux > lineX5 + 15 && Math.abs(uy - 470) < 60) { bugI = i; break; }
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
      await call('dbg_warmove', miBait, lineX5 - 60, 430);   // pin the bait west of the line
      const v = await call('dbg_warunitxy', bugI);
      if (v >= 0 && Math.floor(v / 10000) <= lineX5 - 14) { crossed5 = true; break; }
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
    // v1.18: spawned at 545, not 570 — barrier lines start at
    // x>=560, and one standing at the old spawn could detour the
    // bug around the wall's end instead of onto the gate
    await call('dbg_warspawnbug', 545, 600);
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
  // neither the line's x nor its gate spot is fixed: scan the
  // terrain field and put the line + gate where the clearance is
  // best (clearance >= 62 also covers the gate's wall neighbours:
  // debris refusal reaches ~37u, and a neighbour sits 24u away).
  // v1.18: candidate columns also keep >=64u from any BARRIER
  // line's x — a parallel barrier would refuse the wall's segments
  // en masse along its whole span, which the spot scan can't see.
  const tn12 = await call('dbg_warterrain');
  const feats12 = [];
  const barrierXs = new Set();
  { // v1.27: ~30% of barrier lines run HORIZONTAL — a horizontal
    // line crosses any column at a single spot (the clearance scan
    // below already prices single spots), so barrierXs keeps only
    // VERTICAL lines: an x is one when >=4 barrier segments share
    // it (a horizontal line contributes at most one segment per x)
    const segX12 = new Map();
    for (let t = 0; t < tn12; t++) {
      feats12.push(unxy(await call('dbg_warfeatxy', t)));
      const f = await call('dbg_warfeat', t);
      if (f >= 0 && f % 100 === 5) segX12.set(feats12[t][0], (segX12.get(feats12[t][0]) || 0) + 1);
    }
    for (const [bx, n] of segX12) if (n >= 4) barrierXs.add(bx);
  }
  let lineX = -1, gateY = -1, bestClear = -1;
  for (const lx of [816, 864, 912, 960]) {
    if ([...barrierXs].some(bx => Math.abs(bx - lx) < 64)) continue;
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
  // v1.18: the gate's spot is scanned for — the old fixed (600,300)
  // sits in barrier-line country now and a line there refuses it
  const tn13 = await call('dbg_warterrain');
  const feats13 = [];
  for (let t = 0; t < tn13; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) feats13.push(unxy(v)); }
  let gateSpot = null;
  for (const [cx, cy] of [[600, 300], [500, 300], [700, 300], [600, 450], [450, 450], [750, 450], [600, 200], [500, 800], [700, 800], [400, 300]]) {
    if (feats13.some(([fx, fy]) => Math.hypot(fx - cx, fy - cy) < 48)) continue;
    gateSpot = [cx, cy]; break;
  }
  ok(!!gateSpot, 'v1.17 check 13: a clear gate spot exists');
  const gateS = gateSpot ? await call('dbg_warbuildxy', 6, gateSpot[0], gateSpot[1]) : -1;
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
  await call('dbg_warhurt', 0, harvQ15, 99999);   // isolate the purse (300 bits: no grant)
  const pbQ15 = await call('dbg_warbuildat', 1);   // 300 - 150 = 150 bits left...
  await call('dbg_waraddbits', -50);   // v1.19: the richer purse (300, was 250) is trimmed back to this check's exact 100-bit premise below
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
  // v1.18: the rally offset is scanned for — random debris on the
  // tap point would swallow the tap before it could plant the flag
  const tn17r = await call('dbg_warterrain');
  const feats17r = [];
  for (let t = 0; t < tn17r; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) feats17r.push(unxy(v)); }
  const structs17r = [];
  for (let s = 0; s < 28; s++) { const v = await call('dbg_warstructxy', s); if (v >= 0) structs17r.push(unxy(v)); }
  let rallyOff = null;
  for (const [ox, oy] of [[120, 0], [-120, 0], [0, 120], [0, -120], [120, 120], [-120, -120], [120, -120], [-120, 120]]) {
    const px = rx + ox, py = ry + oy;
    if (px < 60 || px > 1540 || py < 60 || py > 1140) continue;
    if (feats17r.some(([fx, fy]) => Math.hypot(fx - px, fy - py) < 32)) continue;
    if (structs17r.some(([sx2, sy2]) => Math.hypot(sx2 - px, sy2 - py) < 48)) continue;   // never on a building (the Core included)
    rallyOff = [ox, oy]; break;
  }
  ok(!!rallyOff, 'v1.17 check 17: a clear rally offset exists');
  const rdx = rallyOff ? rallyOff[0] : 120, rdy = rallyOff ? rallyOff[1] : 0;
  await tap(320 + rdx, 157 + rdy);
  const rallyV = await call('dbg_warrally', pbR);
  ok(rallyV >= 0 && Math.abs(Math.floor(rallyV / 10000) - (rx + rdx)) <= 1 && Math.abs((rallyV % 10000) - (ry + rdy)) <= 1,
     'v1.17 check 17: the rally point is where the field was tapped', 'rally=' + rallyV + ' want ~' + (rx + rdx) + ',' + (ry + rdy));
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
      return Math.hypot(ux - (rx + rdx), uy - (ry + rdy)) < 30;
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
  // v1.18: the rally tap point is scanned for — the old fixed
  // (380,643) sits just outside the Core's debris clearance, where
  // a random piece can now swallow the tap. Any clear point on the
  // ~110u ring does; the round trip below reads it back live.
  const tn18 = await call('dbg_warterrain');
  const feats18 = [];
  for (let t = 0; t < tn18; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) feats18.push(unxy(v)); }
  let rallyPt18 = null;
  for (const [cx, cy] of [[390, 600], [380, 643], [280, 490], [280, 710], [170, 600], [358, 522], [358, 678], [202, 522], [202, 678]]) {
    if (feats18.some(([fx, fy]) => Math.hypot(fx - cx, fy - cy) < 45)) continue;
    rallyPt18 = [cx, cy]; break;
  }
  ok(!!rallyPt18, 'v1.17 check 18: a clear rally point exists near the Core');
  await tap(320 + (rallyPt18[0] - 280), 157 + (rallyPt18[1] - 600));   // the scanned point: the rally tap
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
  // v1.18: the "empty field" tap point is scanned for — random
  // debris there would eat the tap (the debris branch returns
  // before the deselect fallthrough) and the Zapper would stay
  // selected. Barriers are safe (the scan skips them); debris not.
  const tn19 = await call('dbg_warterrain');
  const feats19 = [];
  for (let t = 0; t < tn19; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) feats19.push(unxy(v)); }
  const structs19 = [];
  for (let s = 0; s < 28; s++) { const v = await call('dbg_warstructxy', s); if (v >= 0) structs19.push(unxy(v)); }
  let deselOff = null;
  for (const [ox, oy] of [[0, -97], [120, -97], [-120, -97], [0, 120], [150, 60], [-150, 60], [180, 0], [-180, 0]]) {
    const px = szx + ox, py = szy + oy;
    if (feats19.some(([fx, fy]) => Math.hypot(fx - px, fy - py) < 28)) continue;
    if (structs19.some(([sx2, sy2]) => Math.hypot(sx2 - px, sy2 - py) < 44)) continue;
    deselOff = [ox, oy]; break;
  }
  ok(!!deselOff, 'v1.17 check 19: a clear deselect point exists');
  await tap(320 + deselOff[0], 157 + deselOff[1]);   // empty field by the Zapper
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
  const nodesFresh8 = await call('dbg_warnodes');   // v1.27: this fresh battle's exact node reading — the restore below must match IT, not a fixed total
  ok((await call('dbg_warbits')) === 300 && (await call('dbg_warunits', 0, -1)) === 2 && nodesInBand(nodesFresh8, 11, 8200),
     'v1.19 check 8: fresh start — 300 bits, the two starting units, full nodes (v1.27 jitter band)',
     'bits=' + (await call('dbg_warbits')) + ' units=' + (await call('dbg_warunits', 0, -1)) + ' nodes=' + nodesFresh8);
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
  ok((await poll('dbg_warstate', [], v => v === 1, 4000)) === 1 && (await call('dbg_warnodes')) === nodesFresh8,
     'v1.17 check 8: re-entering restores the FRESH battle — the old parked fight is gone (nodes match the fresh reading exactly)',
     'nodes=' + (await call('dbg_warnodes')) + ' vs fresh ' + nodesFresh8);

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

  // ================= v1.18 "SHIFTING GROUND" (Nov 16) =============
  // checks 20-24. Check 20 is Daniel's repro, folded in from the
  // old probe-sqmap.js: SAVE & QUIT, then tap the SAME region's
  // node on the world map — v1.17 started a FRESH battle there and
  // the next autosave erased the parked one. Map node positions
  // come from war_node_pos over the 640x360 view (centre (320,194),
  // radii (230.4, 106.2), region r at angle -PI/2 + r*PI/4).
  const nodeXY = r => {
    const a = -Math.PI / 2 + r * Math.PI / 4;
    return [Math.round(320 + Math.cos(a) * 230.4), Math.round(194 + Math.sin(a) * 106.2)];
  };
  // readings taken with the pause menu open read a frozen battle
  // exactly (the check-7 pattern); leaves the pause menu UP
  const frozenReadings = async () => {
    await key('Escape', 300);
    await poll('dbg_warpause', [], v => v === 1, 4000);
    return { bits: await call('dbg_warbits'), units: await call('dbg_warunits', 0, -1), nodes: await call('dbg_warnodes') };
  };
  const saveQuitToMap = async () => {   // the pause menu is open
    await tap(320, 234);   // SAVE & QUIT
    await poll('dbg_warstate', [], v => v === 0, 4000);
    await sleep(600);   // SAVE & QUIT arms a 0.3s map grace (the closing press must not leak a launch) — let it lapse before tapping a node
  };
  const sameReadings = (a, b) => a.bits === b.bits && a.units === b.units && a.nodes === b.nodes;
  const readingsStr = r => r.bits + '/' + r.units + '/' + r.nodes;
  // state+region+snap read in ONE evaluate, between frames: a
  // restore consumes the snapshot, but the 5s autosave re-fills the
  // buffer from the live battle — separate reads race that refill,
  // so assertions about the consumption instant read all three at
  // the same instant the battle appears
  const instantWar = () => page.evaluate(() => [window.__TSG.dbg_warstate(), window.__TSG.dbg_warregion(), window.__TSG.dbg_warsnap()]);
  const pollInstant = async () => { const t0 = Date.now(); let v = await instantWar(); while (v[0] !== 1 && Date.now() - t0 < 6000) { await sleep(120); v = await instantWar(); } return v; };

  // check 20: park the frontier (region 2) battle, tap region 2's
  // node on the map — the PARKED battle must come back, consumed
  const r20 = await frozenReadings();
  await saveQuitToMap();
  ok((await call('dbg_warstate')) === 0 && (await call('dbg_warsnap')) === 1, 'v1.18 check 20: battle parked, back on the map');
  await cv.screenshot({ path: __dirname + '/war-map-progress.png' });   // the IN PROGRESS marker lives here
  const [n1x, n1y] = nodeXY(1);
  await tap(n1x, n1y);   // region 2's node — the player move from the repro
  const inst20 = await pollInstant();
  ok(inst20[0] === 1, 'v1.18 check 20: the map tap re-entered a battle');
  ok(inst20[1] === 1 && inst20[2] === 0,
     'v1.18 check 20: the PARKED battle was restored (snapshot consumed), not a fresh one',
     'region=' + inst20[1] + ' snap=' + inst20[2]);
  const r20b = await frozenReadings();
  ok(sameReadings(r20b, r20), 'v1.18 check 20: bits, army and nodes match the parked battle exactly',
     readingsStr(r20b) + ' vs parked ' + readingsStr(r20));
  await tap(320, 146);   // RESUME — close the pause menu again

  // check 21: the park survives a detour — park the frontier
  // battle, REPLAY a conquered region from the map, win it, leave
  // the cabinet, re-enter: the FRONTIER battle must restore
  const r21 = await frozenReadings();
  await saveQuitToMap();
  ok((await call('dbg_warsnap')) === 1, 'v1.18 check 21: frontier battle parked again');
  const [n0x, n0y] = nodeXY(0);
  await tap(n0x, n0y);   // region 1: conquered — a fresh replay
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1 && (await call('dbg_warregion')) === 0,
     'v1.18 check 21: the conquered region started a fresh replay');
  ok((await call('dbg_warsnap')) === 1, 'v1.18 check 21: the frontier park survived starting another battle');
  await call('dbg_warhurt', 1, 1, 99999);   // raze the replay's hive (struct 1)
  ok((await poll('dbg_warstate', [], v => v === 2, 5000)) === 2, 'v1.18 check 21: replay won');
  ok((await poll('dbg_warstate', [], v => v === 0, 8000)) === 0, 'v1.18 check 21: back on the map');
  ok((await call('dbg_warsnap')) === 1, 'v1.18 check 21: winning the replay did NOT clear the frontier park');
  await key('Escape', 300);
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'v1.18 check 21: left WAR!');
  await call('dbg_bwentm', 8); await sleep(500);
  const inst21 = await pollInstant();
  ok(inst21[0] === 1, 'v1.18 check 21: re-entering the cabinet restored a battle');
  ok(inst21[1] === 1, 'v1.18 check 21: it is the FRONTIER battle, not the replay', 'region=' + inst21[1]);
  const r21b = await frozenReadings();
  ok(sameReadings(r21b, r21), 'v1.18 check 21: bits, army and nodes match the frontier park exactly',
     readingsStr(r21b) + ' vs parked ' + readingsStr(r21));
  await tap(320, 146);   // RESUME

  // check 22: novelty — two consecutive battles of the SAME region
  // (RESTART between) roll different fields: the hive moves along
  // the far side, the terrain re-rolls. The seed pin comes OFF for
  // this check only: its two battles must roll free.
  ok((await call('dbg_warseed', 0)) === 0, 'check 22: seed pin released (novelty rolls free)');
  ok((await call('dbg_warenter', 0)) === 1, 'v1.18 check 22: battle A');
  await sleep(500);
  const hiveA = await call('dbg_warhivexy'), tnA22 = await call('dbg_warterrain'), barA22 = await call('dbg_warbarriers');
  const fA0 = await call('dbg_warfeatxy', 0), fA10 = await call('dbg_warfeatxy', 10);
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'v1.18 check 22: pause menu up');
  await tap(320, 190);   // RESTART BATTLE — a fresh roll of the same region
  ok((await poll('dbg_warpause', [], v => v === 0, 4000)) === 0 && (await call('dbg_warstate')) === 1,
     'v1.18 check 22: RESTART rolled battle B');
  await sleep(300);
  const hiveB = await call('dbg_warhivexy'), tnB22 = await call('dbg_warterrain'), barB22 = await call('dbg_warbarriers');
  const fB0 = await call('dbg_warfeatxy', 0), fB10 = await call('dbg_warfeatxy', 10);
  ok(hiveA >= 0 && hiveB >= 0 && hiveA !== hiveB, 'v1.18 check 22: the hive MOVED between battles of the same region',
     'hive ' + hiveA + ' -> ' + hiveB);
  ok(Math.floor(hiveA / 10000) >= 1200 && Math.floor(hiveB / 10000) >= 1200,
     'v1.18 check 22: both hives sit along the far (east) side', hiveA + ' / ' + hiveB);
  ok(tnA22 !== tnB22 || barA22 !== barB22 || fA0 !== fB0 || fA10 !== fB10,
     'v1.18 check 22: the terrain re-rolled too', 'tn ' + tnA22 + '->' + tnB22 + ', barriers ' + barA22 + '->' + barB22);

  // novelty proven — the pin goes back on for the rest of the run
  ok((await call('dbg_warseed', WAR_PIN_SEED)) === WAR_PIN_SEED, 'check 22 done: battle seed re-pinned');

  // check 23: barriers — they exist, a unit paths THROUGH the gap
  // (or around an end), and tapping one is a move order, never an
  // attack (they are indestructible)
  ok((await call('dbg_warenter', 0)) === 1, 'v1.18 check 23: fresh battle');
  await sleep(500);
  const barN = await call('dbg_warbarriers');
  ok(barN > 20, 'v1.18 check 23: barrier segments stand on the field', 'count=' + barN);
  const tn23 = await call('dbg_warterrain');
  const segs23 = [];   // [terrainIdx, x, y] of every barrier segment
  for (let t = 0; t < tn23; t++) {
    const f = await call('dbg_warfeat', t);
    if (f < 0 || f % 100 !== 5) continue;
    const [fx, fy] = unxy(await call('dbg_warfeatxy', t));
    segs23.push([t, fx, fy]);
  }
  // v1.27: lines can run horizontal now, so "the west barrier
  // line" is read as the VERTICAL line — the x the most barrier
  // segments share (a horizontal line spreads one segment per x
  // across its whole span and can never win this count)
  let lineX23 = -1, lineCount23 = 0;
  { const byX23 = new Map();
    for (const [, sx] of segs23) byX23.set(sx, (byX23.get(sx) || 0) + 1);
    for (const [sx, n] of byX23) if (n > lineCount23) { lineCount23 = n; lineX23 = sx; } }
  const lineYs = segs23.filter(([, sx]) => Math.abs(sx - lineX23) < 6).map(([, , sy]) => sy).sort((a, b) => a - b);
  // the passage: the widest internal gap in the line, else an open end
  let gapY23 = -1, widest = 0;
  for (let i = 1; i < lineYs.length; i++) {
    const jump = lineYs[i] - lineYs[i - 1];
    if (jump > widest) { widest = jump; gapY23 = (lineYs[i] + lineYs[i - 1]) / 2; }
  }
  if (widest < 96) gapY23 = lineYs[0] >= 200 ? lineYs[0] - 70 : lineYs[lineYs.length - 1] + 70;
  ok(lineYs.length >= 8 && gapY23 > 0, 'v1.18 check 23: the vertical barrier line has a passage', 'segments=' + lineYs.length + ' gapY=' + Math.round(gapY23));
  const mi23 = await call('dbg_warfind', 1);
  await call('dbg_warmove', mi23, lineX23 - 80, Math.round(gapY23));
  ok((await call('dbg_wartapunit', mi23)) === 1, 'v1.18 check 23: melee selected');
  await call('dbg_warcam', lineX23, Math.round(gapY23)); await sleep(250);
  await cv.screenshot({ path: __dirname + '/war-barrier.png' });
  await tap(440, 157);   // world (lineX+120, gapY): east of the line
  const crossed23 = await poll('dbg_warunitxy', [mi23], v => v >= 0 && Math.floor(v / 10000) > lineX23 + 60, 30000, 250);
  ok(crossed23 >= 0 && Math.floor(crossed23 / 10000) > lineX23 + 60,
     'v1.18 check 23: the melee crossed the barrier line (through the gap / around an end)', 'xy=' + crossed23);
  // tapping a barrier segment: a ranged unit ordered onto one must
  // WALK to it (move order) — an attack order would stop it at
  // range (~110u out) and leave it shooting a wall it cannot hurt
  await call('dbg_waraddbits', 1000);
  ok((await call('dbg_warbuildat', 1)) > 0, 'v1.18 check 23: Patch Bay built');
  ok((await call('dbg_wartrain', 2)) === 1, 'v1.18 check 23: a bolt buddy trained');
  let segFar = null;   // the line segment farthest from the passage
  for (const s of segs23) if (Math.abs(s[1] - lineX23) < 6 && (!segFar || Math.abs(s[2] - gapY23) > Math.abs(segFar[2] - gapY23))) segFar = s;
  const bolt23 = await call('dbg_warfind', 2);
  await call('dbg_warmove', bolt23, lineX23 - 170, segFar[2]);
  ok((await call('dbg_wartapunit', bolt23)) === 1, 'v1.18 check 23: bolt buddy selected');
  await call('dbg_warcam', segFar[1], segFar[2]); await sleep(250);
  const segPos = unpos(await call('dbg_warfeatpos', segFar[0]));
  await tap(segPos[0], segPos[1]);   // tap the barrier segment itself
  const walked23 = await poll('dbg_warunitxy', [bolt23], v => {
    if (v < 0) return false;
    const [ux, uy] = unxy(v);
    return Math.hypot(ux - segFar[1], uy - segFar[2]) < 45;
  }, 40000, 250);
  ok(walked23 >= 0, 'v1.18 check 23: the bolt buddy WALKED to the tapped barrier (a move order, not an attack)',
     'xy=' + walked23 + ' segment at ' + segFar[1] + ',' + segFar[2]);
  const segHp = await call('dbg_warfeat', segFar[0]);
  ok(Math.floor(segHp / 100) === 99999 && (await call('dbg_warbarriers')) === barN,
     'v1.18 check 23: the barrier is untouched — indestructible', 'hp=' + Math.floor(segHp / 100));

  // check 24: barriers ride the snapshot — park this battle, leave
  // the cabinet, re-enter: the same wall stands
  const barBefore = await call('dbg_warbarriers'), tnBefore = await call('dbg_warterrain');
  await frozenReadings();
  await saveQuitToMap();
  ok((await call('dbg_warsnap')) === 1, 'v1.18 check 24: battle with barriers parked');
  await key('Escape', 300);
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'v1.18 check 24: left WAR!');
  await call('dbg_bwentm', 8); await sleep(500);
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1, 'v1.18 check 24: battle restored');
  ok((await call('dbg_warbarriers')) === barBefore && (await call('dbg_warterrain')) === tnBefore,
     'v1.18 check 24: the barrier count survived the snapshot round trip',
     (await call('dbg_warbarriers')) + '/' + (await call('dbg_warterrain')) + ' vs ' + barBefore + '/' + tnBefore);

  // ================= v1.19 "BOOT CAMP" (Nov 17) ==================
  // checks 25-28: SKIRMISH. (The richer campaign field rode check 1
  // — 300 bits, 11 nodes / 8,200 — check 2's 30-bit carry, and
  // check 29's conquest XP above.) Setup-panel geometry in the
  // 640x360 view: panel px=122, py=58, rows at y 116/154/192/230
  // (h 32) — centres 132 (MAP SIZE), 170 (ENEMY), 208 (LAUNCH),
  // 246 (BACK); a value row's left third (x < 254) steps back. The
  // map's SKIRMISH button is (500..628, 292..320), centre (564,306).

  // check 25: a fresh campaign battle carries the v1.19 field and
  // is NOT a skirmish; winning it returns to the map
  ok((await call('dbg_warenter', 0)) === 1, 'v1.19 check 25: fresh campaign battle');
  await sleep(500);
  ok(nodesInBand(await call('dbg_warnodes'), 11, 8200), 'v1.19 check 25: eleven nodes, total inside the v1.27 ±20% jitter band of the nominal 8,200', await call('dbg_warnodes'));
  ok((await call('dbg_warskirmish')) === 0, 'v1.19 check 25: a campaign battle is not a skirmish');
  await call('dbg_warhurt', 1, 1, 99999);
  ok((await poll('dbg_warstate', [], v => v === 0, 9000)) === 0, 'v1.19 check 25: won and back on the map');

  // check 26: the setup panel — SKIRMISH button, SMALL + EASY, LAUNCH
  await sleep(500);   // let the post-victory map grace lapse
  await tap(568, 307);   // the SKIRMISH button, bottom-right of the map
  ok((await poll('dbg_warstate', [], v => v === 4, 4000)) === 4, 'v1.19 check 26: the SKIRMISH button opened the setup panel');
  await cv.screenshot({ path: __dirname + '/war-skirmish-setup.png' });
  await tap(170, 132);   // MAP SIZE row, left third: STANDARD -> SMALL
  await tap(170, 170);   // ENEMY row, left third: NORMAL -> EASY
  await tap(320, 208);   // LAUNCH
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1, 'v1.19 check 26: LAUNCH started the battle');
  ok((await call('dbg_warskirmish')) === 1, 'v1.19 check 26: the battle is a skirmish');
  ok(nodesInBand(await call('dbg_warnodes'), 6, 4550), 'v1.19 check 26: SMALL field — six nodes (standard-table indices {0,1,4,5,6,10}), total inside the v1.27 ±20% jitter band of the nominal 4,550', await call('dbg_warnodes'));
  ok((await call('dbg_warhives')) === 1, 'v1.19 check 26: EASY fields exactly one hive');
  ok((await call('dbg_warbits')) === 250, 'v1.19 check 26: SMALL starts with 250 bits', await call('dbg_warbits'));
  await cv.screenshot({ path: __dirname + '/war-skirmish-small.png' });

  // check 27: a skirmish win pays EASY XP and moves NO campaign state
  const xp27 = await call('dbg_buddyxp'), prog27 = await call('dbg_warconq');
  await call('dbg_warhurt', 1, 1, 99999);
  ok((await poll('dbg_warstate', [], v => v === 2, 5000)) === 2, 'v1.19 check 27: skirmish won');
  ok((await call('dbg_buddyxp')) === xp27 + 30, 'v1.19 check 27: the win paid +30 buddy XP (EASY)', 'xp ' + xp27 + ' -> ' + (await call('dbg_buddyxp')));
  ok((await poll('dbg_warstate', [], v => v === 0, 8000)) === 0, 'v1.19 check 27: back on the map');
  ok((await call('dbg_warconq')) === prog27, 'v1.19 check 27: campaign progress did not move', prog27 + ' -> ' + (await call('dbg_warconq')));

  // check 28a: QUIT BATTLE — the skirmish pause menu's third row
  // leaves without parking anything (the only battles since the
  // last snapshot clear were skirmishes, which never serialize)
  await sleep(500);
  await tap(568, 307);
  ok((await poll('dbg_warstate', [], v => v === 4, 4000)) === 4, 'v1.19 check 28: setup panel again (picks kept: SMALL / EASY)');
  await tap(320, 208);   // LAUNCH
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1 && (await call('dbg_warskirmish')) === 1, 'v1.19 check 28: skirmish relaunched');
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'v1.19 check 28: pause menu up in the skirmish');
  await tap(320, 234);   // QUIT BATTLE (the skirmish's third row)
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'v1.19 check 28: QUIT BATTLE returned to the map');
  ok((await call('dbg_warsnap')) === 0, 'v1.19 check 28: nothing was parked by quitting');

  // check 28b: a skirmish never evicts a campaign park — park a
  // region-2 battle, run a skirmish beside it, quit the skirmish,
  // and the park must still restore exactly
  ok((await call('dbg_warenter', 1)) === 1, 'v1.19 check 28: region-2 campaign battle for the park');
  await sleep(500);
  const r28 = await frozenReadings();
  await saveQuitToMap();
  ok((await call('dbg_warsnap')) === 1, 'v1.19 check 28: campaign battle parked');
  await tap(568, 307);
  ok((await poll('dbg_warstate', [], v => v === 4, 4000)) === 4, 'v1.19 check 28: setup panel over the map (park in place)');
  await tap(320, 208);   // LAUNCH
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1 && (await call('dbg_warskirmish')) === 1, 'v1.19 check 28: skirmish launched beside the park');
  await key('Escape', 300);
  ok((await poll('dbg_warpause', [], v => v === 1, 4000)) === 1, 'v1.19 check 28: pause menu up');
  await tap(320, 234);   // QUIT BATTLE
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'v1.19 check 28: skirmish quit to the map');
  ok((await call('dbg_warsnap')) === 1, 'v1.19 check 28: the campaign park SURVIVED the skirmish');
  await sleep(600);   // QUIT BATTLE arms the same 0.3s map grace as SAVE & QUIT — let it lapse before tapping a node (the saveQuitToMap lesson)
  const [n28x, n28y] = nodeXY(1);
  await tap(n28x, n28y);   // region 2's node: resumes the park
  const inst28 = await pollInstant();
  ok(inst28[0] === 1 && inst28[1] === 1 && inst28[2] === 0, 'v1.19 check 28: the parked battle restored (consumed)', 'state/region/snap=' + inst28.join('/'));
  const r28b = await frozenReadings();
  ok(sameReadings(r28b, r28), 'v1.19 check 28: bits, army and nodes match the park exactly', readingsStr(r28b) + ' vs parked ' + readingsStr(r28));
  await tap(320, 146);   // RESUME

  // check 30: the PATCHER (v1.19 addendum) — repair infantry from
  // the Patch Bay: trained through the real bar, repairs on a tap
  // order, AUTOSEEKS when idle (Daniel's amendment), yields to a
  // manual move order, and never harms a bug.
  ok((await call('dbg_warenter', 0)) === 1, 'v1.19 check 30: fresh battle for the patcher');
  await sleep(500);
  await call('dbg_waraddbits', 1000);
  const pb30 = await call('dbg_warbuildat', 1);
  ok(pb30 > 0, 'v1.19 check 30: Patch Bay built', 'idx=' + pb30);
  const [pbx30, pby30] = unxy(await call('dbg_warstructxy', pb30));
  await call('dbg_warcam', pbx30, pby30); await sleep(250);
  await tap(320, 157);   // select the Patch Bay — its lone TRAIN category opens itself
  ok((await poll('dbg_warselstruct', [], v => v === pb30, 4000)) === pb30, 'v1.19 check 30: Patch Bay selected');
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.19 check 30: TRAIN strip open');
  const bits30 = await call('dbg_warbits');
  await tap(228, 291);   // strip slot 2: PATCHER
  ok((await poll('dbg_warqueue', [pb30], v => v === 1, 4000)) === 1, 'v1.19 check 30: patcher queued at the Patch Bay');
  ok((await call('dbg_warbits')) === bits30 - 45, 'v1.19 check 30: the queue charged 45 bits up front', bits30 + ' -> ' + (await call('dbg_warbits')));
  ok((await poll('dbg_warunits', [0, 7], v => v === 1, 15000)) === 1, 'v1.19 check 30: a patcher rolled out');
  const patcher30 = await call('dbg_warfind', 7);
  ok(patcher30 >= 0, 'v1.19 check 30: patcher found on the field');
  await cv.screenshot({ path: __dirname + '/war-patcher.png' });
  // isolate the purse for the repair bookkeeping below (the
  // check-3/15 idiom): a dead harvester delivers nothing, and the
  // purse stays far above the emergency-grant line throughout
  await call('dbg_warhurt', 0, await call('dbg_warfind', 0), 99999);
  ok((await call('dbg_warunits', 0, 0)) === 0, 'v1.19 check 30: harvester down (economy isolated)');
  // never fights: stage the patcher in a quiet corner, spawn a
  // crawler 160u off, order the patcher ONTO it — the tap must
  // land as a plain move (the barrier precedent), and the crawler
  // must still be whole after standing next to the patcher
  await call('dbg_warmove', patcher30, 750, 200);
  ok((await poll('dbg_warunitxy', [patcher30], v => { if (v < 0) return false; const [ux, uy] = unxy(v); return Math.hypot(ux - 750, uy - 200) < 40; }, 20000, 250)) >= 0,
     'v1.19 check 30: patcher walked to the staging corner');
  await call('dbg_warspawnbug', 910, 200);
  const bugs30 = await call('dbg_warunits', 1, -1);
  ok((await call('dbg_wartapunit', patcher30)) === 1, 'v1.19 check 30: patcher selected');
  await call('dbg_warcam', 910, 200); await sleep(250);
  await tap(320, 157);   // tap the crawler itself
  ok((await poll('dbg_warunitxy', [patcher30], v => { if (v < 0) return false; const [ux, uy] = unxy(v); return Math.hypot(ux - 910, uy - 200) < 60; }, 20000, 250)) >= 0,
     'v1.19 check 30: the patcher WALKED to the tapped crawler (a move order, not an attack)');
  await sleep(2500);   // standing next to it, chewing range — a fighter would have bitten by now
  ok((await call('dbg_warunits', 1, -1)) === bugs30 && (await call('dbg_warunits', 0, 7)) === 1,
     'v1.19 check 30: the crawler is unharmed and the patcher still stands — it has no weapon',
     'bugs=' + (await call('dbg_warunits', 1, -1)) + ' vs ' + bugs30);
  // ordered repair: hurt the Patch Bay, select the patcher, tap
  // the building — HP must rise while the purse falls, and both
  // must stop at full
  const pbFull30 = await call('dbg_warstructhp', pb30);
  await call('dbg_warhurt', 1, pb30, 120);
  const pbHurt30 = await call('dbg_warstructhp', pb30);
  ok(pbHurt30 === pbFull30 - 120, 'v1.19 check 30: Patch Bay damaged for the repair', pbFull30 + ' -> ' + pbHurt30);
  ok((await call('dbg_wartapunit', patcher30)) === 1, 'v1.19 check 30: patcher selected for the order');
  await call('dbg_warcam', pbx30, pby30); await sleep(250);
  const bitsOrd30 = await call('dbg_warbits');
  await tap(320, 157);   // tap the damaged Patch Bay: a repair order
  ok((await poll('dbg_warstructhp', [pb30], v => v >= pbHurt30 + 36, 30000, 250)) >= pbHurt30 + 36,
     'v1.19 check 30: the ordered repair is flowing (HP rising)', 'hp=' + (await call('dbg_warstructhp', pb30)));
  ok((await call('dbg_warbits')) < bitsOrd30, 'v1.19 check 30: ...and the purse is paying for it', bitsOrd30 + ' -> ' + (await call('dbg_warbits')));
  ok((await poll('dbg_warstructhp', [pb30], v => v >= pbFull30, 30000, 250)) >= pbFull30,
     'v1.19 check 30: repaired back to full', 'hp=' + (await call('dbg_warstructhp', pb30)));
  const bitsFull30 = await call('dbg_warbits'); await sleep(1500);
  ok((await call('dbg_warbits')) === bitsFull30, 'v1.19 check 30: at full HP the billing stops', bitsFull30 + ' -> ' + (await call('dbg_warbits')));
  // autoseek: hurt the Core and give NO order at all — the idle
  // patcher must find the work itself
  const coreFull30 = await call('dbg_warstructhp', 0);
  await call('dbg_warhurt', 1, 0, 60);
  ok((await poll('dbg_warstructhp', [0], v => v >= coreFull30 - 36, 30000, 250)) >= coreFull30 - 36,
     'v1.19 check 30: AUTOSEEK — the idle patcher repaired the Core unbidden', 'hp=' + (await call('dbg_warstructhp', 0)));
  // ...and a manual move order interrupts it: hurt the Core again,
  // let the patcher start, then order it away — the target is a
  // scanned clearing >200u from the Core and >=48u clear of every
  // terrain feature. (The target used to be the (430,430) node's
  // clearing, guaranteed open because terrain kept its distance
  // from nodes; v1.27's node jitter moved the node, not the
  // guarantee's centre, so the clearing is scanned for instead —
  // the (430,430) candidate still leads the list. An arbitrary far
  // point can sit inside a debris disc, where separation would
  // hold the unit off its arrival threshold forever and autoseek
  // would never resume.)
  await call('dbg_warhurt', 1, 0, 60);
  await sleep(1500);   // autoseek has it walking / starting the channel
  ok((await call('dbg_wartapunit', patcher30)) === 1, 'v1.19 check 30: patcher selected for the interrupt');
  const tn30 = await call('dbg_warterrain');
  const feats30 = [];
  for (let t = 0; t < tn30; t++) { const v = await call('dbg_warfeatxy', t); if (v >= 0) feats30.push(unxy(v)); }
  let away30 = null;
  for (const [cx, cy] of [[430, 430], [500, 380], [380, 380], [560, 430], [430, 300], [620, 380], [500, 500], [350, 500]]) {
    if (Math.hypot(cx - 280, cy - 600) <= 200) continue;
    if (feats30.some(([fx, fy]) => Math.hypot(fx - cx, fy - cy) < 48)) continue;
    away30 = [cx, cy]; break;
  }
  ok(!!away30, 'v1.19 check 30: a clear away-point exists for the interrupt');
  await call('dbg_warcam', away30[0], away30[1]); await sleep(250);
  await tap(320, 157);   // the scanned clearing, >200u from the Core: a move order
  ok((await poll('dbg_warunitxy', [patcher30], v => { if (v < 0) return false; const [ux, uy] = unxy(v); return Math.hypot(ux - 280, uy - 600) > 200; }, 20000, 250)) >= 0,
     'v1.19 check 30: the move order pulled the patcher off the repair');
  ok((await call('dbg_warstructhp', 0)) < coreFull30 - 10, 'v1.19 check 30: the Core is still hurt — the repair really stopped',
     'hp=' + (await call('dbg_warstructhp', 0)));
  ok((await poll('dbg_warstructhp', [0], v => v >= coreFull30, 60000, 500)) >= coreFull30,
     'v1.19 check 30: ...and once the move completed, autoseek brought it back to finish the job', 'hp=' + (await call('dbg_warstructhp', 0)));

  // check 31: tap-toggle (v1.19 addendum C) — tapping the selected
  // building deselects it; tapping a different one moves the
  // selection instead
  ok((await call('dbg_warenter', 0)) === 1, 'v1.19 check 31: fresh battle for the toggle');
  await sleep(500);
  await call('dbg_waraddbits', 500);
  const pb31 = await call('dbg_warbuildat', 1);
  ok(pb31 > 0, 'v1.19 check 31: Patch Bay built', 'idx=' + pb31);
  const [pbx31, pby31] = unxy(await call('dbg_warstructxy', pb31));
  await call('dbg_warcam', pbx31, pby31); await sleep(250);
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === pb31, 4000)) === pb31, 'v1.19 check 31: Patch Bay selected');
  await tap(320, 157);   // the same building again
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'v1.19 check 31: tapping the selected building toggled it OFF');
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);   // the Core — a different building
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'v1.19 check 31: tapping the Core selected it');
  await call('dbg_warcam', pbx31, pby31); await sleep(250);
  await tap(320, 157);   // back to the Patch Bay
  ok((await poll('dbg_warselstruct', [], v => v === pb31, 4000)) === pb31, 'v1.19 check 31: tapping another building MOVED the selection (not a clear)');
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'v1.19 check 31: and tapping it once more toggles it off');

  // ================= v1.20 (Nov 17): the bar CAPTURES the pad =====
  // Daniel's design — he explicitly rejected flying the cursor down
  // to the bar. When a building becomes selected through the pad's
  // own A-tap, the command bar captures pad control: the highlight
  // lands on the first option of the current row, the field cursor
  // freezes and hides, the stick/bumpers walk the menu (the stick
  // no longer retires the highlight), A activates through
  // war_bar_activate, a TRAIN activation keeps capture, and B
  // cancels out of the selection entirely. dbg_warbarcap reads the
  // contract back: 0 = no capture, 1 = captured on the options row,
  // 2 = captured on the category row. Pad-only and deterministic:
  // menu steps are stick EDGES and button edges, never positions.
  ok((await call('dbg_warenter', 0)) === 1, 'v1.20 check 32: fresh battle for the bar-capture flow');
  await sleep(500);
  await call('dbg_waraddbits', 1000);
  const pb32 = await call('dbg_warbuildat', 1);
  ok(pb32 > 0, 'v1.20 check 32: Patch Bay built', 'idx=' + pb32);
  const [pbx32, pby32] = unxy(await call('dbg_warstructxy', pb32));
  await call('dbg_warhurt', 0, await call('dbg_warfind', 0), 99999);   // economy isolation (the check-30 idiom): a dead harvester delivers nothing, the purse is static
  // select the Patch Bay with the pad alone (the v1.16 wake trick:
  // A summons the cursor at the camera centre, A again presses it)
  if ((await call('dbg_warcur')) >= 0) { await tap(630, 10); await poll('dbg_warcur', [], v => v === -1, 4000); }
  await call('dbg_warcam', pbx32, pby32); await sleep(250);
  await padTap(0);
  ok((await poll('dbg_warcur', [], v => v >= 0, 4000)) >= 0, 'v1.20 check 32: A woke the cursor at the Patch Bay');
  await padTap(0);
  ok((await poll('dbg_warselstruct', [], v => v === pb32, 4000)) === pb32, 'v1.20 check 32: A at the camera centre selected the Patch Bay');
  ok((await poll('dbg_warbarcap', [], v => v === 1, 4000)) === 1, 'v1.20 check 32: selecting it CAPTURED the bar — highlight on the options row');
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'v1.20 check 32: the highlight landed on the first option');
  ok((await poll('dbg_warbcat', [], v => v === 0, 4000)) === 0, 'v1.20 check 32: its lone TRAIN category auto-opened');
  // while captured the field cursor is FROZEN and the stick walks
  // the menu instead — and does NOT retire the highlight (the
  // clearing rule Daniel rejected)
  const frozen32 = await call('dbg_warcur');
  await padAxes(1, 0, 0, 0); await sleep(250); await padAxes(0, 0, 0, 0); await sleep(200);
  ok((await poll('dbg_warbarhl', [], v => v === 1, 4000)) === 1, 'v1.20 check 32: stick-right walked the highlight to BOLT (slot 1)');
  ok((await call('dbg_warcur')) === frozen32, 'v1.20 check 32: the field cursor did not move while captured');
  await padAxes(1, 0, 0, 0); await sleep(250); await padAxes(0, 0, 0, 0); await sleep(200);
  ok((await poll('dbg_warbarhl', [], v => v === 2, 4000)) === 2, 'v1.20 check 32: stick-right again walked it to PATCHER (slot 2)');
  await padTap(4);   // LB walks back
  ok((await poll('dbg_warbarhl', [], v => v === 1, 4000)) === 1, 'v1.20 check 32: LB walked the highlight back to BOLT');
  await padTap(5);   // RB walks forward again — the bumpers share the captured row
  ok((await poll('dbg_warbarhl', [], v => v === 2, 4000)) === 2, 'v1.20 check 32: RB walked it forward to PATCHER again');
  await cv.screenshot({ path: __dirname + '/war-padcap.png' });
  // A on the PATCHER slot queues it through war_bar_activate, and
  // a TRAIN activation KEEPS the capture
  const bits32 = await call('dbg_warbits');
  await padTap(0);
  ok((await poll('dbg_warqueue', [pb32], v => v === 1, 4000)) === 1, 'v1.20 check 32: A on PATCHER queued a patcher at the Patch Bay');
  ok((await call('dbg_warbits')) === bits32 - 45, 'v1.20 check 32: the queue charged 45 bits', bits32 + ' -> ' + (await call('dbg_warbits')));
  ok((await call('dbg_warbarcap')) === 1, 'v1.20 check 32: the TRAIN activation KEPT capture');
  // stick-down hops to the category row (landing on the open
  // category); A there returns to its options row
  await padAxes(0, 1, 0, 0); await sleep(250); await padAxes(0, 0, 0, 0); await sleep(200);
  ok((await poll('dbg_warbarcap', [], v => v === 2, 4000)) === 2, 'v1.20 check 32: stick-down hopped the highlight to the category row');
  ok((await call('dbg_warbarhl')) === 0, 'v1.20 check 32: ...landing on the open category (slot 0)');
  await padTap(0);
  ok((await poll('dbg_warbarcap', [], v => v === 1, 4000)) === 1, 'v1.20 check 32: A on the category returned to its options row');
  ok((await call('dbg_warbcat')) === 0, 'v1.20 check 32: TRAIN is still the open category');
  // B on a single-category building has no category peel to make:
  // it exits capture AND clears the selection — Daniel's "cancel
  // to get out of the selection"
  await padTap(1);
  ok((await poll('dbg_warbarcap', [], v => v === 0, 4000)) === 0, 'v1.20 check 32: B ended the capture');
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'v1.20 check 32: ...and cleared the selection');
  // control is back with the field cursor: the stick flies it again
  const cur32a = await call('dbg_warcur');
  await padAxes(1, 0, 0, 0); await sleep(400); await padAxes(0, 0, 0, 0); await sleep(200);
  const cur32b = await call('dbg_warcur');
  ok(cur32a >= 0 && cur32b > cur32a, 'v1.20 check 32: the stick flies the field cursor again after capture', cur32a + ' -> ' + cur32b);
  await padAxes(0, 0, 0, 0);

  // ================= v1.21 (Nov 17): the hive has an ECONOMY =====
  // Daniel: "make it so that the economics are relatively balanced
  // between the player and the hive" — decided by skill and
  // strategy, not a structural edge. Check 33 soaks the economy.
  // Battle A is left completely alone: the purse must RISE while
  // the hive's two bug harvesters work the same nodes yours does
  // (a), at a total rate at least matching your single harvester.
  // Battle B starves the hive: both bug harvesters die at once,
  // and with the purse frozen the wave ticks become exact
  // arithmetic — replacements first, then the wave (b) — and once
  // the purse drops under a crawler's price the waves shrink and
  // stop dead (c).
  ok((await call('dbg_warenter', 0)) === 1, 'v1.21 check 33: fresh battle for the economy soak');
  await sleep(500);
  ok((await call('dbg_warhbits')) === 150, 'v1.21 check 33: the hive opens with a 150-bit purse', await call('dbg_warhbits'));
  ok((await call('dbg_warunits', 1, 8)) === 2, 'v1.21 check 33: two bug harvesters work for the hive');
  const tA33 = Date.now();
  ok((await poll('dbg_warhbits', [], v => v > 150, 60000, 500)) > 150,
     'v1.21 check 33 (a): the hive purse RISES while its harvesters work', 'stuck at ' + (await call('dbg_warhbits')));
  while (Date.now() - tA33 < 38000) await sleep(500);   // still ahead of the 45s first wave: everything read here is pure income
  const hiveEarned33 = (await call('dbg_warhbits')) - 150;
  const playerEarned33 = (await call('dbg_warbits')) - 300;
  ok(hiveEarned33 >= playerEarned33 && playerEarned33 > 0,
     'v1.21 check 33: balance — the hive\'s two workers together earn at least what your one harvester does (no structural edge either way)',
     'hive earned ' + hiveEarned33 + ' (2 workers), player earned ' + playerEarned33 + ' (1 worker)');
  // battle B: starvation
  ok((await call('dbg_warenter', 0)) === 1, 'v1.21 check 33: fresh battle for the starvation run');
  await sleep(500);
  for (let k = 0; k < 2; k++) { const bi = await call('dbg_warfind', 8); if (bi >= 0) await call('dbg_warhurt', 0, bi, 99999); }
  await call('dbg_warhurt', 0, await call('dbg_warfind', 0), 99999);   // your harvester too: no income on either side, both purses frozen
  ok((await call('dbg_warunits', 1, 8)) === 0, 'v1.21 check 33: both bug harvesters are dead');
  // v1.27: the hive's garrison is rolled per battle now (2-3 guard
  // crawlers, war_build_battle) — count it live, before the 45s tick
  // buys anything, and hold the wave to exactly +2 against it.
  const garrison33 = await call('dbg_warunits', 1, 3);
  await sleep(19000);
  ok((await call('dbg_warhbits')) === 150, 'v1.21 check 33: no harvesters, no income — the purse is still exactly 150', await call('dbg_warhbits'));
  ok((await poll('dbg_warhbits', [], v => v === 30, 40000, 250)) === 30,
     'v1.21 check 33 (b): the 45s tick bought replacements first (2 x 40) then the wave (2 x 20): 150 -> 30', 'hbits=' + (await call('dbg_warhbits')));
  ok((await call('dbg_warunits', 1, 8)) === 2, 'v1.21 check 33 (b): the workforce was replaced before the wave was bought');
  ok((await call('dbg_warunits', 1, 3)) === garrison33 + 2, 'v1.21 check 33 (b): the garrison crawlers (counted live) + the two the wave bought', await call('dbg_warunits', 1, 3));
  for (let k = 0; k < 2; k++) { const bi = await call('dbg_warfind', 8); if (bi >= 0) await call('dbg_warhurt', 0, bi, 99999); }
  ok((await call('dbg_warunits', 1, 8)) === 0, 'v1.21 check 33: the replacements are dead too — the hive is starving');
  ok((await poll('dbg_warhbits', [], v => v === 10, 45000, 250)) === 10,
     'v1.21 check 33 (c): at 30 bits the next tick buys NO replacement (40) and a wave of ONE crawler (20): 30 -> 10', 'hbits=' + (await call('dbg_warhbits')));
  ok((await call('dbg_warunits', 1, 8)) === 0, 'v1.21 check 33 (c): still no workforce — 10 bits buys nothing that harvests');
  await sleep(33000);   // past the following tick (105s)
  ok((await call('dbg_warhbits')) === 10, 'v1.21 check 33 (c): and the tick after that buys NOTHING — a starved hive sends no wave', await call('dbg_warhbits'));

  // check 34: DEMOLISH (v1.21 addendum) — a selected building can
  // be sold back for half its build cost, floored, through a
  // two-step arm + confirm in its command-bar options (the v1.20
  // pad capture reaches it too). The Compiler Core is never
  // offered it, and B cancels an armed demolition harmlessly.
  ok((await call('dbg_warenter', 0)) === 1, 'v1.21 check 34: fresh battle for the demolish flow');
  await sleep(500);
  await call('dbg_warhurt', 0, await call('dbg_warfind', 0), 99999);   // economy isolation: the purse moves only when this check moves it
  await call('dbg_waraddbits', 1000);
  const zp34 = await call('dbg_warbuildat', 2);
  ok(zp34 > 0 && (await call('dbg_warstructs', 2)) === 1, 'v1.21 check 34: Zapper built', 'idx=' + zp34);
  const [zx34, zy34] = unxy(await call('dbg_warstructxy', zp34));
  const bits34 = await call('dbg_warbits');
  await call('dbg_warcam', zx34, zy34); await sleep(250);
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === zp34, 4000)) === zp34, 'v1.21 check 34: Zapper selected');
  await tap(48, 337);   // the Zapper has no categories: its whole bar is [DEMOLISH]
  ok((await call('dbg_warstructs', 2)) === 1 && (await call('dbg_warbits')) === bits34,
     'v1.21 check 34: the first DEMOLISH tap only ARMS — building standing, purse untouched');
  await tap(48, 337);   // CONFIRM?
  ok((await poll('dbg_warstructs', [2], v => v === 0, 4000)) === 0, 'v1.21 check 34: the second tap DEMOLISHES');
  ok((await call('dbg_warbits')) === bits34 + 50, 'v1.21 check 34: the refund is exactly half the 100-bit build cost: +50', bits34 + ' -> ' + (await call('dbg_warbits')));
  const zp34b = await call('dbg_warbuildxy', 2, zx34, zy34);
  ok(zp34b >= 0, 'v1.21 check 34: the footprint is free — a new Zapper builds on the same spot', 'idx=' + zp34b);
  const bits34b = await call('dbg_warbits');
  // the Core shows NO demolish option: with TRAIN open, strip
  // slot 1 (DEMOLISH's seat in a Patch Bay's row) is empty air,
  // and so is bar-row slot 3 past its three categories — poking
  // each twice (arm + confirm, if anything hid there) is a no-op
  await call('dbg_warcam', 280, 600); await sleep(250);
  await tap(320, 157);
  ok((await poll('dbg_warselstruct', [], v => v === 0, 4000)) === 0, 'v1.21 check 34: Core selected');
  await tap(48, 337);   // TRAIN category
  await tap(138, 291); await tap(138, 291);   // strip slot 1, twice
  await tap(318, 337); await tap(318, 337);   // bar-row slot 3, twice
  ok((await call('dbg_warstructs', 0)) === 1 && (await call('dbg_warbits')) === bits34b && (await call('dbg_warstate')) === 1,
     'v1.21 check 34: the Core has no DEMOLISH — it stands, the purse is untouched, the battle goes on');
  // arming then cancelling with B leaves the building standing:
  // the rebuilt Zapper, driven through the pad capture
  if ((await call('dbg_warcur')) >= 0) { await tap(630, 10); await poll('dbg_warcur', [], v => v === -1, 4000); }
  await call('dbg_warcam', zx34, zy34); await sleep(250);
  await padTap(0);
  ok((await poll('dbg_warcur', [], v => v >= 0, 4000)) >= 0, 'v1.21 check 34: A woke the pad cursor at the Zapper');
  // v1.22 fix: the cursor wakes AT the camera centre — no stick
  // nudge. (Before the fix it woke WAR_BAR_H/2 below centre, 23u
  // from a centred Zapper: a hair outside its 22u tap disc, and this
  // section had to nudge the stick up onto the building first.)
  { const [cx34, cy34] = unxy(await call('dbg_warcur'));
    ok(Math.abs(cx34 - zx34) <= 4 && Math.abs(cy34 - zy34) <= 4,
       'v1.22 check 34: the woken cursor sits ON the centred Zapper (no nudge)', cx34 + ',' + cy34 + ' vs ' + zx34 + ',' + zy34); }
  await padTap(0);
  ok((await poll('dbg_warselstruct', [], v => v === zp34b, 4000)) === zp34b, 'v1.21 check 34: A selected the Zapper');
  ok((await poll('dbg_warbarcap', [], v => v === 1, 4000)) === 1, 'v1.21 check 34: the [DEMOLISH] bar CAPTURED the pad (the v1.20 capture, a new menu)');
  ok((await poll('dbg_warbarhl', [], v => v === 0, 4000)) === 0, 'v1.21 check 34: the highlight sits on DEMOLISH');
  const bits34c = await call('dbg_warbits');
  await padTap(0);   // A on DEMOLISH: arm only
  ok((await call('dbg_warstructs', 2)) === 1 && (await call('dbg_warbits')) === bits34c,
     'v1.21 check 34: A armed DEMOLISH — the Zapper still stands');
  await padTap(1);   // B: cancel out of the selection — and disarm
  ok((await poll('dbg_warbarcap', [], v => v === 0, 4000)) === 0, 'v1.21 check 34: B ended the capture');
  ok((await poll('dbg_warselstruct', [], v => v === -1, 4000)) === -1, 'v1.21 check 34: B cleared the selection');
  ok((await call('dbg_warstructs', 2)) === 1 && (await call('dbg_warbits')) === bits34c,
     'v1.21 check 34: arming then cancelling with B leaves the building standing, purse untouched');

  // ================= v1.29 "ON THE BUTTON" (Nov 23) ==================
  // checks 35-37: SAVE YOUR SKIRMISH (Daniel). A skirmish in
  // progress parks in its OWN slot (kb..kh), beside — never instead
  // of — the campaign park; it survives a full reload and resumes
  // from the entry panel; NEW SKIRMISH is a two-step discard. The
  // mid-battle state is realistic: bits spent on a building, a unit
  // roster on the field, the hive wounded, the fight underway.
  // (After the reload, wartest's T handle is stale — the call2/poll2
  // pair below re-reads window.__TSG fresh, the mordortest way.)
  const call2 = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll2 = async (fn, pred, ms = 10000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call2(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const structHPs = async c => { const a = []; for (let i = 0; i < 28; i++) a.push(await c('dbg_warstructhp', i)); return a; };
  const frozen2 = async () => {   // frozenReadings, post-reload edition
    await key('Escape', 300);
    await poll2('dbg_warpause', v => v === 1, 4000);
    return { bits: await call2('dbg_warbits'), units: await call2('dbg_warunits', 0, -1), nodes: await call2('dbg_warnodes') };
  };

  // check 35: build the mid-skirmish state and SAVE & QUIT it
  await frozenReadings();   // park check 34's campaign battle first (its own slot — the skirmish must not disturb it)
  await saveQuitToMap();
  ok((await call('dbg_warsnap')) === 1, 'v1.29 check 35: the campaign battle is parked before the skirmish');
  await tap(568, 307);   // SKIRMISH button
  ok((await poll('dbg_warstate', [], v => v === 4, 4000)) === 4, 'v1.29 check 35: setup panel for the saveable skirmish');
  await tap(400, 132);   // MAP SIZE forward: SMALL -> STANDARD
  await tap(400, 170);   // ENEMY forward: EASY -> NORMAL
  await tap(320, 208);   // LAUNCH
  ok((await poll('dbg_warstate', [], v => v === 1, 5000)) === 1 && (await call('dbg_warskirmish')) === 1, 'v1.29 check 35: STANDARD/NORMAL skirmish launched');
  await call('dbg_waraddbits', 600);
  const pb35 = await call('dbg_warbuildat', 1);
  ok(pb35 > 0, 'v1.29 check 35: a Patch Bay was built (bits spent)', 'idx=' + pb35);
  const skHive0 = await call('dbg_warstructhp', 1);
  await call('dbg_warhurt', 1, 1, 40);
  const skHive1 = await call('dbg_warstructhp', 1);
  ok(skHive0 > 0 && skHive1 < skHive0, 'v1.29 check 35: the hive is wounded (the fight is underway)', skHive0 + ' -> ' + skHive1);
  await sleep(2500);   // let the battle breathe: waves, movement, harvesting
  const r35 = await frozenReadings();
  const hp35 = await structHPs(call);
  await saveQuitToMap();   // the skirmish's third pause row is SAVE & QUIT now
  ok((await poll('dbg_warstate', [], v => v === 0, 4000)) === 0, 'v1.29 check 35: SAVE & QUIT returned to the map');
  ok((await call('dbg_warsksnap')) === 1, 'v1.29 check 35: the skirmish is parked in its own slot');
  ok((await call('dbg_warsnap')) === 1, 'v1.29 check 35: the campaign park SURVIVED the skirmish save');

  // check 36: the skirmish park survives a full reload and resumes
  // from the entry panel, state intact. The campaign park is
  // consumed first (war_enter gives it priority by design), the
  // cabinet is left, and the file settles — so the reload's entry
  // is the skirmish panel's stage, exactly as a player's would be.
  const [skN0x, skN0y] = nodeXY(0);
  await tap(skN0x, skN0y);   // region 1's node: resumes (consumes) the campaign park
  ok((await poll('dbg_warstate', [], v => v === 1, 6000)) === 1, 'v1.29 check 36: the campaign park resumed (consumed) on the way');
  await call('dbg_warhurt', 1, 1, 99999);
  ok((await poll('dbg_warstate', [], v => v === 0, 9000)) === 0, 'v1.29 check 36: the replayed campaign battle is won and done');
  await sleep(500);
  await key('Escape', 300);   // leave the cabinet from the map
  ok((await poll('dbg_bwmode', [], v => v === -1, 4000)) === -1, 'v1.29 check 36: out of WAR! before the reload');
  await sleep(6000);   // an autosave writes the settled file (skirmish park in, campaign park out)
  const blob36 = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  ok(blob36.includes(' kb=') && !blob36.includes(' wb='), 'v1.29 check 36: the save file carries the skirmish park (kb=) and no campaign park', blob36.slice(0, 60));
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  await key('Enter', 400);
  ok((await poll2('dbg_state', v => v === 1, 8000)) === 1, 'v1.29 check 36: the run reloads from the slot');
  await call2('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  ok((await poll2('dbg_mall', v => v === 1)) === 1, 'v1.29 check 36: in the mall after the reload');
  await call2('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  ok((await poll2('dbg_inarcade', v => v === 1)) === 1, 'v1.29 check 36: in the arcade after the reload');
  await call2('dbg_tp', 0, -11758); await sleep(200);
  ok((await call2('dbg_cabinet')) === 6, 'v1.29 check 36: standing at the WAR! cabinet');
  await key('KeyE', 250);
  ok((await poll2('dbg_arcgate', v => v === 1, 3000)) === 1, 'v1.29 check 36: WAR! cabinet panel opened');
  await key('Space', 300);
  ok((await poll2('dbg_bwmode', v => v === 8, 4000)) === 8, 'v1.29 check 36: WAR! launched after the reload');
  ok((await poll2('dbg_warskpanel', v => v === 1, 4000)) === 1, 'v1.29 check 36: the RESUME / NEW SKIRMISH panel is up');
  ok((await call2('dbg_warstate')) === 0, 'v1.29 check 36: the map waits under the panel (nothing auto-resumed)');
  const instantSk = () => page.evaluate(() => [window.__TSG.dbg_warstate(), window.__TSG.dbg_warskirmish(), window.__TSG.dbg_warsksnap()]);
  await key('Space', 300);   // RESUME SKIRMISH (the panel's first row)
  const inst36 = await (async () => { const t0 = Date.now(); let v = await instantSk(); while (v[0] !== 1 && Date.now() - t0 < 6000) { await sleep(120); v = await instantSk(); } return v; })();
  ok(inst36[0] === 1 && inst36[1] === 1 && inst36[2] === 0, 'v1.29 check 36: RESUME restored the skirmish (park consumed at the same instant)', 'state/skirmish/sksnap=' + inst36.join('/'));
  const r36 = await frozen2();
  const hp36 = await structHPs(call2);
  ok(sameReadings(r36, r35), 'v1.29 check 36: bits, army and nodes match the saved skirmish exactly', readingsStr(r36) + ' vs saved ' + readingsStr(r35));
  ok(JSON.stringify(hp36) === JSON.stringify(hp35), 'v1.29 check 36: every structure HP matches (the wounded hive included)',
     'hive ' + hp36[1] + ' vs saved ' + hp35[1]);
  await tap(320, 234);   // SAVE & QUIT — park it again for the discard check (the pause menu is up)
  ok((await poll2('dbg_warstate', v => v === 0, 4000)) === 0 && (await call2('dbg_warsksnap')) === 1, 'v1.29 check 36: the skirmish is parked again');

  // check 37: NEW SKIRMISH — the first pick only arms, the second
  // discards the saved battle and frees the map
  await sleep(600);
  await key('Escape', 300);   // leave the cabinet from the map
  ok((await poll2('dbg_bwmode', v => v === -1, 4000)) === -1, 'v1.29 check 37: out of WAR! again');
  await call2('dbg_tp', 0, -11758); await sleep(200);
  await key('KeyE', 250);
  ok((await poll2('dbg_arcgate', v => v === 1, 3000)) === 1, 'v1.29 check 37: cabinet panel again');
  await key('Space', 300);
  ok((await poll2('dbg_warskpanel', v => v === 1, 4000)) === 1, 'v1.29 check 37: the panel is up again');
  await key('ArrowDown', 200);   // selection to NEW SKIRMISH
  await key('Space', 300);
  ok((await call2('dbg_warskpanel')) === 1 && (await call2('dbg_warsksnap')) === 1, 'v1.29 check 37: the first NEW SKIRMISH pick only arms the confirm');
  await sleep(400);   // the arm's pick lock lapses
  await key('Space', 300);
  ok((await poll2('dbg_warskpanel', v => v === 0, 4000)) === 0, 'v1.29 check 37: the second pick discards — panel closed');
  ok((await call2('dbg_warsksnap')) === 0, 'v1.29 check 37: the saved skirmish is gone');
  ok((await call2('dbg_warstate')) === 0, 'v1.29 check 37: the bare map remains');
  await tap(568, 307);
  ok((await poll2('dbg_warstate', v => v === 4, 4000)) === 4, 'v1.29 check 37: a fresh skirmish can be set up (the discard freed the field)');
  await key('Escape', 300);
  ok((await poll2('dbg_warstate', v => v === 0, 4000)) === 0, 'v1.29 check 37: setup backed out to the map');

  console.log(`\nwartest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
