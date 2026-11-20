// e2e-touch.js — END-TO-END USABILITY PASS, touch phase (v1.25).
// A Pixel 7 context (the kbmobiletest device): the shell presents
// the mobile UI itself. Menu rows take REAL touchscreen taps through
// the canvas (tapGame converts game coords to CSS px, kbmobiletest's
// idiom). In play, fingers go through touch_point — the exact export
// the shell's own pointer handler calls for real fingers (the
// touchaudit path): floating stick to walk, USE button (514,318) to
// interact, FIRE button (582,278) to act. STICK scheme is chosen on
// the title's own CONTROLS row, the way a player picks it.
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const ctx = await browser.newContext({ ...devices['Pixel 7'] });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  const tapGame = async (gx, gy) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(320);
  };
  const finger = (id, x, y, phase) => call('touch_point', id, x, y, phase);
  async function stickWalkTo(tx, ty, tol = 14, timeout = 30000) {
    const t0 = Date.now();
    await finger(1, 110, 200, 0); await sleep(80);
    let arrived = false, lx = 1e9, ly = 1e9, stallSince = Date.now(), detourUntil = 0;
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) { arrived = true; break; }
      if (Math.abs(x - lx) > 3 || Math.abs(y - ly) > 3) { lx = x; ly = y; stallSince = Date.now(); }
      else if (Date.now() - stallSince > 2500 && Date.now() > detourUntil) detourUntil = Date.now() + 700;   // snagged on a fixture: back off south and retry
      let dx = Math.abs(x - tx) <= tol ? 0 : Math.sign(tx - x) * 44;
      let dy = Math.abs(y - ty) <= tol ? 0 : Math.sign(ty - y) * 44;
      if (Date.now() < detourUntil) { dx = 0; dy = 44; }
      await finger(1, 110 + dx, 200 + dy, 1);
      await sleep(130);
    }
    await finger(1, 110, 200, 2); await sleep(150);
    return arrived;
  }
  const useBtn = async () => { await finger(2, 514, 318, 0); await sleep(160); await finger(2, 514, 318, 2); await sleep(350); };
  const fireBtn = async () => { await finger(2, 582, 278, 0); await sleep(160); await finger(2, 582, 278, 2); await sleep(350); };

  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 25000 });
  await sleep(600);
  T('boots to the title on the mobile UI', (await call('dbg_state')) === 0);

  // pick the STICK control scheme on the title's own CONTROLS row
  // (mobile flavour: row 2). Tap targets are found the player's way:
  // the rows are centred, ~26px apart from y=116 (touchaudit's map).
  if ((await call('dbg_touchctl', -1)) !== 1) {
    await tapGame(320, 168);   // CONTROLS row (3rd row)
    await sleep(300);
  }
  T('control scheme is STICK', (await call('dbg_touchctl', -1)) === 1, await call('dbg_touchctl', -1));

  await tapGame(320, 116);   // START row
  await sleep(400);
  await tapGame(320, 138);   // slot 1 (kbmobiletest's proven slot row)
  if ((await call('dbg_state')) === 4) {
    // the soft keyboard is the mobile name path; if it is up, tap DONE-side keys is a
    // suite of its own (kbmobiletest) — here, confirm with the shell's own Enter route
    await page.keyboard.press('Enter');
    await sleep(400);
    console.log('NOTE name entry confirmed via Enter (soft-keyboard depth lives in kbmobiletest)');
  }
  T('run started by taps', (await poll('dbg_state', v => v === 1, 10000)) === 1, await call('dbg_state'));

  // walk to the hut on the floating stick, enter with the USE button
  T('walked to the hut door on the touch stick', await stickWalkTo(0, 80, 13));
  await useBtn();
  T('entered the Mall with the USE button', (await poll('dbg_mall', v => v === 1, 6000)) === 1, await call('dbg_mall'));
  for (let i = 0; i < 3; i++) await call('dbg_feed');   // carcass staging (as bowlpad)

  // the Bank by stick + USE; deposit by tapping the panel row
  T('walked to the Bug Bank on the stick', await stickWalkTo(105, -6148, 14));
  await useBtn();
  T('bank panel opens on USE', (await poll('dbg_station', v => v === 2, 5000)) === 2 && (await call('dbg_state')) === 2, await call('dbg_station'));
  { const b0 = await call('dbg_bank');
    // station rows live at MALL_ROW0 + i*18 (row 0 = 80); a tap
    // selects, a tap on the selected row activates — tap twice
    await tapGame(320, 85);
    await tapGame(320, 85);
    await sleep(400);
    T('tapping DEPOSIT ALL banked the stack', (await call('dbg_bank')) > b0, b0 + ' -> ' + (await call('dbg_bank'))); }
  await tapGame(24, 24);   // outside the panel: tap-close
  T('bank panel tap-closes from outside', (await poll('dbg_state', v => v === 1, 5000)) === 1, await call('dbg_state'));

  // the lanes: throw with the FIRE button
  T('walked to the Bug Bowl counter on the stick', await stickWalkTo(-180, -5868, 14));
  await useBtn();
  T('bowling view opens on USE', (await poll('dbg_station', v => v === 4, 5000)) === 4, await call('dbg_station'));
  { const f0 = await call('dbg_bowl_frame');
    await fireBtn(); await fireBtn(); await fireBtn(); await fireBtn();
    const moved = await poll('dbg_bowl_frame', v => v !== f0, 12000);
    T('a ball was thrown with the FIRE button', moved !== f0 || (await call('dbg_bowl_pins')) < 10,
      'frame=' + f0 + ' -> ' + (await call('dbg_bowl_frame'))); }
  // the lanes eat taps as bowl input — the touch player's back is
  // the shell's ✕ button (an Esc pulse), like everywhere else
  await page.tap('#escbtn');
  await sleep(400);
  T('bowling view closes on the ✕ button', (await poll('dbg_state', v => v === 1, 5000)) === 1, await call('dbg_state'));

  // adopt a buddy first — the Banzai gate refuses without one
  // ("YOU NEED A BANZAI BUDDY FIRST"), on every device
  for (let i = 0; i < 8 && (await call('dbg_carried')) < 800; i++) await call('dbg_feed');
  T('walked to the Banzai Buddies counter on the stick', await stickWalkTo(315, -6148, 14));
  await useBtn();
  T('buddies panel opens on USE', (await poll('dbg_station', v => v === 3, 5000)) === 3 && (await call('dbg_state')) === 2, await call('dbg_station'));
  // station rows at MALL_ROW0 + i*18 (row 0 = 80): tap selects, a
  // second tap on the selected row activates
  await tapGame(320, 152); await tapGame(320, 152);   // row 4: PET ROCK
  T('buddies: species picked by taps', (await call('dbg_budstage')) === 1, await call('dbg_budstage'));
  await tapGame(320, 85); await tapGame(320, 85);     // name row 0 (already selected: a tap activates; stage reads under rapid taps are racy — the completed adoption below is the asserted verb)
  console.log('NOTE after name taps: budstage=' + (await call('dbg_budstage')));
  await tapGame(320, 85); await tapGame(320, 85);     // colour row 0
  T('buddies: PET ROCK adopted through the tapped panel', (await poll('dbg_buddy', v => v !== 0, 8000)) === 5, await call('dbg_buddy'));
  await tapGame(24, 24);
  T('buddies panel tap-closes', (await poll('dbg_state', v => v === 1, 5000)) === 1, await call('dbg_state'));

  // the Banzai gate: tap the STORY row, roam, leave via the ✕ button
  T('walked to the Banzai gate on the stick', await stickWalkTo(215, -5868, 14));
  await useBtn();
  await sleep(300);
  await tapGame(320, 160);   // gate panel row 0: STORY MODE (rows at panel y+52+r*20 = 157/177/197)
  T('STORY MODE launches from a tapped gate row', (await poll('dbg_bwmode', v => v === 1, 6000)) === 1, await call('dbg_bwmode'));
  { const x0 = await call('dbg_bwx');
    await finger(1, 110, 200, 0); await sleep(60);
    await finger(1, 154, 200, 1); await sleep(800);
    await finger(1, 154, 200, 2); await sleep(150);
    T('the buddy roams Banzai World on the stick', (await call('dbg_bwx')) !== x0, x0 + ' -> ' + (await call('dbg_bwx'))); }
  // touch exit from Banzai World is the shell's own ✕ button (Back or
  // pause): it pulses Esc, and Esc leaves BW instantly by design —
  // the Start menu itself is a keyboard/pad affordance (documented
  // in the report; the CONTROLS panel inside BW is likewise
  // keyboard/pad-only, while the overworld pause opens on Esc).
  await page.tap('#escbtn');
  await sleep(400);
  T('the ✕ button leaves Banzai World back to the Mall', (await poll('dbg_bwmode', v => v === -1, 6000)) === -1 && (await call('dbg_mall')) === 1,
    'mode=' + (await call('dbg_bwmode')) + ' mall=' + (await call('dbg_mall')));

  console.log(`e2e-touch: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
