// Probe: why does the brass key for door 42 never get pocketed?
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
const ROOMW = 15 * 48;
const roomCX = (room) => (room - 1) * ROOMW + 360;
const tileCX = (room, lx) => (room - 1) * ROOMW + lx * 48 + 24;
const tileCY = (ly) => ly * 48 + 24;

(async () => {
  const browser = await chromium.launch({ executablePath: '/home/hatch/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const tp = async (x, y) => { await call('dbg_bwtp', x, y); await sleep(140); };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  console.log('mall:', await call('dbg_mall'));
  await call('dbg_bwent2');
  await sleep(400);
  console.log('bwstate:', await call('dbg_bwstate'), 'room:', await call('dbg_entroom'));

  // jump straight to room 42 and look around
  await tp(roomCX(42), 216);
  console.log('room now:', await call('dbg_entroom'), 'ev:', await call('dbg_entev'), 'door_upto:', await call('dbg_entdoor'), 'hp:', await call('dbg_buddyhp'));
  // tile map of room 42
  for (let ly = 0; ly <= 9; ly++) {
    let row = '';
    for (let lx = 0; lx <= 14; lx++) row += String(await call('dbg_enttile', 41 * 15 + lx, ly));
    console.log('tiles ly=' + ly, row);
  }
  // sweep every interior tile like the suite does, logging the key state
  let found = 0;
  for (let ly = 1; ly <= 8 && !found; ly++) for (let lx = 1; lx <= 13 && !found; lx++) {
    await tp(tileCX(42, lx), tileCY(ly));
    if ((await call('dbg_entkey')) === 1) { found = 1; console.log('KEY POCKETED at lx=' + lx, 'ly=' + ly); }
  }
  console.log('sweep done. key:', await call('dbg_entkey'), 'hp:', await call('dbg_buddyhp'), 'room:', await call('dbg_entroom'), 'ev:', await call('dbg_entev'));
  await browser.close();
})().catch(e => { console.log('PROBE ERROR', String(e).slice(0, 300)); process.exit(1); });
