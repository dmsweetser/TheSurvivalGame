// buddyquit.js — Daniel's exact scenario (Nov 3): BUY a buddy at the
// Banzai Buddies counter, play in the normal game, SAVE & QUIT from the
// pause menu, then CONTINUE — the buddy must still be there.
// Variant B repeats it with the buddy fainted before the save, to see
// exactly what "gone" looks like in that case.
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
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  async function poll(fn, want, timeout = 12000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  async function freshRun() {
    await page.goto('http://localhost:8903/the-survival-game.html');
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await page.evaluate(() => localStorage.clear());
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await key('Space', 400);
    if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  }
  async function buyPuppy() {
    await call('dbg_tp', 0, 80); await sleep(150);
    await key('KeyE', 300);                       // into the mall
    await call('dbg_tp', 315, -6148); await sleep(200);
    for (let i = 0; i < 10; i++) await call('dbg_feed');   // 100 kg: a puppy's price
    await key('KeyE', 350);                       // buddies panel
    if ((await call('dbg_station')) !== 3) return false;
    await key('Space', 350);                      // row 0: PUPPY
    if ((await call('dbg_budstage')) !== 1) return false;
    await key('Space', 350);                      // name: row 0
    if ((await call('dbg_budstage')) !== 2) return false;
    await key('Space', 400);                      // color: row 0 -> payment -> adopt
    return (await poll('dbg_buddy', v => v !== 0)) === 1;
  }
  async function saveQuitContinue(tag) {
    await key('Escape', 350);                     // pause
    T(tag + ': paused', (await call('dbg_state')) === 5, await call('dbg_state'));
    // pause menu is 3 rows since Nov 6 (Daniel): CONTINUE / CONTROLS /
    // SAVE + QUIT TO TITLE — two Downs reach the quit row.
    await key('ArrowDown', 150);
    await key('ArrowDown', 150);
    await key('Space', 500);                      // SAVE + QUIT TO TITLE
    T(tag + ': back at title', (await poll('dbg_state', v => v === 0)) === 0, await call('dbg_state'));
    await sleep(400);
    await key('Space', 500);                      // CONTINUE -> slot picker
    T(tag + ': slot picker', (await poll('dbg_state', v => v === 7)) === 7, await call('dbg_state'));
    await key('Space', 600);                      // load slot 1
    T(tag + ': run resumed', (await poll('dbg_state', v => v === 1)) === 1, await call('dbg_state'));
    await sleep(500);
  }

  // ---- Variant A: Daniel's scenario, beat for beat -------------------------
  await freshRun();
  T('A: puppy bought at the counter', await buyPuppy());
  await key('Escape', 300);                       // close the panel
  await call('dbg_tp', 0, -5810); await sleep(200);
  await key('KeyE', 300);                         // out of the mall
  T('A: back in the field', (await call('dbg_mall')) === 0);
  // play in the normal game for a bit: stroll and let time pass
  await page.keyboard.down('ArrowRight'); await sleep(1200); await page.keyboard.up('ArrowRight');
  await sleep(4500);
  const by = await call('dbg_budy'), py = await call('dbg_py');
  T('A: buddy is with the player in the field', Math.abs(by - py) < 300, 'buddy y ' + by + ' vs player y ' + py);
  const rawA = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  await saveQuitContinue('A');
  T('A: BUDDY SURVIVED save+quit+continue', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  T('A: buddy is awake (hp > 0)', (await call('dbg_buddyhp')) > 0, await call('dbg_buddyhp'));
  console.log('   (slot before quit carried bd=: ' + / bd=\d/.test(rawA) + ')');

  // ---- Variant B: same, but the buddy fainted before the save ---------------
  await freshRun();
  T('B: puppy bought at the counter', await buyPuppy());
  await key('Escape', 300);
  await call('dbg_tp', 0, -5810); await sleep(200);
  await key('KeyE', 300);
  await call('dbg_budfaint');
  await sleep(400);
  T('B: buddy fainted in the field', (await call('dbg_buddyhp')) === 0, await call('dbg_buddyhp'));
  await saveQuitContinue('B');
  T('B: buddy still OWNED after continue', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  T('B: buddy loads FAINTED (hp 0, waits for revival)', (await call('dbg_buddyhp')) === 0, await call('dbg_buddyhp'));
  const byB = await call('dbg_budy');
  T('B: fainted buddy was carried home to the mall (not left in the field)', byB < -5900, 'buddy y ' + byB);
  await page.screenshot({ path: __dirname + '/buddyquit-fainted.png' });

  console.log('\nbuddyquit: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
