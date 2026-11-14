// Probe 2: can door 42 be opened by Interact at all (held E, two spots)?
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
const ROOMW = 15 * 48;
const roomCX = (room) => (room - 1) * ROOMW + 360;
const doorX = (room) => (room - 1) * ROOMW + 14 * 48 + 24;

(async () => {
  const browser = await chromium.launch({ executablePath: '/home/hatch/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const holdE = async (ms) => { await page.keyboard.down('KeyE'); await sleep(ms); await page.keyboard.up('KeyE'); await sleep(250); };
  const tp = async (x, y) => { await call('dbg_bwtp', x, y); await sleep(140); };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  await call('dbg_bwent2');
  await sleep(400);
  console.log('in BW, room:', await call('dbg_entroom'));

  await tp(roomCX(42), 216);
  console.log('room:', await call('dbg_entroom'), 'door:', await call('dbg_entdoor'), 'entx:', await call('dbg_entx'));

  await tp(doorX(42) - 40, 216);
  console.log('at door-40. room:', await call('dbg_entroom'), 'entx:', await call('dbg_entx'));
  await holdE(350);
  console.log('after held E at door-40: door:', await call('dbg_entdoor'), 'key:', await call('dbg_entkey'));

  await tp(doorX(42), 216);
  await holdE(350);
  console.log('after held E on the door tile: door:', await call('dbg_entdoor'), 'room:', await call('dbg_entroom'), 'entx:', await call('dbg_entx'));

  // comparison: door 41 spot (one room west), same treatment
  await tp(doorX(41) - 40, 216);
  await holdE(350);
  console.log('door 41 spot: room:', await call('dbg_entroom'), 'door:', await call('dbg_entdoor'));
  await browser.close();
})().catch(e => { console.log('PROBE ERROR', String(e).slice(0, 300)); process.exit(1); });
