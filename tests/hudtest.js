// hudtest.js — v1.25 feature 16: CONTEXTUAL HUD CONTROLS.
// Every prompt must name the control for the device the player is
// actually using (keyboard / gamepad / touch), follow the context's
// verbs, and swap live mid-session. P1 and the local P2 (pad 2) hold
// independent devices on a split screen.
// Device classes: 0 keyboard, 1 gamepad, 2 touch.
// Actions: 0 FIRE 1 INT 2 WALL 3 BEAN 4 CONFIRM 5 BACK 6 PAUSE 7 MOVE 8 POINT.
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));
const PADSCRIPT = npads => {
  function mkpad(index) {
    const buttons = [];
    for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
  }
  window.__pads = [];
  for (let i = 0; i < npads; i++) window.__pads.push(mkpad(i));
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
};

async function rig(browser, npads) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  if (npads) await page.addInitScript(PADSCRIPT, npads);
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const str = async (fn, act) => page.evaluate(([f, a]) => {
    const ptr = window.__TSG[f](a);
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]);
    return s;
  }, [fn, act]);
  const fresh = async () => {
    await page.evaluate(() => localStorage.clear());
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(400);
  };
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 130) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  async function poll(fn, args, want, timeout = 10000, step = 150) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
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
  async function padWalkTo(tx, ty, tol = 10, timeout = 25000) {
    const press = async (bi, on) => page.evaluate(([b, o]) => window.__btn(0, b, o), [bi, on]);
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) break;
      const want = [];
      if (x < tx - tol) want.push(15); else if (x > tx + tol) want.push(14);
      if (y < ty - tol) want.push(13); else if (y > ty + tol) want.push(12);
      for (let bIdx = 12; bIdx <= 15; bIdx++) await press(bIdx, want.includes(bIdx));
      await sleep(90);
    }
    for (let bIdx = 12; bIdx <= 15; bIdx++) await press(bIdx, false);
    const x = await call('dbg_px'), y = await call('dbg_py');
    return Math.abs(x - tx) <= tol + 6 && Math.abs(y - ty) <= tol + 6;
  }
  return { page, call, str, fresh, key, btn, poll, walkTo, padWalkTo };
}

