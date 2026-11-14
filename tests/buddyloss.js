// buddyloss.js — repro: adopt a buddy, let the game save, reload,
// CONTINUE through the slot picker, and check the buddy survives.
// Also screenshots the slot picker (name-listing check).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = [];
const T = (name, ok, info) => { console.log((ok ? 'PASS' : 'FAIL') + ' | ' + name + (ok ? '' : ' | ' + info)); if (!ok) fails.push(name); };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1);
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  await sleep(7000);   // autosave fires every 5s in PLAY
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('slot 1 saved', raw.length > 50, raw.length);
  T('save carries a buddy token (bd=)', / bd=\d/.test(raw), raw.slice(0, 120));
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500);   // CONTINUE -> slot picker
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await page.screenshot({ path: __dirname + '/slots-screen.png' });
  await key('Space', 600);   // load slot 1
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(400);
  T('BUDDY SURVIVED save+continue', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  console.log(fails.length ? 'BUDDYLOSS FAILURES: ' + fails.join(', ') : 'BUDDYLOSS ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
