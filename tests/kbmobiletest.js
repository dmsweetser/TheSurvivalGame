// kbmobiletest.js — Daniel's phone path (Nov 9): TAP the menu NAME
// field (not arrow keys), type the way a phone keyboard delivers text,
// and make sure the soft keyboard can never resurface during play.
// Methods: (A) CDP key events, (B) Input.insertText (IME commit, no
// keydown at all — how Gboard/Samsung keyboards deliver most text),
// (C) wholesale value replacement + input event (autocorrect commit).
// Bug 2: after tap-selecting the field and starting a run by taps,
// text_mode() must be 0 in play and no tap may focus the kb input.
const { chromium, devices } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const dev = devices['Pixel 7'];
  const ctx = await browser.newContext({ ...dev });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const myname = () => page.evaluate(() => {
    const ptr = window.__TSG.dbg_myname();
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    let s = ''; for (let i = ptr; mem[i]; i++) s += String.fromCharCode(mem[i]);
    return s;
  });
  const kbFocused = () => page.evaluate(() => document.activeElement && document.activeElement.id === 'kb');
  const fresh = async () => {
    await page.goto('http://localhost:8903/the-survival-game.html');
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await page.evaluate(() => localStorage.clear());
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await sleep(300);
  };
  // locate the NAME field box in the wasm framebuffer (fill 44,46,66)
  const fieldPos = () => page.evaluate(() => {
    const EX = window.__TSG, mem = new Uint8Array(EX.memory.buffer), fb = EX.fb();
    for (let y = 0; y < 720; y += 2) {
      let run = 0, start = 0;
      for (let x = 0; x < 1280; x++) {
        const o = fb + (y * 1280 + x) * 4;
        if (mem[o] === 44 && mem[o + 1] === 46 && mem[o + 2] === 66) { if (!run) start = x; if (++run > 200) return { gx: (start + 110) / 2, gy: y / 2 + 6 }; }
        else run = 0;
      }
    }
    return null;
  });
  // locate the START row from live geometry: the NAME field's top sits
  // at 108 + n*22 + 8 in game coords (n = dbg_menucount), rows are 22
  // apart, and START is row 0. (A hardcoded y went stale on Nov 14
  // when Daniel shortened the title band and the stack moved up; the
  // selection's yellow bar can't be scanned for here because the
  // field tap just before leaves the selection ON the field.)
  const startRowY = async () => {
    const p = lastFieldPos;
    if (!p) return null;
    const n = await call('dbg_menucount');
    const fy = p.gy - 6;                 // fieldPos reports field top + 6
    return fy - n * 22 - 8 + 5;          // row 0's tap centre
  };
  const tapGame = async (gx, gy) => {
    const box = await page.locator('#cv').boundingBox();
    await page.touchscreen.tap(box.x + gx / 640 * box.width, box.y + gy / 360 * box.height);
    await sleep(350);
  };
  let lastFieldPos = null;   // the field's fill changes colour once
                             // selected, so later scans reuse this
  const selectFieldByTap = async () => {
    const p = await fieldPos();
    T('name field found in framebuffer', !!p, p && JSON.stringify(p));
    if (!p) return false;
    lastFieldPos = p;
    await tapGame(p.gx, p.gy);
    let tm = 0; for (let i = 0; i < 10 && tm !== 1; i++) { tm = await call('text_mode'); if (tm !== 1) await sleep(150); }
    T('tap selects the name field (text_mode 1)', tm === 1, tm);
    let foc = false; for (let i = 0; i < 15 && !foc; i++) { foc = await kbFocused(); if (!foc) await sleep(150); }
    T('kb input focused after tap-select', foc);
    return tm === 1;
  };

  // ---- method A: CDP key events into the focused input ----
  await fresh();
  if (await selectFieldByTap()) {
    const before = await myname();
    await page.keyboard.type('PIX', { delay: 90 });
    await sleep(600);
    T('A: keyboard.type lands in the name', (await myname()) === before + 'PIX', (await myname()));
  }
  // ---- method B: IME commit (Input.insertText, no key events) ----
  await fresh();
  if (await selectFieldByTap()) {
    const before = await myname();
    const cdp = await ctx.newCDPSession(page);
    await cdp.send('Input.insertText', { text: 'PIX' });
    await sleep(600);
    T('B: insertText (IME commit) lands in the name', (await myname()) === before + 'PIX', (await myname()));
    // autocorrect-style: the keyboard rewrites the whole field value
    await page.evaluate(() => {
      const kb = document.getElementById('kb');
      kb.value = 'PIXIE';
      kb.dispatchEvent(new InputEvent('input', { inputType: 'insertReplacementText', data: 'PIXIE', bubbles: true }));
    });
    await sleep(600);
    T('C: wholesale value replacement lands in the name', (await myname()) === before + 'PIXIE', (await myname()));
    // shrinking replacement (autocorrect shortens what was typed)
    await page.evaluate(() => {
      const kb = document.getElementById('kb');
      kb.value = 'PI';
      kb.dispatchEvent(new InputEvent('input', { inputType: 'insertReplacementText', data: 'PI', bubbles: true }));
    });
    await sleep(600);
    T('D: shrinking replacement converges the name', (await myname()) === before + 'PI', (await myname()));
    // the browser resets the field behind the page's back (no event),
    // then the IME commits fresh text: the frame reconcile must heal
    await page.evaluate(() => { document.getElementById('kb').value = ''; });
    await cdp.send('Input.insertText', { text: 'Q' });
    await sleep(600);
    T('E: silent value reset heals, next char lands', (await myname()) === before + 'Q', (await myname()));
  }
  // ---- bug 2: field selected, then start a run by taps ----
  await fresh();
  if (await selectFieldByTap()) {
    const sy = await startRowY();
    T('START row found in framebuffer', !!sy, sy);
    await tapGame(320, sy || 168);      // START row -> slots screen
    let st = -1; for (let i = 0; i < 10 && st !== 7; i++) { st = await call('dbg_state'); if (st !== 7) await sleep(150); }
    T('slots screen reached', st === 7, st);
    await tapGame(320, 138);            // slot 1 -> new run
    st = -1; for (let i = 0; i < 12 && st !== 1; i++) { st = await call('dbg_state'); if (st !== 1) await sleep(150); }
    T('run started (GS_PLAY)', st === 1, st);
    const tm = await call('text_mode');
    T('text_mode is 0 during gameplay', tm === 0, tm);
    // gameplay taps must never focus the keyboard input
    let summoned = false;
    for (const [gx, gy] of [[320, 180], [200, 120], [450, 260], [320, 300]]) {
      await tapGame(gx, gy);
      if (await kbFocused()) summoned = true;
    }
    T('gameplay taps never focus the kb input', !summoned);
    T('kb input stays unfocused in play', !(await kbFocused()));
  }
  console.log(`kbmobiletest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
