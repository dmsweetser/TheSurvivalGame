// Probe: where does the pause -> SAVE+QUIT -> CONTINUE -> slot flow stop?
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const hold = async (k, ms = 200) => { await page.keyboard.down(k); await sleep(ms); await page.keyboard.up(k); await sleep(150); };
  const shot = async name => { const cv = await page.$('canvas'); await cv.screenshot({ path: __dirname + '/' + name }); console.log('shot', name, 'state=', await call('dbg_state')); };
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await hold('Space', 400); await hold('Enter', 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  console.log('in run, state=', await call('dbg_state'));
  await hold('KeyP', 500); await shot('pm-1-pause.png');
  await hold('ArrowDown', 250); await shot('pm-2-down.png');
  await hold('Space', 700); await shot('pm-3-space.png');
  await hold('Space', 600); await shot('pm-4-space2.png');
  await hold('Enter', 900); await shot('pm-5-enter.png');
  await sleep(1500); await shot('pm-6-wait.png');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
