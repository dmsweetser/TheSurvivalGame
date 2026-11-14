// probe-stairs.js — forensic one-off (Nov 6): is the subfloor west
// door's stairwell climb reachable? Enters Entrances, completes the
// Library via dbg hooks where possible, rides the bookcase down, then
// probes the west-door region of B-1 tile by tile and reports where the
// stairwell trigger (ent_sub flips to 0) can be reached from.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 160)));
  await page.evaluate(() => localStorage.clear()).catch(() => {});
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(60); };
  const tp = async (x, y, hold = 160) => { await call('dbg_bwtp', x, y); await sleep(hold); };

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  await call('dbg_bwent2');
  await sleep(400);
  console.log('in hotel, room', await call('dbg_entroom'));

  // to the Library: teleport into room 25, gather the 5 fuses by standing
  // on their tiles (same spots the suite uses)
  const tileCX = (room, lx) => (room - 1) * 15 * 48 + lx * 48 + 24;
  const tileCY = (ly) => ly * 48 + 24;
  await tp(tileCX(25, 7), tileCY(4));
  for (const [lx, ly] of [[2, 1], [12, 1], [7, 4], [2, 8], [12, 8]]) { await tp(tileCX(25, lx), tileCY(ly)); await sleep(180); }
  console.log('fuses', await call('dbg_entfuse'), 'libdone', await call('dbg_entlib'));
  // through the Library door on its own row: find the door row first
  let doorRow = 0;
  for (let ly = 1; ly <= 8; ly++) if ((await call('dbg_enttile', 24 * 15 + 14, ly)) === 0) doorRow = ly;
  console.log('library east wall open row:', doorRow);
  await tp(tileCX(25, 14) + 30, tileCY(doorRow || 4));
  console.log('room now', await call('dbg_entroom'));
  // back into 25 and ride the bookcase
  await tp(tileCX(25, 7), tileCY(4)); await sleep(200);
  await tp(tileCX(25, 12), tileCY(8));
  await key('KeyE', 250); await sleep(400);
  console.log('sub', await call('dbg_entsub'), 'room', await call('dbg_entroom'));
  if ((await call('dbg_entsub')) !== 1) { console.log('PROBE ABORT: never got downstairs'); await browser.close(); return; }

  // what's on B-1's west wall?
  for (let ly = 0; ly <= 9; ly++)
    console.log('B-1 west wall ly', ly, 'tile', await call('dbg_enttile', 0, ly));
  console.log('B-1 door row (east wall scan):');
  for (let ly = 1; ly <= 8; ly++) { const t = await call('dbg_enttile', 14, ly); if (t === 2 || t === 0) console.log('  ly', ly, 'tile', t); }

  // walk east through doors 1 and 2 like the suite does (tp into next room)
  await tp(tileCX(2, 7), tileCY(4)); await sleep(200);
  await tp(tileCX(3, 7), tileCY(4)); await sleep(200);
  console.log('walked to B-', await call('dbg_entroom'), 'sub', await call('dbg_entsub'));

  // walk back WEST through the doors like a player: stand just inside
  // room 3's west doorway, then room 2's, then face B-1's west door
  await tp(2 * 15 * 48 + 30, 216); await sleep(250);
  console.log('after west-door walk 1: room', await call('dbg_entroom'), 'sub', await call('dbg_entsub'));
  await tp(1 * 15 * 48 + 30, 216); await sleep(250);
  console.log('after west-door walk 2: room', await call('dbg_entroom'), 'sub', await call('dbg_entsub'));

  // the climb, WALKED like a player: hold ArrowLeft from mid-room and
  // log the buddy's actual x (dbg_entx) while he slides to the wall
  await tp(200, 216); await sleep(200);
  console.log('start x', await call('dbg_entx'));
  await page.keyboard.down('ArrowLeft');
  for (let i = 0; i < 10; i++) { await sleep(200); const s = await call('dbg_entsub'); console.log('walking west: entx', await call('dbg_entx'), 'sub', s, 'room', await call('dbg_entroom')); if (s === 0) break; }
  await page.keyboard.up('ArrowLeft');
  await key('KeyE', 250); await sleep(400);
  console.log('after walk+E: room', await call('dbg_entroom'), 'sub', await call('dbg_entsub'), 'entx', await call('dbg_entx'));
  console.log('final: sub', await call('dbg_entsub'), 'room', await call('dbg_entroom'));
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
