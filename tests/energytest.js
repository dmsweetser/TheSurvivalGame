// energytest.js — buddy energy (Nov 3): taught moves spend energy in
// the real game, the pool refills on its own, and the two NEW moves
// (move 3 / move 4) can be taught at the Banzai Buddies counter.
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
  async function poll(fn, want, timeout = 12000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(250); }
    return call(fn);
  }
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');

  // --- energy gates the second move in the field
  await call('dbg_adopt', 0);                       // puppy
  await call('dbg_teach', 1);                       // move 2 taught
  ok((await call('dbg_budenergy')) === 100, 'a fresh buddy starts with a full tank');
  await call('dbg_setbudenergy', 20);               // exactly one move-2 in the tank
  await call('dbg_tp', 0, 420);
  await sleep(300);
  const px = await call('dbg_px'), py = await call('dbg_py');
  await call('dbg_spawnbug', px + 26, py + 10);
  const spent = await poll('dbg_budenergy', v => v < 20, 12000);
  ok(spent < 20, 'the buddy spent its 20 energy on the taught move', spent);
  const refilled = await poll('dbg_budenergy', v => v >= 10, 6000);
  ok(refilled >= 10, 'energy refills on its own as you go', refilled);

  // --- the two new moves teach at the counter (rows 5 and 6)
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  ok(await walkTo(225, -6108, 14), 'walked to the buddies counter');
  await call('dbg_setbudxp', 500);
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open');
  for (let i = 0; i < 5; i++) await key('ArrowDown', 130);   // row 5: TEACH move 3
  await key('Space', 350);
  ok((await call('dbg_budups')) === 5, 'move 3 taught (skills 1|4)', await call('dbg_budups'));
  ok((await call('dbg_buddyxp')) === 440, 'move 3 cost 60 XP', await call('dbg_buddyxp'));
  await key('ArrowDown', 130);                               // row 6: TEACH move 4
  await key('Space', 350);
  ok((await call('dbg_budups')) === 13, 'move 4 taught (skills 1|4|8)', await call('dbg_budups'));
  ok((await call('dbg_buddyxp')) === 320, 'move 4 cost 120 XP', await call('dbg_buddyxp'));
  await key('Escape', 300);

  console.log(`\nenergytest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
