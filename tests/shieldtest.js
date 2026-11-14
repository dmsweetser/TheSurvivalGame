// Entering the mall expels every bug from the epicenter ring —
// including bugs with nobody to hunt (all players inside).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
const ok = (c, m, x) => { if (c) { pass++; console.log('PASS', m); } else { fail++; console.log('FAIL', m, x !== undefined ? ' [' + x + ']' : ''); } };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (f, ...a) => page.evaluate(([f, a]) => window.__TSG[f](...a), [f, a]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(500);
  await key('Space', 500); await key('Enter', 800);
  ok((await call('dbg_state')) === 1, 'run started');
  // bugs loitering in the plaza, one near the center
  await call('dbg_spawnbug', 60, 40);
  await call('dbg_spawnbug', -100, 80);
  await call('dbg_spawnbug', 10, -120);
  await sleep(300);
  // step into the mall: the shield rises and the plaza must clear
  await call('dbg_enter');
  await sleep(900);
  const nb = await call('dbg_bugs');
  let inside = 0, checked = 0;
  for (let i = 0; i < nb; i++) {
    const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i);
    const d = Math.sqrt(bx * bx + by * by);
    if (Math.abs(bx) < 900 && Math.abs(by) < 900) { checked++; if (d < 155) inside++; }
  }
  ok(checked >= 3, 'spawned bugs are on the field', checked);
  ok(inside === 0, 'all bugs expelled outside the epicenter ring', inside);
  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
