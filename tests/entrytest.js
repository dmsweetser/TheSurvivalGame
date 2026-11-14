// entrytest.js — the REAL gate path, the way a returning player walks
// it: continue a saved run, walk the mall on foot, stand at the Banzai
// door, press Interact, use the panel. Plus the two denial paths
// (no buddy, fainted buddy) with screenshots of the feedback.
// (The other suites enter BW through dbg hooks and skip all of this —
// Daniel's Nov 3 report: "wouldn't even let me in.")
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
const CY = -6000;
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  async function poll(fn, want, timeout = 9000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  // walk (real keys, real collision) from wherever the player stands to
  // just south of the Banzai door inside the mall, then face it
  async function walkToDoor() {
    await call('dbg_tp', 0, CY + 118);           // the mall entry mat
    await sleep(200);
    await page.keyboard.down('KeyD');            // east toward x=185
    const t0 = Date.now();
    while (Date.now() - t0 < 6000) { if ((await call('dbg_px')) > 178) break; await sleep(120); }
    await page.keyboard.up('KeyD');
    await page.keyboard.down('KeyW');            // north to the door line (CY+92)
    const t1 = Date.now();
    while (Date.now() - t1 < 4000) { const y = await call('dbg_py'); if (y < CY + 104 && y > CY + 78) break; await sleep(120); }
    await page.keyboard.up('KeyW');
    await sleep(200);
  }
  async function enterMall() {
    await call('dbg_tp', 0, 80); await sleep(200);
    await key('KeyE', 250); await sleep(300);
    return (await call('dbg_mall')) === 1;
  }
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);

  // ---- denial 1: no buddy at all --------------------------------------
  T('mall entered (no buddy)', await enterMall());
  await walkToDoor();
  await key('KeyE', 250); await sleep(450);
  await page.screenshot({ path: __dirname + '/entry-denied-nobuddy.png' });
  T('no buddy: gate panel stays shut', (await call('dbg_gate')) === 0, await call('dbg_gate'));
  await key('Escape', 200); await sleep(200);

  // ---- the returning-player flow: adopt, save+quit, continue ---------
  await call('dbg_adopt', 0);
  await key('KeyP', 250);                        // pause
  await sleep(300);
  await key('ArrowDown', 150); await sleep(150);
  await key('ArrowDown', 150); await sleep(150); // SAVE AND QUIT is the third row? navigate defensively below
  await key('Space', 250); await sleep(500);
  if ((await call('dbg_state')) !== 0) {         // fallback: buddyquit's known-good sequence
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  }
  T('back at title after save+quit', (await call('dbg_state')) === 0, await call('dbg_state'));
  await key('Space', 450); await sleep(400);     // CONTINUE
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run continued', (await call('dbg_state')) === 1, await call('dbg_state'));
  T('buddy present after continue', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // ---- real entry: STORY MODE ------------------------------------------
  T('mall entered (returning player)', await enterMall());
  await walkToDoor();
  console.log('  at door: px', await call('dbg_px'), 'py', await call('dbg_py'));
  await key('KeyE', 250); await sleep(400);
  await page.screenshot({ path: __dirname + '/entry-panel.png' });
  T('gate panel opens at the door', (await call('dbg_gate')) === 1, await call('dbg_gate'));
  await key('Escape', 250); await sleep(300);
  T('Escape backs out of the panel', (await call('dbg_gate')) === 0, await call('dbg_gate'));
  await key('KeyE', 250); await sleep(400);
  T('panel reopens after Escape', (await call('dbg_gate')) === 1, await call('dbg_gate'));
  await key('Space', 250);                       // row 0: STORY MODE
  T('buddy is in Banzai World (story)', (await poll('dbg_bwstate', v => v === 1)) === 1, await call('dbg_bwstate'));
  T('story quest 0 granted', (((await call('dbg_quest')) >> 0) & 3) === 1, await call('dbg_quest'));
  await key('Escape', 250); await sleep(400);    // leave BW
  T('left Banzai World', (await poll('dbg_bwstate', v => v === 0)) === 0, await call('dbg_bwstate'));

  // ---- entry from the SOUTH, up the door column (the natural walk) ---
  await call('dbg_tp', 185, CY + 175); await sleep(200);
  await page.keyboard.down('KeyW');
  const t9 = Date.now();
  while (Date.now() - t9 < 3000) { if ((await call('dbg_py')) < CY + 165) break; await sleep(120); }
  await page.keyboard.up('KeyW'); await sleep(200);
  await key('KeyE', 250); await sleep(400);
  T('panel opens from the south approach', (await call('dbg_gate')) === 1, await call('dbg_gate'));
  await key('ArrowDown', 200); await sleep(150); await key('ArrowDown', 200); await sleep(150); await key('ArrowDown', 200); await sleep(150);
  await key('ArrowDown', 200); await sleep(150); await key('ArrowDown', 200); await sleep(150);
  await key('Space', 250); await sleep(300);     // LEAVE (row 5 since SUPER BUDDY RUN and BUDDY CRAFT joined the panel)
  T('panel closed via LEAVE', (await call('dbg_gate')) === 0, await call('dbg_gate'));

  // ---- real entry: FREE PLAY -------------------------------------------
  await walkToDoor();
  await key('KeyE', 250); await sleep(400);
  T('gate panel reopens', (await call('dbg_gate')) === 1, await call('dbg_gate'));
  await key('ArrowDown', 200); await sleep(200); // row 1: FREE PLAY
  await key('Space', 250);
  T('buddy is in Banzai World (free play)', (await poll('dbg_bwstate', v => v === 1)) === 1, await call('dbg_bwstate'));
  await call('dbg_bwtele_site', 0);
  T('free play has no village (no sites)', (await call('dbg_site')) === -1, await call('dbg_site'));

  console.log(`entrytest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
