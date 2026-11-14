// probe: dump the east column of rooms 1-9 and key pickup state
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  console.log('mall:', await call('dbg_mall'));
  await call('dbg_bwent2'); await sleep(300);
  console.log('entroom:', await call('dbg_entroom'), 'bwstate:', await call('dbg_bwstate'));
  for (let room = 1; room <= 9; room++) {
    const col = [];
    for (let ly = 0; ly <= 9; ly++) col.push(await call('dbg_enttile', (room - 1) * 15 + 14, ly));
    console.log('room', room, 'east col:', col.join(','));
  }
  // key test in room 3 — walk through doors 1,2 properly (upto must advance),
  // then stand on EVERY tile of room 3 for 300ms watching entkey
  const tp = async (x, y) => { await call('dbg_bwtp', x, y); await sleep(140); };
  // frame probe: call EX.frame() manually N times and watch anim state move
  const frames = async (n) => page.evaluate((n) => {
    const t0 = window.__TSG.dbg_state();
    let acc = 0;
    for (let i = 0; i < n; i++) window.__TSG.frame();
    return { state: t0, mall: window.__TSG.dbg_mall(), hp: window.__TSG.dbg_buddyhp() };
  }, n);
  // drive REAL input through the loop: hold a key for wall-time ms and
  // sample a getter each 300ms — mimics the suite's timing exactly
  const holdKey = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const tileCX = (room, lx) => (room - 1) * 720 + lx * 48 + 24;
  const tileCY = (ly) => ly * 48 + 24;
  const rowsOf = async (room) => { const r = []; for (let ly = 1; ly <= 8; ly++) if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) r.push(ly); return r; };
  async function pressDoor(room) {
    for (let t = 0; t < 3; t++) {
      await page.keyboard.down('KeyE'); await sleep(150); await page.keyboard.up('KeyE'); await sleep(250);
      if ((await call('dbg_entdoor')) >= room) return 1;
    }
    return 0;
  }
  async function openDoor(room) {
    for (const ly of await rowsOf(room)) {
      await tp((room - 1) * 720 + 14 * 48 + 24 - 40, tileCY(ly));
      if (await pressDoor(room)) return 1;
    }
    return 0;
  }
  console.log('open door 1:', await openDoor(1), 'upto:', await call('dbg_entdoor'));
  await tp(tileCX(2, 7), tileCY(4)); await sleep(200);
  console.log('open door 2:', await openDoor(2), 'upto:', await call('dbg_entdoor'));
  await tp(tileCX(3, 7), tileCY(4)); await sleep(200);
  console.log('now in room:', await call('dbg_entroom'), 'upto:', await call('dbg_entdoor'));
  console.log('manual frames x30:', JSON.stringify(await frames(30)));
  let found = 0;
  for (let ly = 1; ly <= 8 && !found; ly++) for (let lx = 1; lx <= 13; lx++) {
    await tp(tileCX(3, lx), tileCY(ly)); await sleep(160);
    if ((await call('dbg_entkey')) === 1) { found = 1; console.log('key picked up at tile', lx, ly); break; }
  }
  console.log('room3 key after sweep:', await call('dbg_entkey'));
  // focus + text-mode state, then a synthetic-key walk
  console.log('focus/textmode:', JSON.stringify(await page.evaluate(() => ({
    ae: document.activeElement ? (document.activeElement.id || document.activeElement.tagName) : null,
    tm: window.__TSG.text_mode ? window.__TSG.text_mode() : 'n/a',
    vis: document.visibilityState, hasFocus: document.hasFocus(),
  }))));
  const fbhash = () => page.evaluate(() => {
    const fb = window.__TSG.fb(); const mem = new Uint8Array(window.__TSG.memory.buffer, fb, 1280 * 720 * 4);
    let h = 0; for (let i = 0; i < mem.length; i += 4096) h = (h * 31 + mem[i]) >>> 0; return h;
  });
  await tp(tileCX(1, 7), tileCY(4)); await sleep(200);
  const h0 = await fbhash();
  const hpBefore = await call('dbg_buddyhp');
  await page.evaluate(() => {
    const ev = (t, code, key) => window.dispatchEvent(new KeyboardEvent(t, { code, key, bubbles: true, cancelable: true }));
    ev('keydown', 'KeyD', 'd');
    setTimeout(() => ev('keyup', 'KeyD', 'd'), 900);
  });
  await sleep(1200);
  console.log('synthetic KeyD: fb changed?', (await fbhash()) !== h0, 'hp:', hpBefore, '->', await call('dbg_buddyhp'), 'entroom:', await call('dbg_entroom'));
  await page.screenshot({ path: __dirname + '/probe-synth.png' });
  // key placement audit: for rooms 2..12, report which grid row the key
  // pickup actually lands on (step on every centre), no events involved
  for (let room = 2; room <= 12; room++) {
    await call('dbg_bwtp', tileCX(room, 7), tileCY(4)); await sleep(120);
    let hit = '';
    for (let ly = 0; ly <= 9 && !hit; ly++) for (let lx = 1; lx <= 13; lx++) {
      await tp(tileCX(room, lx), tileCY(ly));
      if ((await call('dbg_entkey')) === 1) { hit = 'key at lx ' + lx + ' row ' + ly; break; }
    }
    console.log('room', room, hit || 'NO KEY FOUND ON CENTRES');
    // unlock the door so the next room's key spawns (mirrors real play)
    if (hit) {
      const rows = [];
      for (let ly = 1; ly <= 8; ly++) if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) rows.push(ly);
      for (const ly of rows) {
        await tp((room - 1) * 15 * 48 + 14 * 48 + 24 - 40, tileCY(ly));
        await page.keyboard.down('KeyE'); await sleep(150); await page.keyboard.up('KeyE'); await sleep(250);
        if ((await call('dbg_entdoor')) >= room) break;
      }
      console.log('  door', room, 'upto now', await call('dbg_entdoor'), 'rows were', JSON.stringify(rows));
    }
  }
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
