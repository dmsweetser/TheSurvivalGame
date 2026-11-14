// gatetest.js — the Banzai World gate's west face must push you OUT
// (a sign error used to shove you INTO the block: "two tiles to the
// left of the shop block the path", Daniel, Nov 3).
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
  const CY = -6000;   // MALL_CY
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);                        // enter the mall properly
  T('in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));
  await call('dbg_tp', 60, CY + 134);            // west of the gate, in its band
  await sleep(150);
  await page.keyboard.down('KeyD');              // walk east into the gate block
  let maxx = -1e9;
  for (let i = 0; i < 30; i++) { await sleep(100); const x = await call('dbg_px'); if (x > maxx) maxx = x; }
  await page.keyboard.up('KeyD');
  const fx = await call('dbg_px');
  // gate west face with radius: 185-52-11 = 122. Stuck-out means ~122;
  // the old bug parked you at 185-52+11 = 144, inside the block.
  T('walking east into the gate stops at its west face', fx > 112 && fx < 134, 'x=' + fx);
  T('never shoved inside the block', maxx < 134, 'maxx=' + maxx);
  console.log(fails.length ? 'GATETEST FAILURES: ' + fails.join(', ') : 'GATETEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
