// v123-cartprobe.js — v1.23 cart progression check: play both Grand Prix
// cups with dbg_crtskip finishes, measuring the TRUE in-game CRT_LEN of
// every track from the skip return value (prog parked at LAPS*LEN - 130,
// so LEN = (ret + 130) / 3 while laps stay 3), and screenshot an early
// (track 0) and a late (track 7) race. Tag via V123_TAG=before|after.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = process.env.TSG_URL || 'http://localhost:8903/the-survival-game.html';
const TAG = process.env.V123_TAG || 'before';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = 0;
function note(name, cond, extra) { console.log((cond ? 'OK  ' : 'FAIL') + ' ' + name, extra !== undefined ? '-> ' + extra : ''); if (!cond) fails++; }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 12000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  let cv = null;
  async function freshBoot() {
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
    note('run started', (await call('dbg_state')) === 1);
    await call('dbg_adopt', 0);
    cv = await page.$('canvas');
  }

  async function enterCart(cup) {
    await call('dbg_tp', 0, 80); await sleep(200);
    await key('KeyE', 300);
    await poll('dbg_mall', v => v === 1);
    await call('dbg_tp', 230, -5976); await sleep(200);
    await key('KeyE', 250);
    await poll('dbg_inarcade', v => v === 1, 4000);
    await call('dbg_tp', 256, -11898); await sleep(200);
    await key('KeyE', 250);
    await poll('dbg_arcgate', v => v === 1, 3000);
    await key('Space', 300);
    await poll('dbg_bwmode', v => v === 7, 4000);
    await poll('dbg_crtstate', v => v === 3, 4000);
    if (cup === 1) { await key('ArrowDown', 250); note('far cup selected', (await call('dbg_crtcup')) === 1, await call('dbg_crtcup')); }
    await key('Space', 300);   // FIRE: begin the cup
    await poll('dbg_crtstate', v => v === 0, 6000);
  }

  const lens = {};
  async function runCup(cup) {
    await freshBoot();
    await enterCart(cup);
    for (let r = 0; r < 4; r++) {
      const track = cup * 4 + r;
      note('race on track ' + track, (await poll('dbg_crttrack', v => v === track, 6000)) === track, await call('dbg_crttrack'));
      await poll('dbg_crtstate', v => v === 1, 9000);
      if (track === 0 || track === 7) {
        // drive a little for the screenshot: gas on, gentle steer to stay near the line
        await page.keyboard.down('Space'); await sleep(1600); await page.keyboard.up('Space');
        await cv.screenshot({ path: __dirname + '/tsg-v123-' + TAG + (track === 0 ? '-cartearly.png' : '-cartlate.png') });
        console.log('screenshot saved for track', track);
      }
      const ret = await call('dbg_crtskip');
      const len = (ret + 130) / 3;
      lens[track] = len;
      console.log('track', track, 'skipret', ret, 'CRT_LEN', len.toFixed(1), 'race distance', (3 * len).toFixed(0));
      await hold('Space', 1700);
      const st = await poll('dbg_crtstate', v => v === 2 || v === 4 || v === 5, 9000);
      note('track ' + track + ' race closed', st === 2 || st === 4 || st === 5, 'state=' + st);
      if (r < 3) {
        await poll('dbg_crtstate', v => v === 4, 9000);
        await key('Space', 300);
        await poll('dbg_crtstate', v => v === 0, 6000);
      } else {
        await poll('dbg_crtstate', v => v === 5, 9000);
        await poll('dbg_bwmode', v => v === -1, 14000);
        note('cup ' + cup + ' exited', (await call('dbg_bwmode')) === -1);
      }
    }
  }
  if (process.env.V123_ONLY !== 'far') await runCup(0);
  await runCup(1);
  console.log('LENS ' + JSON.stringify(lens));
  const base = 3 * (lens[0] || 0);
  for (let t = 0; t < 8; t++) if (lens[t]) console.log('track', t, 'LEN', lens[t].toFixed(1), 'race x' + (3 * lens[t] / base).toFixed(3));
  console.log('v123-cartprobe ' + (fails ? 'FAILURES: ' + fails : 'ALL OK'));
  await browser.close();
  process.exit(fails ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
