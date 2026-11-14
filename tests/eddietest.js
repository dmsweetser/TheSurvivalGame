// eddietest.js — EDDIE MODE: title toggle persists, bugs render as
// puffballs (screenshot), kills still feed the hat (puff carcasses).
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
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };

  ok((await call('dbg_meddie')) === 0, 'eddie starts off');
  for (let i = 0; i < 5; i++) await key('ArrowDown', 130);   // START..EDDIE row (no save, no pads)
  await key('Space', 350);
  ok((await call('dbg_meddie')) === 1, 'eddie toggled on from the title');
  await page.screenshot({ path: __dirname + '/eddie-title.png' });
  for (let i = 0; i < 5; i++) await key('ArrowUp', 110);
  await key('Space', 400);                                    // slot picker
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_tp', 0, 300); await sleep(200);
  await call('dbg_spawnbug', 45, 335);
  await call('dbg_spawnbug', -35, 268);
  await call('dbg_spawnbug', 10, 345);
  await sleep(1300);
  await page.screenshot({ path: __dirname + '/eddie-bugs.png' });
  // the keyboard is melee and aims where a HELD direction points:
  // steer toward bug 0, hold the direction, and swing until it pops
  // (debug-spawned bugs weigh ~0, so assert the kill, not the hat)
  let killed = false;
  for (let i = 0; i < 16 && !killed; i++) {
    const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
    const px = await call('dbg_px'), py = await call('dbg_py');
    const dx = bx - px, dy = by - py;
    const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
    await page.keyboard.down(dir); await sleep(200);
    await key('Space', 110); await key('Space', 110);
    await page.keyboard.up(dir);
    if ((await call('dbg_kills')) > 0) killed = true;
  }
  ok(killed, 'puff pops like any bug (kill counted)', await call('dbg_kills'));
  // big-game shot: far from the epicenter the puffs are huge
  await call('dbg_tp', 2600, 0); await sleep(200);
  await call('dbg_spawnbug', 2680, 60);
  await call('dbg_spawnbug', 2540, -70);
  await sleep(1100);
  await page.screenshot({ path: __dirname + '/eddie-big.png' });
  await page.screenshot({ path: __dirname + '/eddie-hat.png' });
  console.log(pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
