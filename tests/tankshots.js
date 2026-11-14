// tankshots.js — screenshots for the Nov 11 Tank Rescue rework:
// widened arcade cabinets, the mission line + yellow arrow in play,
// and the blockade gun behind its barrier.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
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
  await call('dbg_adopt', 0);
  await call('dbg_arcade'); await sleep(300);
  await call('dbg_tp', 0, -11900); await sleep(600);
  await page.screenshot({ path: 'arcadeshot-cabinets-wide.png' });
  await call('dbg_tp', 219, -12108); await sleep(200);
  await key('KeyE', 250); await sleep(300);
  await key('Space', 300); await sleep(800);
  await page.screenshot({ path: 'tankshot-spawn.png' });
  await call('dbg_tankquota');
  await call('dbg_tankwarp', 2450); await sleep(700);
  await page.screenshot({ path: 'tankshot-blockade.png' });
  console.log('shots done');
  await browser.close();
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
