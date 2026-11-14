// hatsavetest.js — Daniel's vanishing hat (Nov 10): the save writer
// capped the hat at 200 carcasses, so any taller hat silently lost
// the rest on save/Continue. Feed a 350-carcass hat (700 KG), let it
// autosave, reload, CONTINUE through the slot picker like a player,
// and the whole hat must come back.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 130) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));

  for (let i = 0; i < 70; i++) await call('dbg_feed');   // 350 carcasses, 700 KG
  T('hat holds 700 KG before save', (await call('dbg_carried')) === 7000, await call('dbg_carried'));

  await sleep(7000);   // let the autosave take it
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('slot 1 saved', raw.length > 50, raw.length);

  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500);
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await key('Space', 600);
  T('run resumed', (await poll('dbg_state', v => v === 1)) === 1, await call('dbg_state'));
  await sleep(400);
  T('THE WHOLE HAT CAME BACK (700 KG, not capped at 400)', (await call('dbg_carried')) === 7000, await call('dbg_carried'));

  console.log('hatsavetest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
