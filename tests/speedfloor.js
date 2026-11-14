// speedfloor.js — probe: the overburdened crawl never drops below
// 40 px/s (dbg_speed is px/s x10), sprint still lifts it to ~48.8.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  const s0 = await call('dbg_speed');
  T('unburdened speed is the usual 148', s0 >= 1470 && s0 <= 1490, s0);
  for (let i = 0; i < 40; i++) await call('dbg_feed');
  const carried = await call('dbg_carried');
  const s1 = await call('dbg_speed');
  console.log('carried', carried, 'speed', s1);
  T('hat is enormous', carried >= 300, carried);
  T('speed sits exactly on the 40 floor', s1 === 400, s1);
  // sprint at the floor: hold Shift and re-read
  await page.keyboard.down('ShiftLeft'); await sleep(500);
  const s2 = await call('dbg_speed');
  await page.keyboard.up('ShiftLeft');
  T('sprint still lifts the crawl (~48.8)', s2 >= 480 && s2 <= 496, s2);
  console.log('speedfloor: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
