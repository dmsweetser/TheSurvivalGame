// Walls, played by hand: build one, then REINFORCE it twice by
// building right on top of it. Since Nov 7 the stack is one wall
// ENTRY (the Nov 6 absorb fix for stacking lag) that still counts
// its layers: it renders as a pile, counts as several walls in its
// run (shared pool + tiers), persists through save/Continue, and
// sheds one layer at a time when chewed. Then a fourth wall placed
// slightly off-line snaps into the run beside it.
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
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const hold = async (k, ms = 200) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(150); };
  async function poll(fn, args, want, timeout = 9000, step = 150) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }
  async function faceSouth() {
    await page.keyboard.down('ArrowDown'); await sleep(90); await page.keyboard.up('ArrowDown');
    await sleep(150);
  }

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_feed'); await call('dbg_feed'); await call('dbg_feed'); await call('dbg_feed');
  await call('dbg_tp', 0, 300); await sleep(200);
  await faceSouth();
  await key('KeyF', 200);
  ok((await poll('dbg_walls', [], v => v === 1)) === 1, 'first wall built');
  ok((await call('dbg_wallstack')) === 1, 'fresh wall is a single layer');
  await sleep(600);
  await key('KeyF', 200);   // same spot, no moving: reinforce it
  await sleep(600);
  ok((await call('dbg_walls')) === 1, 'reinforcement keeps ONE wall entry (no twin pile-up)', await call('dbg_walls'));
  ok((await poll('dbg_wallstack', [], v => v === 2)) === 2, 'stack counts its layers: 2');
  ok((await poll('dbg_carried', [], v => v === 280)) === 280, 'reinforcing cost the usual 3 carcasses');
  const hp2 = await call('dbg_wallhp');
  ok(hp2 >= 1100, 'the stack pooled the strength of both walls (maxhp ~116)', hp2);
  ok((await poll('dbg_grpmax', [], v => v === 2)) === 2, 'the stack counts as TWO walls in its run (shared pool)');
  await sleep(600);
  await key('KeyF', 200);   // reinforce again: three deep
  await sleep(600);
  ok((await poll('dbg_wallstack', [], v => v === 3)) === 3, 'second reinforcement: stack of 3');
  ok((await call('dbg_walllayers')) === 3, 'three layers total in the world');

  await call('dbg_tp', 66, 288); await sleep(200);
  await faceSouth();
  await key('KeyF', 200);   // candidate lands off-line and long: must snap
  ok((await poll('dbg_walls', [], v => v === 2)) === 2, 'fourth wall built (second entry)');
  const gm = await poll('dbg_grpmax', [], v => v === 4, 6000);
  ok(gm === 4, 'off-line placement SNAPPED into the run (run of 4: stack 3 + neighbour)', gm);
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/walltest.png' });

  // layer shedding: bugs chewing a reinforced wall crumble ONE layer
  // off — the entry (and the rest of the pile) survives the shed.
  // (Done while the player is legitimately out in the world: after a
  // Continue he respawns flagged in-mall, and bugs won't target him.)
  await call('dbg_tp', 0, 300); await sleep(300);
  await call('dbg_spawnbug', 0, 356);
  await call('dbg_spawnbug', -12, 360);
  await call('dbg_spawnbug', 12, 360);
  const shed = await poll('dbg_wallstack', [], v => v === 2, 90000, 500);
  ok(shed === 2, 'chewed stack SHED one layer (3 -> 2) instead of dying whole', shed);
  ok((await call('dbg_walls')) === 2, 'the shed stack is still standing (entry alive)', await call('dbg_walls'));
  // draw the bugs off south, then rebuild the layer for the save test
  await call('dbg_tp', 0, 520); await sleep(1500);
  await call('dbg_tp', 0, 300); await sleep(200);
  await faceSouth();
  await key('KeyF', 200);
  ok((await poll('dbg_wallstack', [], v => v === 3, 6000)) === 3, 'shed layer rebuilt: stack of 3 again');
  await call('dbg_tp', 0, 520); await sleep(300);   // stand clear of the run

  // persistence: save + quit, Continue — the stack must survive the
  // round trip with its layers and pooled strength intact
  await hold('KeyP', 500);
  await hold('ArrowDown', 250);
  await hold('ArrowDown', 250);        // rows: CONTINUE / CONTROLS / SAVE + QUIT
  await hold('Space', 700);            // SAVE + QUIT TO TITLE
  await hold('Space', 600);            // CONTINUE row
  await hold('Enter', 900);            // slot 1
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  ok((await poll('dbg_walls', [], v => v === 2)) === 2, 'after Continue: both wall entries are back');
  ok((await call('dbg_wallstack')) === 3, 'after Continue: the stack kept its 3 layers');
  ok((await poll('dbg_grpmax', [], v => v === 4)) === 4, 'after Continue: the run still counts 4 walls');

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
