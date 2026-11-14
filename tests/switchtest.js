// switchtest.js — changing a buddy's species costs the FULL price of
// the new species (Daniel, Nov 2), not a flat 50 KG: a rock (60) that
// wants to become a kitchen sink must pay the sink's 175.
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
  async function poll(fn, want, timeout = 15000) {
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
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  ok(await walkTo(225, -6108, 14), 'walked to the buddies counter');
  for (let i = 0; i < 6; i++) await call('dbg_feed');   // 60 kg: a rock's price

  // adopt the rock: species -> AEGNOR -> NATURAL
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open');
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);
  await key('Space', 350);
  await key('Space', 350);   // name AEGNOR
  await key('Space', 400);   // color NATURAL (row 0)
  const bud = await poll('dbg_buddy', v => v !== 0, 20000);
  ok(bud === 5, 'pet rock adopted', bud);
  await key('Escape', 300);

  // try to switch to a KITCHEN SINK (next species) holding only 120:
  // the old flat fee was 50 — that must NOT be enough any more
  for (let i = 0; i < 12; i++) await call('dbg_feed');   // 120 kg
  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open again');
  for (let i = 0; i < 7; i++) await key('ArrowDown', 130);   // row 7: CHANGE TO KITCHEN SINK (panel grew: teach rows 5/6 are move3/move4 now)
  await key('Space', 400);
  await sleep(700);
  ok((await call('dbg_buddy')) === 5, 'still a rock at 120 kg — 175 is the price now', await call('dbg_buddy'));
  ok((await call('dbg_carried')) === 1200, 'hat untouched by the refused switch', await call('dbg_carried'));

  // top up to 180 and the switch goes through at exactly 175
  for (let i = 0; i < 6; i++) await call('dbg_feed');   // 180 kg
  await key('Space', 400);
  const bud2 = await poll('dbg_buddy', v => v === 6, 20000);
  ok(bud2 === 6, 'now a kitchen sink after paying full price', bud2);
  const left = await call('dbg_carried');
  ok(Math.abs(left - 50) <= 20, 'charged exactly 175 of the 180 kg hat', left);
  await key('Escape', 300);

  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
