// glowshot.js — before/after captures for the Nov 5 cosmetic glow-up.
// Usage: node glowshot.js <tag>   (writes glow-<tag>-*.png)
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
const tag = process.argv[2] || 'after';
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  async function poll(fn, want, timeout = 9000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(1600);
  await page.screenshot({ path: __dirname + '/glow-' + tag + '-title.png' });

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(900);
  await page.screenshot({ path: __dirname + '/glow-' + tag + '-plaza.png' });

  // the mall: west counters at shopping distance (keepers!)
  await call('dbg_tp', 0, 80); await key('KeyE', 400);
  await call('dbg_tp', -225, -6118); await sleep(700);
  await page.screenshot({ path: __dirname + '/glow-' + tag + '-keepers.png' });

  // Banzai World: adopt, free play, the east shore, then a battle
  await call('dbg_adopt', 0);
  await call('dbg_bwenter');
  await poll('dbg_bwstate', v => v === 1);
  await call('dbg_bwtp', 6520, 300); await sleep(800);
  await page.screenshot({ path: __dirname + '/glow-' + tag + '-shore.png' });
  await call('dbg_bwtp', -300, 200); await sleep(300);
  await call('dbg_bwhackhere');
  await poll('dbg_bwstate', v => v === 2, 6000);
  await sleep(700);
  await page.screenshot({ path: __dirname + '/glow-' + tag + '-battle.png' });
  await browser.close();
  console.log('glow shots done:', tag);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
