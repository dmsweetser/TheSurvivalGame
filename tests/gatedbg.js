const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwstory');
  await sleep(300);
  await call('dbg_bwgivecrank');
  console.log('crank bit', ((await call('dbg_quest')) >> 23) & 1);
  await call('dbg_bwtp', 5800, 3760);
  await sleep(250);
  console.log('start y', await call('dbg_bwy'), 'battle', await call('dbg_bwbattle'));
  await page.keyboard.down('ArrowUp');
  for (let i = 0; i < 10; i++) { await sleep(200); console.log('t+' + (i + 1) * 200 + 'ms y', await call('dbg_bwy'), 'site', await call('dbg_site'), 'battle', await call('dbg_bwbattle')); }
  await page.keyboard.up('ArrowUp');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
