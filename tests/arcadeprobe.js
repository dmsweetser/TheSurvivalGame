// arcadeprobe.js — diagnostic: does E open the gate panel / arcade door?
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(150); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  console.log('state', await call('dbg_state'));
  await call('dbg_adopt', 0);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 250);
  console.log('mall', await call('dbg_mall'), 'px', await call('dbg_px'), 'py', await call('dbg_py'));
  // gate: stand at its west face
  await call('dbg_tp', 122, -5866); await sleep(300);
  console.log('at gate: px', await call('dbg_px'), 'py', await call('dbg_py'));
  await key('KeyE', 250);
  await page.screenshot({ path: __dirname + '/arcadeprobe-gate.png' });
  await key('ArrowDown', 200);
  await key('Space', 250);
  await sleep(400);
  console.log('after Down+Space at gate: bwmode', await call('dbg_bwmode'), '(0=free play in BW, -1=not in BW)');
  await call('dbg_bwexit'); await sleep(300);
  console.log('after bwexit: bwmode', await call('dbg_bwmode'), 'mall', await call('dbg_mall'), 'px', await call('dbg_px'), 'py', await call('dbg_py'));
  // arcade door
  await call('dbg_tp', 230, -5976); await sleep(300);
  console.log('at arcade door: px', await call('dbg_px'), 'py', await call('dbg_py'));
  await key('KeyE', 250);
  await sleep(300);
  console.log('inarcade', await call('dbg_inarcade'), 'px', await call('dbg_px'), 'py', await call('dbg_py'));
  await page.screenshot({ path: __dirname + '/arcadeprobe-door.png' });
  await browser.close();
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
