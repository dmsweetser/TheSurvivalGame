// wartest.js — WAR! (Nov 15): the seventh arcade cabinet, a tap-first
// RTS. World map -> battle for region 1: harvest loop fills the purse,
// the command bar trains through a real tap flow, buildings go up,
// double-tap selects every unit of a type, a zapper tower kills a bug,
// the hive falls, the region is conquered — and the conquest survives
// a save/Continue round trip (the Buddy's wr key).
// dbg packings (see survival.c): dbg_warunits(side,type) counts alive
// units (side 0 buddies / 1 bugs; types 0 harvester, 1 melee, 2 ranged,
// 3 crawler, 4 spitter; -1 = any). dbg_warunitxy / dbg_warstructxy pack
// arena-local coords as x*10000+y; dbg_warunitpos packs on-screen fb
// coords the same way. Struct 0 is always the Compiler Core; the hive
// of a fresh battle is struct 1.
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
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); };
  const hold = async (k, ms = 200) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(150); };
  const tap = async (x, y) => { await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [x, y]); await sleep(250); };
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
