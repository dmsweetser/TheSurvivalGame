// Visual check: the Whales Only counter, panel, and code-entry view.
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const { codes } = JSON.parse(fs.readFileSync(__dirname + '/whale-codes.json', 'utf8'));
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  const walkTo = async (tx, ty, tol) => {
    tol = tol || 12;
    for (let i = 0; i < 60; i++) {
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
  };
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  // own everything so the effects show in the world shot
  for (const c of codes) await page.evaluate(code => {
    const ptr = window.__TSG.dbg_scratch();
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    for (let i = 0; i < code.length; i++) mem[ptr + i] = code.charCodeAt(i);
    mem[ptr + code.length] = 0;
    return window.__TSG.dbg_redeem();
  }, c);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  await walkTo(-75, -6108, 14);
  await page.screenshot({ path: __dirname + '/whale-mall.png' });       // the counter + tuxedo keeper
  await key('KeyE', 350);
  await sleep(400);
  await page.screenshot({ path: __dirname + '/whale-panel.png' });      // the price list
  for (let i = 0; i < 7; i++) await key('ArrowDown', 130);
  await key('Space', 400);
  await sleep(300);
  await page.screenshot({ path: __dirname + '/whale-code.png' });       // code entry view
  await key('Escape', 300);
  // back outside with a hat on, to show the golden carcasses + cape + glasses
  await call('dbg_tp', 0, 300); await sleep(200);
  for (let i = 0; i < 4; i++) await call('dbg_feed');
  await sleep(400);
  await page.screenshot({ path: __dirname + '/whale-world.png' });
  console.log('shots done');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
