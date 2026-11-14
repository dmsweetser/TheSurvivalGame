// tankdiag.js — diagnostic: park south of the boss gate, hold fire,
// sample boss hp / hull / lives / position every 2s for 60s.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_arcade'); await sleep(300);
  await call('dbg_tp', 219, -12108); await sleep(200);
  await key('KeyE', 250); await key('Space', 300);
  await poll('dbg_bwmode', v => v === 5, 4000);
  await call('dbg_tankquota');
  await call('dbg_tankwarp', 2350); await hold('ArrowUp', 400);
  await page.keyboard.down('Space');
  await poll('dbg_tankmid', v => v === 0, 60000);
  await page.keyboard.up('Space');
  console.log('blockade down, obj=' + (await call('dbg_tankobj')) + ' boss=' + (await call('dbg_tankboss')));
  await call('dbg_tankwarp', 500);
  await hold('ArrowUp', 300);
  await page.keyboard.down('Space');
  for (let i = 0; i < 30; i++) {
    await sleep(2000);
    const y = await call('dbg_tanky');
    if (y > 900) { await call('dbg_tankwarp', 500); await hold('ArrowUp', 200); }
    console.log('t=' + (i * 2 + 2) + 's mode=' + (await call('dbg_bwmode')) + ' boss=' + (await call('dbg_tankboss')) +
      ' hull=' + (await call('dbg_tankhull')) + ' lives=' + (await call('dbg_tanklives')) + ' y=' + y +
      ' shells=' + (await call('dbg_tankshells')) + ' obj=' + (await call('dbg_tankobj')));
  }
  await page.keyboard.up('Space');
  await browser.close();
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
