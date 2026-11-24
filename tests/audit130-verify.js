// audit130-verify.js — v1.30 fix verification (post-fix evidence).
// D2 DRAG mall, D1 bowl touch, H5 cabinet Up direction, T1 settings
// capture cancel, D3 cart waiting room DRAG start, H8 log (visual).
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
const T = (name, cond) => { console.log((cond ? 'PASS ' : 'FAIL ') + name); cond ? pass++ : fail++; };

async function bootMobile(browser) {
  const ctx = await browser.newContext({ ...devices['Pixel 7'] });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(500);
  await call('set_mobile', 1); await sleep(300);
  return { ctx, page, call };
}
async function startRunTouch(page, call) {
  const tapGame = async (gx, gy, wait = 450) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  await tapGame(320, 117);
  if ((await call('dbg_state')) === 7) await tapGame(320, 138);
  await sleep(500);
  await call('dbg_adopt', 0);
  return tapGame;
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ---------- Boot A: DRAG mall (D2) + Bug Bowl touch (D1) ----------
  {
    const { ctx, page, call } = await bootMobile(browser);
    const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
    const ftap = async (x, y, wait = 550) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
    T('A0 scheme defaults to DRAG', (await call('dbg_touchctl')) === 0);
    await startRunTouch(page, call);
    await call('dbg_tp', 0, 80); await sleep(250);
    await ftap(320, 30, 600);
    T('A1 DRAG tap enters the mall', (await call('dbg_mall')) === 1);
    await call('dbg_tp', -315, -6148); await sleep(250);
    await ftap(320, 180, 600);
    T('A2 D2: DRAG tap opens the Counter panel', (await call('dbg_state')) === 2 && (await call('dbg_station')) === 0);
    await ftap(24, 24, 600);
    T('A3 tap outside closes the panel', (await call('dbg_state')) === 1);
    await call('dbg_tp', 0, -5810); await sleep(250);
    await ftap(320, 180, 600);
    T('A4 D2: DRAG tap on the exit mat leaves the mall', (await call('dbg_mall')) === 0);
    // Bug Bowl under DRAG
    await call('dbg_tp', 0, 80); await sleep(200);
    await ftap(320, 30, 600);
    T('A5 back inside the mall', (await call('dbg_mall')) === 1);
    await call('dbg_feed'); await call('dbg_feed');
    await call('dbg_tp', -180, -5868); await sleep(250);
    await ftap(320, 180, 600);
    T('A6 bowl panel opens (station 4)', (await call('dbg_state')) === 2 && (await call('dbg_station')) === 4);
    await ftap(320, 180, 600);
    T('A7 D1: tap starts a bowl game (phase 0)', (await call('dbg_bowl_ph', 0)) === 0);
    await finger(7, 140, 180, 0); await sleep(700);
    const tx = await call('dbg_bowl_tx', 0);
    T('A8 D1: held finger drags the thrower left (tx=' + tx + ')', tx < -15);
    await finger(7, 140, 180, 2); await sleep(300);
    await ftap(320, 180, 500);
    T('A9 D1: tap locks the angle (phase 1)', (await call('dbg_bowl_ph', 0)) === 1);
    await ftap(320, 180, 500);
    T('A10 D1: tap sets power (phase 2)', (await call('dbg_bowl_ph', 0)) === 2);
    await ftap(320, 180, 400);
    let ph = await call('dbg_bowl_ph', 0);
    for (let i = 0; i < 6 && ph === 2; i++) { await sleep(300); ph = await call('dbg_bowl_ph', 0); }
    T('A11 D1: tap throws (phase left 2, now ' + ph + ')', ph === 3 || ph === 4);
    await page.keyboard.press('Escape'); await sleep(400);
    T('A12 bowl panel closes', (await call('dbg_state')) === 1);
    await ctx.close();
  }

  // ---------- Boot B: H5 cabinet panel Up direction (Castle, 3 rows) ----------
  {
    const ctx = await browser.newContext({ viewport: { width: 1280, height: 720 } });
    const page = await ctx.newPage();
    page.on('pageerror', e => console.log('PAGEERROR', e.message));
    const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
    const key = async (k, w = 350) => { await page.keyboard.press(k); await sleep(w); };
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(500);
    if ((await call('dbg_state')) === 0) await key('Enter', 500);
    if ((await call('dbg_state')) === 7) { await key('ArrowDown', 200); await key('Enter', 500); }
    await sleep(400);
    await call('dbg_adopt', 0);
    await call('dbg_tp', 0, -11898); await sleep(300);   // CASTLE cabinet stand point
    await key('KeyE', 600);
    T('B1 Castle cabinet panel open', (await call('dbg_arcgate')) === 1);
    await page.screenshot({ path: __dirname + '/v130-shots/verify-castle-panel-sel0.png' });
    await key('ArrowUp', 400);
    await page.screenshot({ path: __dirname + '/v130-shots/verify-castle-panel-up.png' });
    await key('Space', 600);
    T('B2 H5: Up from row 0 wraps to LEAVE (panel closed, not in BW)', (await call('dbg_arcgate')) === 0 && (await call('dbg_bwstate')) === 0);
    await ctx.close();
  }

  // ---------- Boot C: T1 settings capture — touch cancel ----------
  {
    const { ctx, page, call } = await bootMobile(browser);
    const tapGame = async (gx, gy, wait = 450) => {
      const box = await page.locator('#cv').boundingBox();
      await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
      await sleep(wait);
    };
    // title: SETTINGS is row index 2 in the keyboard flavour, but the
    // mobile title inserts CONTROLS — find SETTINGS by stepping: the
    // menu rows on mobile are START / HOW TO PLAY / SETTINGS / ...
    // Tap the 3rd row (y=138 per earlier touch audits).
    await tapGame(320, 117); await sleep(300);
    // if that opened slots, back out
    if ((await call('dbg_state')) === 7) { await page.keyboard.press('Escape'); await sleep(400); }
    // navigate by taps: settings row position from the v130 title shot
    // (mobile flavour): rows at y 117 (START), 138 (HOW TO PLAY) — the
    // audit used 117/138 for START/slot; SETTINGS sits below HOW TO PLAY.
    await tapGame(320, 159, 600);
    T('C1 settings open', (await call('dbg_state')) === 3);
    if ((await call('dbg_state')) === 3) {
      // MOVE UP binding row: set_row_y(4) = 134 (game coords)
      await tapGame(320, 134, 400);   // select the row
      await tapGame(320, 134, 500);   // tap again -> capture
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-capture.png' });
      await tapGame(320, 54, 500);    // tap MUSIC VOLUME row: post-fix cancels capture + selects
      await page.keyboard.press('KeyQ'); await sleep(500);
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-after-q.png' });
      // if capture had survived, Q would now be MOVE UP's binding and
      // the capture line would be gone with the bind changed — judged
      // from the screenshot (row must still read W).
      await tapGame(320, 134, 400);
      await tapGame(320, 134, 500);   // capture again
      await page.keyboard.press('KeyQ'); await sleep(500);
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-bound-q.png' });
    }
    await ctx.close();
  }

  // ---------- Boot D: D3 cart waiting room under DRAG ----------
  {
    const { ctx, page, call } = await bootMobile(browser);
    const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
    const ftap = async (x, y, wait = 550) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
    await startRunTouch(page, call);
    await call('dbg_tp', 0, 80); await sleep(200);
    await ftap(320, 30, 600);   // into the mall
    await call('dbg_tp', 256, -11898); await sleep(300);   // CART cabinet (arcade reached via tp, harness staging)
    await ftap(320, 180, 700);  // cabinet panel via DRAG tap (interact)
    T('D1 cart cabinet panel open', (await call('dbg_arcgate')) === 1);
    await ftap(320, 165, 900);  // tap the PLAY row area
    await sleep(800);
    const cs = await call('dbg_crtstate');
    T('D2 in cart waiting room (crtstate=' + cs + ')', cs === 3 || (await call('dbg_bwstate')) !== 0);
    await ftap(320, 180, 600);
    await sleep(1200);
    const cs2 = await call('dbg_crtstate');
    T('D3 D3: DRAG tap starts the Grand Prix (crtstate ' + cs + ' -> ' + cs2 + ')', cs2 !== cs && cs2 >= 0);
    await ctx.close();
  }

  console.log('VERIFY DONE pass=' + pass + ' fail=' + fail);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
