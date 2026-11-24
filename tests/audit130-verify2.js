// audit130-verify2.js — v1.30 fix verification, boots B/C/D with the
// corrected harness paths (proper mall->arcade entry; settings state 6;
// desktop mouse for the settings capture test).
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
const T = (name, cond) => { console.log((cond ? 'PASS ' : 'FAIL ') + name); cond ? pass++ : fail++; };

async function bootDesktop(browser) {
  const ctx = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);
  const key = async (k, w = 350) => { await page.keyboard.press(k); await sleep(w); };
  const clickGame = async (gx, gy, wait = 450) => {
    const box = await page.locator('#cv').boundingBox();
    await page.mouse.click(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(wait);
  };
  return { ctx, page, call, key, clickGame };
}
async function startRunKb(page, call, key) {
  if ((await call('dbg_state')) === 0) await key('Enter', 600);       // START -> slots
  if ((await call('dbg_state')) === 7) await key('Enter', 700);       // slot 1 -> run
  await call('dbg_adopt', 0);
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ---------- Boot B: H5 cabinet panel Up (Castle, 3 rows) ----------
  {
    const { ctx, page, call, key } = await bootDesktop(browser);
    await startRunKb(page, call, key);
    T('B0 run started', (await call('dbg_state')) === 1);
    await call('dbg_tp', 0, 80); await sleep(250);
    await key('KeyE', 500);
    T('B0b in the mall', (await call('dbg_mall')) === 1);
    await call('dbg_tp', 350, -5976); await sleep(250);
    await key('KeyE', 500);
    T('B0c in the arcade', (await call('dbg_inarcade')) === 1);
    await call('dbg_tp', 0, -11898); await sleep(250);
    await key('KeyE', 500);
    T('B1 Castle cabinet panel open', (await call('dbg_arcgate')) === 1);
    await page.screenshot({ path: __dirname + '/v130-shots/verify-castle-panel-sel0.png' });
    await key('ArrowUp', 400);
    await page.screenshot({ path: __dirname + '/v130-shots/verify-castle-panel-up.png' });
    await key('Space', 600);
    T('B2 H5: Up from row 0 wraps to LEAVE (panel closed, not in BW)', (await call('dbg_arcgate')) === 0 && (await call('dbg_bwstate')) === 0);
    await ctx.close();
  }

  // ---------- Boot C: T1 settings capture — tap cancels, key binds ----------
  {
    const { ctx, page, call, key, clickGame } = await bootDesktop(browser);
    await key('ArrowDown'); await key('ArrowDown'); await key('Space', 600);
    T('C1 settings open (state 6)', (await call('dbg_state')) === 6);
    if ((await call('dbg_state')) === 6) {
      await clickGame(320, 134, 400);   // select MOVE UP row
      await clickGame(320, 134, 500);   // click again -> capture
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-capture.png' });
      await clickGame(320, 54, 500);    // click MUSIC VOLUME: post-fix cancels capture + selects the row
      await key('KeyQ', 500);           // must NOT bind (capture is off)
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-after-q.png' });
      await clickGame(320, 134, 400);   // select MOVE UP again
      await clickGame(320, 134, 500);   // capture again
      await key('KeyQ', 500);           // now Q binds MOVE UP
      await page.screenshot({ path: __dirname + '/v130-shots/verify-settings-bound-q.png' });
    }
    await ctx.close();
  }

  // ---------- Boot D: D3 cart waiting room under DRAG ----------
  {
    const ctx = await browser.newContext({ ...devices['Pixel 7'] });
    const page = await ctx.newPage();
    page.on('pageerror', e => console.log('PAGEERROR', e.message));
    const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
    const finger = async (id, x, y, phase) => { await call('touch_point', id, x, y, phase); };
    const ftap = async (x, y, wait = 550) => { await finger(7, x, y, 0); await sleep(110); await finger(7, x, y, 2); await sleep(wait); };
    const tapGame = async (gx, gy, wait = 450) => {
      const box = await page.locator('#cv').boundingBox();
      await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
      await sleep(wait);
    };
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(500);
    await call('set_mobile', 1); await sleep(300);
    await tapGame(320, 117);
    if ((await call('dbg_state')) === 7) await tapGame(320, 138);
    await sleep(500);
    await call('dbg_adopt', 0);
    await call('dbg_tp', 0, 80); await sleep(200);
    await ftap(320, 30, 600);
    T('D0 in the mall', (await call('dbg_mall')) === 1);
    await call('dbg_tp', 350, -5976); await sleep(250);
    await ftap(320, 180, 600);
    T('D0b in the arcade (DRAG tap on the door)', (await call('dbg_inarcade')) === 1);
    await call('dbg_tp', 256, -11898); await sleep(250);
    await ftap(320, 180, 700);
    T('D1 cart cabinet panel open', (await call('dbg_arcgate')) === 1);
    await ftap(320, 168, 900);   // PLAY row
    await sleep(700);
    T('D2 in the waiting room (crtstate=3)', (await call('dbg_crtstate')) === 3);
    await ftap(320, 180, 600);
    await sleep(900);
    const cs2 = await call('dbg_crtstate');
    T('D3 D3: DRAG tap starts the Grand Prix (crtstate 3 -> ' + cs2 + ')', cs2 === 0 || cs2 === 1);
    await ctx.close();
  }

  console.log('VERIFY2 DONE pass=' + pass + ' fail=' + fail);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
