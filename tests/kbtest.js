// kbtest.js — the soft-keyboard bridge: when a text field owns typing
// (menu name field here), the shell must focus its real text input so
// a phone's keyboard appears, typed characters must land in the field
// exactly once, backspace must delete, and leaving the field must
// drop the keyboard (Daniel, Nov 4: no keyboard on mobile at all).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 120) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const ctx = await browser.newContext({ viewport: { width: 900, height: 600 }, hasTouch: true, isMobile: true });
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
  const key = async (code, ms = 130, wait = 160) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  // walk the main menu down to the name field (text_mode becomes 1)
  let tm = 0;
  for (let i = 0; i < 10 && tm !== 1; i++) { await key('ArrowDown'); tm = await call('text_mode'); }
  T('name field reached (text_mode 1)', tm === 1, tm);
  // the shell must have focused the hidden input -> the phone keyboard's cue
  let foc = false;
  for (let i = 0; i < 15 && !foc; i++) { foc = await kbFocused(); if (!foc) await sleep(150); }
  T('keyboard input focused while the name field is selected', foc);
  const before = await myname();
  await page.keyboard.type('ZED', { delay: 90 });
  await sleep(500);
  T('typed characters land in the name exactly once', (await myname()) === before + 'ZED', (before + '|') + (await myname()));
  await page.keyboard.press('Backspace');
  await sleep(500);
  T('backspace deletes one character', (await myname()) === before + 'ZE', await myname());
  // leave the field: arrow up to a plain menu row -> keyboard drops
  for (let i = 0; i < 10 && (await call('text_mode')) !== 0; i++) await key('ArrowUp');
  T('text_mode clears off the field', (await call('text_mode')) === 0, await call('text_mode'));
  let dropped = false;
  for (let i = 0; i < 15 && !dropped; i++) { dropped = !(await kbFocused()); if (!dropped) await sleep(150); }
  T('keyboard input blurred after leaving the field', dropped);
  console.log(`kbtest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
