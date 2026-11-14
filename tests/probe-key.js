// probe-key.js — for each hotel room 1..20: is the door locked, and if so
// does standing exactly on the key tile (from dbg_entdiag) register the
// pickup? Prints one line per room to probe-key.log.
const { chromium } = require('playwright-core');
const fs = require('fs');
const LOG = __dirname + '/probe-key.log';
const out = (s) => { try { fs.appendFileSync(LOG, s + '\n'); } catch (e) {} };
fs.writeFileSync(LOG, 'probe-key start\n');
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 760 } });
  await page.goto('http://127.0.0.1:8903/the-survival-game.html', { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 30000 });
  const call = (fn, ...a) => page.evaluate(([f, args]) => window.__TSG[f](...args), [fn, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(60); };
  const tp = async (x, y) => { await call('dbg_bwtp', x, y); await sleep(160); };
  await page.evaluate(() => { try { localStorage.clear(); } catch (e) {} });
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 450); await sleep(400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(400);
  await call('dbg_adopt', 0);
  await call('dbg_bwent2', 0);
  await sleep(400);
  out('seed ' + (await call('dbg_bwseed')) + ' room ' + (await call('dbg_entroom')));
  await call('dbg_budup', 60);
  // doorStandY / pressDoor mirroring the suite
  async function doorStandY(room) {
    const x = (room - 1) * 15 * 48 + 14 * 48 + 24;
    for (let ly = 1; ly <= 9; ly++) {
      if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) return ly * 48 + 24;
    }
    return 216;
  }
  async function tryOpen(room) {
    const dx = (room - 1) * 15 * 48 + 14 * 48 + 24;
    const y = await doorStandY(room);
    // stand on every door-shaped row, like a player scanning the wall:
    // a painted dupe shares the column and bonks the press away
    const rows = [];
    for (let ly = 1; ly <= 9; ly++)
      if ((await call('dbg_enttile', (room - 1) * 15 + 14, ly)) === 2) rows.push(ly);
    for (const ly of rows.length ? rows : [4]) {
      for (let t = 0; t < 3; t++) {
        await tp(dx - 40, ly * 48 + 24);
        await key('KeyE', 250); await sleep(300);
        if ((await call('dbg_entdoor')) >= room) return true;
      }
    }
    return (await call('dbg_entdoor')) >= room;
  }
  for (let room = 1; room <= 20; room++) {
    const locked = await call('dbg_entdiag', room, 0);
    const kp = await call('dbg_entdiag', room, 3);
    const kx = kp % 100000, ky = Math.floor(kp / 100000);
    let line = 'room ' + room + ' locked ' + locked + ' key ' + kx + ',' + ky;
    if (await tryOpen(room)) { out(line + ' -> opened plain'); continue; }
    if (locked !== 1) { out(line + ' -> PLAIN DOOR DID NOT OPEN'); continue; }
    // stand on the exact key position for 2s
    await tp(kx, ky); await sleep(2000);
    const held = await call('dbg_entkey');
    line += ' entkey-after-stand ' + held + ' bwx ' + (await call('dbg_bwx')) + ' bwy ' + (await call('dbg_bwy')) + ' ev ' + (await call('dbg_entev')) + ' hp ' + (await call('dbg_buddyhp'));
    if (held === 1) {
      const ok = await tryOpen(room);
      line += ok ? ' -> opened with key' : ' -> KEY HELD BUT DOOR STUCK';
    } else {
      // second try: stand 6s, polling every 500ms
      let got = 0;
      for (let i = 0; i < 12 && !got; i++) { await sleep(500); got = await call('dbg_entkey'); }
      line += ' retry6s ' + got;
      if (got === 1) { const ok = await tryOpen(room); line += ok ? ' -> opened with key (retry)' : ' -> KEY HELD BUT DOOR STUCK (retry)'; }
    }
    out(line);
    await call('dbg_budup', 60);
  }
  out('probe-key done');
  await browser.close();
  process.exit(0);
})().catch((e) => { out('ERR ' + (e && e.message || e)); process.exit(1); });
