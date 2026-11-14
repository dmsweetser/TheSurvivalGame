// MINI-REPRO: walk doors 1..7 exactly like entrancetest, logging state to
// a FILE at every step (appendFileSync — survives a kill, no pipe loss).
const { chromium } = require('playwright-core');
const fs = require('fs');
const LOG = __dirname + '/probe-mini.log';
fs.writeFileSync(LOG, 'mini-repro start\n');
const log = (s) => { fs.appendFileSync(LOG, s + '\n'); };
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
const ROOMW = 15 * 48;
const doorX = (room) => (room - 1) * ROOMW + 14 * 48 + 24;
const roomCX = (room) => (room - 1) * ROOMW + 360;
const tileCX = (room, lx) => (room - 1) * ROOMW + lx * 48 + 24;
const tileCY = (ly) => ly * 48 + 24;

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => log('PAGEERROR: ' + String(e).slice(0, 200)));
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
  log('mall: ' + (await call('dbg_mall')));
  await call('dbg_bwent2');
  await sleep(400);
  log('in BW, room: ' + (await call('dbg_entroom')) + ' bwseed: ' + (await call('dbg_bwseed')));

  async function doorRows(room) {
    const rows = [];
    for (let ly = 1; ly <= 8; ly++)
      if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) rows.push(ly);
    return rows;
  }
  async function pressDoor(room) {
    for (let t = 0; t < 3; t++) {
      await page.keyboard.down('KeyE'); await sleep(150); await page.keyboard.up('KeyE'); await sleep(250);
      const u = await call('dbg_entdoor');
      log('  pressDoor room ' + room + ' try ' + t + ' -> upto ' + u);
      if (u >= room) return 1;
    }
    return 0;
  }
  async function tryRows(room, rows) {
    for (const ly of rows) {
      await tp(doorX(room) - 40, tileCY(ly));
      log('  tryRows room ' + room + ' row ' + ly + ' at bwx ' + (await call('dbg_bwx')) + ' bwy ' + (await call('dbg_bwy')) + ' entroom ' + (await call('dbg_entroom')));
      if (await pressDoor(room)) return 1;
    }
    return 0;
  }
  async function keySweep(room) {
    for (let ly = 1; ly <= 8; ly++) for (let lx = 1; lx <= 13; lx++) {
      await tp(tileCX(room, lx), tileCY(ly));
      if ((await call('dbg_entkey')) === 1) { log('  key found at lx ' + lx + ' ly ' + ly); return 1; }
    }
    log('  key NOT found in room ' + room);
    return 0;
  }
  async function openDoor(room) {
    const rows = await doorRows(room);
    log('door ' + room + ': rows ' + JSON.stringify(rows) + ' upto(before) ' + (await call('dbg_entdoor')));
    if (rows.length && await tryRows(room, rows)) { log('door ' + room + ': opened plain'); return; }
    await keySweep(room);
    log('door ' + room + ': after sweep entkey ' + (await call('dbg_entkey')));
    await tryRows(room, rows.length ? rows : [4]);
    log('door ' + room + ': upto(after) ' + (await call('dbg_entdoor')));
  }
  async function advance(room) {
    await openDoor(room);
    await tp(roomCX(room + 1), 216);
    log('advance ' + room + ' -> entroom ' + (await call('dbg_entroom')));
  }

  for (let room = 1; room <= 7; room++) await advance(room);
  log('DONE upto ' + (await call('dbg_entdoor')) + ' room ' + (await call('dbg_entroom')) + ' seed ' + (await call('dbg_bwseed')));
  await browser.close();
  process.exit(0);
})().catch(e => { log('ERR ' + String(e).slice(0, 300)); process.exit(1); });
