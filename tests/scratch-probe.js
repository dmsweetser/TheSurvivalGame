// scratch probe: can the starting keyboard kill a spawned bug?
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_tp', 0, 300); await sleep(200);
  await call('dbg_spawnbug', 34, 322);
  await sleep(300);
  console.log('bugs:', await call('dbg_bugs'), 'bug0 hp:', await call('dbg_bughp', 0), 'at', await call('dbg_bugx', 0), await call('dbg_bugy', 0), 'player', await call('dbg_px'), await call('dbg_py'));
  for (let i = 0; i < 12; i++) {
    const bx = await call('dbg_bugx', 0), by = await call('dbg_bugy', 0);
    const px = await call('dbg_px'), py = await call('dbg_py');
    const dx = bx - px, dy = by - py;
    const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
    await page.keyboard.down(dir);
    await sleep(200);
    await key('Space', 110); await key('Space', 110);
    console.log('i', i, 'aim:', await call('dbg_aim'), 'hp:', await call('dbg_bughp', 0), 'kills:', await call('dbg_kills'), 'carried:', await call('dbg_carried'), 'dist:', Math.abs(bx-px)+Math.abs(by-py));
    await page.keyboard.up(dir);
    if ((await call('dbg_kills')) > 0) break;
  }
  console.log('final kills:', await call('dbg_kills'), 'carried:', await call('dbg_carried'));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
