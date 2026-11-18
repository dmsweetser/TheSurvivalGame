// v122shots.js — before/after scene captures for the v1.22 cosmetic
// pass. Usage: node v122shots.js before|after. Scenes: title,
// overworld (spawn plaza), Bug Mall (north end), CASTLE courtyard.
// Deterministic navigation via the same dbg hooks the suites use.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
const tag = process.argv[2] || 'before';

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const boot = async () => {
    await page.goto(URL);
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  };
  const shot = async name => { await page.screenshot({ path: __dirname + '/tsg-v122-' + tag + '-' + name + '.png' }); console.log('shot', name); };

  // title
  await boot(); await sleep(1400); await shot('title');

  // overworld: start a run, adopt, stand at the plaza
  await boot();
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0); await sleep(900); await shot('overworld');

  // Bug Mall, north end (stations row)
  await call('dbg_enter'); await call('dbg_tp', -75, -6060); await sleep(800); await shot('mall');

  // CASTLE cabinet: mall -> arcade -> cabinet 4 -> launch
  await call('dbg_tp', 230, -5976); await sleep(200); await key('KeyE', 250); await sleep(600);
  await call('dbg_tp', 0, -11898); await sleep(250); await key('KeyE', 250); await sleep(400);
  await key('Space', 300); await sleep(900); await shot('castle');

  await browser.close(); console.log('done', tag);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
