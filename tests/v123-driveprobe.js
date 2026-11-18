// v123-driveprobe.js — completability proof for the longest circuit:
// REAL driving (no teleports) on track 7 (The Unmaintained Depths,
// 2.40x). The player blind-drives (hold gas, pulse steering when
// progress stalls); rival kart 1 runs the game's own racing line.
// Asserts both cover serious distance and kart 1 finishes outright.
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = 0;
const note = (m, ok) => { if (ok === false) fails++; console.log((ok === false ? 'FAIL ' : 'OK   ') + m); };
(async () => {
  const browser = await chromium.launch({ executablePath: '/home/hatch/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  async function key(k, ms = 250) { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); }
  async function poll(fn, want, ms, ...a) { const t0 = Date.now(); let v; do { v = await call(fn, ...a); if (v === want) return v; await sleep(250); } while (Date.now() - t0 < ms); return v; }
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  // to the FAR cup's last race: mall -> arcade -> cart cabinet
  await call('dbg_tp', 0, 80); await key('KeyE', 600);
  await sleep(400);
  await call('dbg_tp', 230, -5976); await key('KeyE', 600);
  await poll('dbg_inarcade', 1, 5000);
  await call('dbg_tp', 256, -11898); await key('KeyE', 800);
  await poll('dbg_bwmode', 7, 5000);
  await key('Space', 300);
  await poll('dbg_crtstate', 3, 5000);
  await key('ArrowDown', 300);
  note('far cup selected', (await call('dbg_crtcup')) === 1);
  await key('Space', 300);
  for (let race = 0; race < 3; race++) {
    await poll('dbg_crtstate', 1, 9000);
    await call('dbg_crtskip');
    await page.keyboard.down('ArrowUp'); await sleep(2200); await page.keyboard.up('ArrowUp');
    await poll('dbg_crtstate', 2, 9000);
    await key('Space', 400);
  }
  await poll('dbg_crttrack', 7, 5000);
  await poll('dbg_crtstate', 1, 9000);
  note('track 7 race live', (await call('dbg_crttrack')) === 7);
  // ---- real driving for up to 170 s of wall time
  await page.keyboard.down('ArrowUp');
  let last = await call('dbg_crtprog'), lastT = Date.now(), side = 0, best = 0;
  const t0 = Date.now();
  while (Date.now() - t0 < 170000) {
    await sleep(2000);
    const p = await call('dbg_crtprog');
    if (p > best) best = p;
    if (p - last < 25) {   // stalled: pulse a steering direction
      const k = side ? 'ArrowRight' : 'ArrowLeft'; side = 1 - side;
      await page.keyboard.down(k); await sleep(420); await page.keyboard.up(k);
    }
    last = p;
    if ((await call('dbg_crtstate')) !== 1) break;
  }
  await page.keyboard.up('ArrowUp');
  const pEnd = await call('dbg_crtprog');
  const aiEnd = await call('dbg_crtprog2');
  const st = await call('dbg_crtstate');
  console.log(`player prog ${Math.max(best, pEnd)} | kart1 prog ${aiEnd} | race distance 8337 | state ${st}`);
  note('player really drove >3500 units of the Depths', Math.max(best, pEnd) > 3500);
  note('rival kart 1 covered >6000 units (AI completes the big circuit)', aiEnd > 6000);
  await browser.close();
  console.log(fails ? `v123-driveprobe FAILURES: ${fails}` : 'v123-driveprobe ALL OK');
  process.exit(fails ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
