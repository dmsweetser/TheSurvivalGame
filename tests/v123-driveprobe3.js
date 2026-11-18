// v123-driveprobe3.js — player completability on track 7 with TAP
// steering: gas held (ArrowUp = sel_held in cart mode), steering only
// in 120 ms taps when progress stalls, direction alternates after two
// failed taps. Success = one full lap of the Depths (prog >= 2779).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = 0;
const note = (m, ok) => { if (ok === false) fails++; console.log((ok === false ? 'FAIL ' : 'OK   ') + m); };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
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
  await call('dbg_tp', 0, 80); await key('KeyE', 600); await sleep(400);
  await call('dbg_tp', 230, -5976); await key('KeyE', 600);
  await poll('dbg_inarcade', 1, 5000);
  await call('dbg_tp', 256, -11898); await key('KeyE', 800);
  await poll('dbg_bwmode', 7, 5000);
  await key('Space', 300);
  await poll('dbg_crtstate', 3, 5000);
  await key('ArrowDown', 300);
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
  await page.keyboard.down('ArrowUp');
  let dir = 'ArrowLeft', failsInDir = 0, last = await call('dbg_crtprog'), best = last;
  const t0 = Date.now();
  while (Date.now() - t0 < 300000) {
    await sleep(300);
    const p = await call('dbg_crtprog');
    if (p > best) best = p;
    if (p - last < 3) {                    // stalled: tap steer
      await page.keyboard.down(dir); await sleep(120); await page.keyboard.up(dir);
      if (++failsInDir >= 2) { dir = (dir === 'ArrowLeft') ? 'ArrowRight' : 'ArrowLeft'; failsInDir = 0; }
    } else failsInDir = 0;
    last = p;
    if ((await call('dbg_crtstate')) !== 1) break;
    if (best >= 8337) break;
  }
  await page.keyboard.up('ArrowUp').catch(() => {});
  const st = await call('dbg_crtstate');
  console.log(`player best prog ${best} | lap 2779 | race 8337 | state ${st} | wall ${Math.round((Date.now() - t0) / 1000)}s`);
  note('player lapped the Depths by driving', best >= 2779 || st === 2);
  await browser.close();
  console.log(fails ? `v123-driveprobe3 FAILURES: ${fails}` : 'v123-driveprobe3 ALL OK');
  process.exit(fails ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
