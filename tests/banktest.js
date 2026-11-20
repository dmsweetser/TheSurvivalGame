// banktest.js — the Bug Bank moves carcasses in fractions now:
// deposit all/half/quarter and withdraw all/half/quarter (rows 0-5).
// Feed carcasses are a uniform 2 kg, so every fraction is exact.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }
  const bank = () => call('dbg_bank');          // whole kg
  const carried = () => call('dbg_carried');    // tenths of a kg
  async function bankRow(row) {
    await key('KeyE', 350);
    ok((await call('dbg_state')) === 2 && (await call('dbg_station')) === 2, 'bank panel open');
    for (let i = 0; i < row; i++) await key('ArrowDown', 130);
    await key('Space', 400);
    await key('Escape', 300);
  }

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  ok(await walkTo(105, -6148, 14), 'walked to the bank counter');

  for (let i = 0; i < 4; i++) await call('dbg_feed');   // 4 x 10 kg = 40 kg
  ok((await carried()) === 400, 'carrying 40 kg', await carried());

  await bankRow(0);   // DEPOSIT ALL
  ok((await bank()) === 40 && (await carried()) === 0, 'deposit all: vault 40, head 0', `${await bank()}/${await carried()}`);

  await bankRow(5);   // WITHDRAW QUARTER of 40 = 10
  ok((await bank()) === 30 && (await carried()) === 100, 'withdraw quarter: vault 30, head 10', `${await bank()}/${await carried()}`);

  await call('dbg_feed'); await call('dbg_feed');       // head 10 + 20 = 30
  await bankRow(1);   // DEPOSIT HALF of 30 = 15 -> pops 8 x 2kg = 16
  ok((await bank()) === 46 && (await carried()) === 140, 'deposit half: vault 46, head 14', `${await bank()}/${await carried()}`);

  await bankRow(4);   // WITHDRAW HALF of 46 = 23
  ok((await bank()) === 23 && (await carried()) === 370, 'withdraw half: vault 23, head 37', `${await bank()}/${await carried()}`);

  await bankRow(2);   // DEPOSIT QUARTER of 37 = 9.25 -> pops 5kg + 6kg chunks = 11
  ok((await bank()) === 34 && (await carried()) === 260, 'deposit quarter: vault 34, head 26', `${await bank()}/${await carried()}`);

  await bankRow(3);   // WITHDRAW ALL
  ok((await bank()) === 0 && (await carried()) === 600, 'withdraw all: vault 0, head 60', `${await bank()}/${await carried()}`);

  const cv = await page.$('canvas');
  await key('KeyE', 350);
  await cv.screenshot({ path: __dirname + '/bank-panel.png' });
  await key('Escape', 300);

  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
