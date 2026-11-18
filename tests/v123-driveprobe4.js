// v123-driveprobe4.js — OBSERVE the blind-drive stall on track 7:
// screenshot every 15 s so a human can see where the kart ends up.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
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
  const cv = await page.$('canvas');
  await page.keyboard.down('ArrowUp');
  let dir = 'ArrowLeft', failsInDir = 0, last = await call('dbg_crtprog'), best = last;
  const t0 = Date.now(); let shot = 0;
  while (Date.now() - t0 < 120000) {
    await sleep(300);
    const p = await call('dbg_crtprog');
    if (p > best) best = p;
    if (p - last < 3) {
      await page.keyboard.down(dir); await sleep(120); await page.keyboard.up(dir);
      if (++failsInDir >= 2) { dir = (dir === 'ArrowLeft') ? 'ArrowRight' : 'ArrowLeft'; failsInDir = 0; }
    } else failsInDir = 0;
    last = p;
    if (Date.now() - t0 > (shot + 1) * 15000) {
      shot++;
      await cv.screenshot({ path: `tsg-v123-drive-obs${shot}.png` });
      console.log(`t=${shot * 15}s prog=${p} best=${best} pos=${await call('dbg_crtpos')}`);
    }
    if ((await call('dbg_crtstate')) !== 1) break;
  }
  await page.keyboard.up('ArrowUp').catch(() => {});
  console.log(`final best=${best} state=${await call('dbg_crtstate')}`);
  await browser.close();
  process.exit(0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
