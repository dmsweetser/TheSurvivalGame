// shots132-item4.js — Item 4 (Mordor cabinet contrast): the cabinet in
// the arcade + its gate panel, desktop + mobile.
// Usage: node shots132-item4.js <tag>
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const tag = process.argv[2] || 'x';
  const dir = __dirname + '/shots132';
  fs.mkdirSync(dir, { recursive: true });
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  for (const flavour of ['desktop', 'mobile']) {
    const mobile = flavour === 'mobile';
    const ctx = await browser.newContext({ viewport: mobile ? { width: 1000, height: 640 } : { width: 1280, height: 720 } });
    const page = await ctx.newPage();
    page.on('pageerror', e => console.log('PAGEERROR', e.message));
    await page.goto('http://localhost:8903/the-survival-game.html');
    await page.evaluate(() => localStorage.clear());
    await page.reload({ waitUntil: 'load' });
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
    const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
    const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
    const poll = async (fn, want, timeout = 6000, ...args) => {
      const t0 = Date.now();
      while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(150); }
      return call(fn, ...args);
    };
    if (mobile) await call('set_mobile', 1);
    await key('Space', 400);
    if ((await call('dbg_state')) !== 1) await key('Enter', 400);
    await sleep(500);
    await call('dbg_adopt', 0);
    await sleep(300);
    await call('dbg_tp', 0, 80); await sleep(200);
    await key('KeyE', 300);
    await call('dbg_tp', 350, -5976); await sleep(250);
    await key('KeyE', 300);
    await sleep(500);
    // stand in front of the Mordor cabinet (bottom row, west end)
    await call('dbg_tp', -256, -11680); await sleep(600);
    await page.screenshot({ path: `${dir}/item4-${tag}-${flavour}-cabinet.png` });
    console.log('shot cabinet', flavour, 'arcgate', await call('dbg_arcgate'));
    // open the gate panel
    await call('dbg_tp', -256, -11758); await sleep(250);
    let gate = 0;
    for (let i = 0; i < 4 && gate !== 1; i++) { await key('KeyE', 300); gate = await poll('dbg_arcgate', v => v === 1, 2500); }
    await sleep(300);
    await page.screenshot({ path: `${dir}/item4-${tag}-${flavour}-panel.png` });
    console.log('shot panel', flavour, 'arcgate', gate);
    await ctx.close();
  }
  await browser.close();
  console.log('item4 done', tag);
})().catch(e => { console.error('ERR', e); process.exit(1); });
