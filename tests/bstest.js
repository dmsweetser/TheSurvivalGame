// bstest.js — unlock-code entry ergonomics (Daniel, Nov 3): a backspace
// TAP deletes one character; HOLDING backspace keeps deleting.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = [];
const T = (name, ok, info) => { console.log((ok ? 'PASS' : 'FAIL') + ' | ' + name + (ok ? '' : ' | ' + info)); if (!ok) fails.push(name); };
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const walkTo = async (tx, ty, tol = 10) => {
    for (let i = 0; i < 90; i++) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = tx - x, dy = ty - y;
      if (Math.abs(dx) <= tol && Math.abs(dy) <= tol) return true;
      const downs = [];
      if (dx < -6) downs.push('KeyA'); else if (dx > 6) downs.push('KeyD');
      if (dy < -6) downs.push('KeyW'); else if (dy > 6) downs.push('KeyS');
      for (const d of downs) await page.keyboard.down(d);
      await sleep(110);
      for (const d of downs) await page.keyboard.up(d);
    }
    return false;
  };

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('entered the mall', (await call('dbg_mall')) === 1);
  T('walked to the Whales Only counter', await walkTo(-75, -6108, 14));
  await key('KeyE', 350);
  T('whales panel open', (await call('dbg_station')) === 1);
  for (let i = 0; i < 15; i++) await key('ArrowDown', 110);
  await key('Space', 400);   // code-entry mode
  T('code entry starts empty', (await call('dbg_codelen')) === 0, await call('dbg_codelen'));
  await page.keyboard.type('ABCDEF', { delay: 60 });
  await sleep(200);
  T('typed 6 chars', (await call('dbg_codelen')) === 6, await call('dbg_codelen'));
  await key('Backspace', 90, 120);
  T('a backspace tap deletes exactly one', (await call('dbg_codelen')) === 5, await call('dbg_codelen'));
  await page.keyboard.down('Backspace');
  for (let i = 0; i < 7; i++){ await sleep(200); console.log('  hold poll', i, 'codelen =', await call('dbg_codelen'), 'state =', await call('dbg_state'), 'station =', await call('dbg_station')); }
  await page.keyboard.up('Backspace');
  await sleep(120);
  T('HOLDING backspace deletes the rest', (await call('dbg_codelen')) === 0, await call('dbg_codelen'));
  await key('Escape', 300);
  console.log(fails.length ? 'BSTEST FAILURES: ' + fails.join(', ') : 'BSTEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
