// tapbattle.js — Banzai World battles must take taps (Daniel, Nov 4:
// "in battle I'm not able to tap to select an attack"). The battle
// menu only listened to keys/pads; bw_battle_tap now mirrors the
// draw grid. Fresh buddy: 4 attacks in a 2x2 grid, box bh=84,
// rstep=18, rows at y = 360-84+22 = 298 and 316.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const tap = async (x, y) => { await page.evaluate(([tx, ty]) => window.__TSG.evt_tap(tx, ty), [x, y]); await sleep(300); };
  async function poll(fn, want, timeout = 8000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(180); }
    return call(fn);
  }
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwenter');
  T('inside Banzai World', (await call('dbg_bwstate')) === 1, await call('dbg_bwstate'));
  await call('dbg_setbudenergy', 100);
  for (let i = 0; i < 20 && (await call('dbg_bwbattle')) === 0; i++) {
    await call('dbg_bwtele_hack'); await sleep(600);
  }
  T('battle started', (await call('dbg_bwbattle')) > 0, await call('dbg_bwbattle'));
  T('choose-an-attack phase', (await poll('dbg_bwbattle', v => v === 1, 6000)) === 1, await call('dbg_bwbattle'));

  // a tap in the sky (outside the battle box) must do nothing
  const hhp0 = await call('dbg_bwhhp'), en0 = await call('dbg_budenergy');
  await tap(320, 100);
  T('tap outside the menu is ignored', (await call('dbg_bwbattle')) === 1 && (await call('dbg_bwhhp')) === hhp0 && (await call('dbg_budenergy')) === en0,
    `${await call('dbg_bwbattle')}/${await call('dbg_bwhhp')}/${await call('dbg_budenergy')}`);

  // tap attack #1 (top-right cell): centre of (316..620, 292..310) -> (470, 301)
  await tap(470, 301);
  T('tap on attack 2 leaves the choose phase (it was picked)', (await poll('dbg_bwbattle', v => v !== 1, 4000)) !== 1, await call('dbg_bwbattle'));
  T('back to choosing after the exchange', (await poll('dbg_bwbattle', v => v === 1 || v === 0, 10000)) !== -1, await call('dbg_bwbattle'));
  const hhp1 = await call('dbg_bwhhp'), en1 = await call('dbg_budenergy');
  T('the tapped attack actually landed (foe hp or energy moved)', hhp1 < hhp0 || en1 < en0 || (await call('dbg_bwbattle')) === 0,
    `${hhp0}->${hhp1} / ${en0}->${en1} / battle ${await call('dbg_bwbattle')}`);
  // second battle: tap attack #0 (top-left cell): centre (160, 301)
  if ((await call('dbg_bwbattle')) !== 0) {
    for (let i = 0; i < 40 && (await call('dbg_bwbattle')) !== 0; i++) {
      if ((await call('dbg_bwbattle')) === 1) await call('dbg_bwchoose', 0);
      await sleep(350);
    }
  }
  await call('dbg_setbudenergy', 100);
  // re-engaging takes a few tries: a post-battle grace period and the
  // nearest hacker may be the one just beaten — keep stepping on one
  for (let i = 0; i < 20 && (await call('dbg_bwbattle')) === 0; i++) {
    await call('dbg_bwtele_hack'); await sleep(600);
  }
  T('second battle reaches the choose phase', (await poll('dbg_bwbattle', v => v === 1, 8000)) === 1, await call('dbg_bwbattle'));
  await tap(160, 301);
  T('tap on attack 1 also picks', (await poll('dbg_bwbattle', v => v !== 1, 4000)) !== 1, await call('dbg_bwbattle'));
  // finish the battle by keyboard picks so nothing dangles
  for (let i = 0; i < 40 && (await call('dbg_bwbattle')) !== 0; i++) {
    if ((await call('dbg_bwbattle')) === 1) await call('dbg_bwchoose', 0);
    await sleep(350);
  }
  T('battle ends cleanly afterwards', (await call('dbg_bwbattle')) === 0, await call('dbg_bwbattle'));
  console.log(`tapbattle: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