const KB_LABELS = ['SPACE', 'E', 'F', '6', 'SPACE', 'ESC', 'P', 'ARROWS', 'CLICK'];
const PD_LABELS = ['A', 'B', 'X', 'Y', 'A', 'B', 'START', 'STICK/DPAD', 'A'];
const ACTN = ['FIRE', 'INT', 'WALL', 'BEAN', 'CONFIRM', 'BACK', 'PAUSE', 'MOVE', 'POINT'];

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ------------- phase 1: keyboard session -------------------------
  {
    const { page, call, str, fresh, key, walkTo } = await rig(browser, 0);
    await fresh();
    ok((await call('dbg_dev')) === 0, 'KB: device is keyboard at boot (no pads)');
    for (let a = 0; a < 9; a++)
      ok((await str('dbg_lbl', a)) === KB_LABELS[a], `KB: label ${ACTN[a]} = ${KB_LABELS[a]}`, await str('dbg_lbl', a));
    await key('Space', 400);
    if ((await call('dbg_state')) !== 1) await key('Enter', 400);
    ok((await call('dbg_state')) === 1, 'KB: run started');
    await call('dbg_tp', 0, 80); await sleep(250);
    await page.screenshot({ path: 'hud-kb-malldoor.png' });   // "E - ENTER THE BUG MALL"
    await key('KeyE', 300);
    ok((await call('dbg_mall')) === 1, 'KB: entered the mall');
    ok(await walkTo(105, -6148, 14), 'KB: walked to the bank counter');
    await key('KeyE', 350);
    ok((await call('dbg_state')) === 2 && (await call('dbg_station')) === 2, 'KB: bank panel open');
    await page.screenshot({ path: 'hud-kb-panel.png' });      // footer: ARROWS CHOOSE - SPACE SELECTS - E LEAVES
    await key('Escape', 300);
    await key('KeyP', 300);                                    // the pause binding
    ok((await call('dbg_state')) === 5, 'KB: pause menu open');
    await page.screenshot({ path: 'hud-kb-pause.png' });
    await key('Escape', 300);
    // WAR! map hint names the mouse verb on a keyboard-class machine
    await call('dbg_adopt', 0);
    await call('dbg_bwentm', 8); await sleep(600);
    ok((await call('dbg_bwmode')) === 8, 'KB: in WAR!');
    await page.screenshot({ path: 'hud-kb-war.png' });         // "CLICK A REGION - OR ARROWS + SPACE"
    await page.close();
  }

  // ------------- phase 2: gamepad session + live swaps -------------
  {
    const { page, call, str, fresh, key, btn, poll, padWalkTo } = await rig(browser, 1);
    await fresh();
    ok((await call('dbg_pads')) === 1, 'PAD: one pad connected');
    ok((await call('dbg_dev')) === 1, 'PAD: pad is the boot-default device when connected');
    for (let a = 0; a < 9; a++)
      ok((await str('dbg_lbl', a)) === PD_LABELS[a], `PAD: label ${ACTN[a]} = ${PD_LABELS[a]}`, await str('dbg_lbl', a));
    // swap to keyboard with one key press, and back with one pad press
    await key('ArrowDown', 200);
    ok((await call('dbg_dev')) === 0, 'SWAP: one key press flips the device to keyboard');
    ok((await str('dbg_lbl', 0)) === 'SPACE', 'SWAP: FIRE label follows (SPACE)', await str('dbg_lbl', 0));
    await btn(0, 12, 150);                                     // dpad up: navigation only
    ok((await call('dbg_dev')) === 1, 'SWAP: one pad press flips the device back to gamepad');
    ok((await str('dbg_lbl', 0)) === 'A', 'SWAP: FIRE label follows (A)', await str('dbg_lbl', 0));
    // start the run on the keyboard, then take the pad in-game
    await key('Space', 400);
    if ((await call('dbg_state')) !== 1) await key('Enter', 400);
    ok((await call('dbg_state')) === 1, 'PAD: run started');
    await call('dbg_tp', 0, 80); await sleep(200);
    await btn(0, 12, 120);                                     // pad is now the live device
    ok((await call('dbg_dev')) === 1, 'PAD: device is gamepad in-game');
    await page.screenshot({ path: 'hud-pad-malldoor.png' });   // "B - ENTER THE BUG MALL"
    await btn(0, 1, 200);                                      // B: enter the mall
    ok((await poll('dbg_mall', [], v => v === 1, 6000)) === 1, 'PAD: entered the mall on B');
    ok(await padWalkTo(105, -6148, 14), 'PAD: walked to the bank counter on the stick/dpad');
    await btn(0, 1, 250);                                      // B: open the bank
    ok((await poll('dbg_station', [], v => v === 2, 6000)) === 2, 'PAD: bank panel open');
    await page.screenshot({ path: 'hud-pad-panel.png' });      // footer: STICK/DPAD CHOOSE - A SELECTS - B LEAVES
    await page.close();
  }

  // ------------- phase 3: split screen, two devices at once --------
  {
    const { page, call, str, fresh, key, btn, poll } = await rig(browser, 2);
    await fresh();
    { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;   // the bowlpad co-op dance
      for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
        if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
      if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
      else { await key('ArrowUp', 160); } }
    await key('Space', 300);
    for (let i = 0; i < 10 && (await call('dbg_menusel')) !== 0; i++) await key('ArrowUp', 160);
    await key('Space', 500);
    if ((await call('dbg_state')) !== 1) await key('Enter', 500);
    ok((await call('dbg_state')) === 1, 'SPLIT: co-op run started');
    await call('dbg_tp', 0, 80); await sleep(150);
    await btn(1, 0, 150);                                      // P2 drops in on pad 2
    ok((await poll('dbg_p2', [], v => v > 0, 6000)) > 0, 'SPLIT: P2 dropped in');
    await key('ArrowLeft', 150);                               // P1's last input: the keyboard
    ok((await call('dbg_dev')) === 0, 'SPLIT: P1 device is keyboard');
    ok((await call('dbg_devp2')) === 1, 'SPLIT: P2 device is gamepad');
    ok((await str('dbg_lbl', 0)) === 'SPACE', 'SPLIT: P1 FIRE label is SPACE', await str('dbg_lbl', 0));
    ok((await str('dbg_lblp2', 0)) === 'A', 'SPLIT: P2 FIRE label is A at the same moment', await str('dbg_lblp2', 0));
    ok((await str('dbg_lblp2', 1)) === 'B', 'SPLIT: P2 INT label is B', await str('dbg_lblp2', 1));
    await btn(1, 1, 200);                                      // P2 enters the mall -> the screen splits
    ok((await poll('dbg_split', [], v => v === 1, 8000)) === 1, 'SPLIT: screen split (P2 in the mall)');
    await sleep(600);
    await page.screenshot({ path: 'hud-split.png' });
    await page.close();
  }
  await browser.close();

  // ------------- phase 4: touch session ----------------------------
  {
    const browser2 = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
    const ctx = await browser2.newContext({ ...devices['Pixel 7'] });
    const page = await ctx.newPage();
    page.on('pageerror', e => console.log('PAGEERROR', e.message));
    await page.goto(URL);
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
    const str = async (act) => page.evaluate((a) => {
      const ptr = window.__TSG.dbg_lbl(a);
      const mem = new Uint8Array(window.__TSG.memory.buffer);
      let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]);
      return s;
    }, act);
    await page.evaluate(() => localStorage.clear());
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await sleep(500);
    ok((await call('dbg_dev')) === 2, 'TOUCH: touch-first boot device is touch');
    ok((await str(4)) === 'TAP', 'TOUCH: CONFIRM label is TAP', await str(4));
    ok((await str(5)) === 'TAP', 'TOUCH: BACK label is TAP', await str(5));
    ok((await str(8)) === 'TAP', 'TOUCH: POINT label is TAP', await str(8));
    ok((await str(7)) === 'STICK', 'TOUCH: MOVE label is STICK (v1.33: the joystick is the only scheme — the DRAG label died with it)', await str(7));
    ok((await str(0)) === 'FIRE', 'TOUCH: FIRE label names the FIRE button (v1.33: no TAP variant remains)', await str(0));
    ok((await str(2)) === 'WALL', 'TOUCH: WALL label names the WALL button', await str(2));
    await page.screenshot({ path: 'hud-touch-menu.png' });
    await page.touchscreen.tap(320, 200); await sleep(300);    // a real tap on the menu
    ok((await call('dbg_dev')) === 2, 'TOUCH: device stays touch after a tap');
    await call('dbg_touchctl', 1);                             // stick scheme
    ok((await str(0)) === 'FIRE', 'TOUCH: FIRE label names the FIRE button in the stick scheme', await str(0));
    ok((await str(1)) === 'USE', 'TOUCH: INT label names the USE button in the stick scheme', await str(1));
    ok((await str(7)) === 'STICK', 'TOUCH: MOVE label is STICK in the stick scheme', await str(7));
    await browser2.close();
  }

  console.log(`hudtest: ${pass} passed, ${fail} failed`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(2); });
