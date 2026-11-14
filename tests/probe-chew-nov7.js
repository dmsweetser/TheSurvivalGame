// Probe: do spawned bugs aggro the player and chew a blocking wall?
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
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await hold('Space', 400); await hold('Enter', 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await call('dbg_tp', 0, 300); await sleep(300);
  console.log('mkwall ->', await call('dbg_mkwall', 0, 336, 1), 'walls=', await call('dbg_walls'), 'wallhp=', await call('dbg_wallhp'));
  await call('dbg_spawnbug', 0, 356);
  await call('dbg_spawnbug', -12, 360);
  await call('dbg_spawnbug', 12, 360);
  console.log('bugs=', await call('dbg_bugs'));
  for (let t = 0; t < 8; t++) {
    await sleep(2500);
    let pos = [];
    for (let i = 0; i < 3; i++) pos.push(`(${await call('dbg_bugx', i)},${await call('dbg_bugy', i)})`);
    console.log(`t=${(t + 1) * 2.5}s bugs at`, pos.join(' '), 'aggro=', await call('dbg_bugaggro'), 'wallhp=', await call('dbg_wallhp'), 'php=', await call('dbg_hp'));
  }
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/probe-chew.png' });
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
