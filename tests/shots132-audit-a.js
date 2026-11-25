// shots132-audit-a.js — v1.32 cosmetic pass #3, part A: title/slots,
// HOW TO PLAY, Settings, pause, mall interior, every station panel.
// Usage: node shots132-audit-a.js <tag> <desktop|mobile>
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const tag = process.argv[2] || 'x';
  const flavour = process.argv[3] || 'desktop';
  const dir = __dirname + '/shots132';
  fs.mkdirSync(dir, { recursive: true });
  const mobile = flavour === 'mobile';
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const ctx = await browser.newContext({ viewport: mobile ? { width: 1000, height: 640 } : { width: 1280, height: 720 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  const shot = async name => { await sleep(350); await page.screenshot({ path: `${dir}/audit-${tag}-${flavour}-${name}.png` }); console.log('shot', name, 'state', await call('dbg_state'), 'station', await call('dbg_station'), 'mall', await call('dbg_mall'), 'sel', await call('dbg_menusel')); };
  if (mobile) await call('set_mobile', 1);
  await sleep(600);
  await shot('title');
  // HOW TO PLAY = title row 1
  await key('ArrowDown', 130); await key('Space', 400);
  await shot('howtoplay');
  await key('ArrowDown', 200); await shot('howtoplay-scrolled');
  await key('Escape', 300); await sleep(300);
  // SETTINGS = title row 2 (selection returns to row 1 after HTP; step by seam)
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 2) break; await key('ArrowDown', 120); }
  await key('Space', 400);
  await shot('settings');
  await key('Escape', 300); await sleep(300);
  // start the run (row 0)
  for (let i = 0; i < 10; i++) { if ((await call('dbg_menusel')) === 0) break; await key('ArrowUp', 120); }
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(500);
  await call('dbg_tp', 0, 900); await sleep(300);
  await key('Escape', 350);
  await shot('pause');
  await key('Escape', 300); await sleep(300);
  // mall interior + stations
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  await shot('mall');
  const stations = [
    ['shop', -315, -6148], ['whales', -105, -6148], ['bank', 105, -6148], ['buddies', 315, -6148],
    ['bowl', -180, -5868], ['trophy', 340, -5852], ['challenge', -330, -5852], ['bounty', -352, -5992], ['booth', -232, -5980],
  ];
  for (const [nm, x, y] of stations) {
    await call('dbg_tp', x, y); await sleep(250);
    await key('KeyE', 350);
    await shot('station-' + nm);
    await key('Escape', 300);
    await sleep(200);
    if ((await call('dbg_state')) === 2) { await key('Escape', 300); await sleep(200); }
  }
  await browser.close();
  console.log('part A done', tag, flavour);
})().catch(e => { console.error('ERR', e); process.exit(1); });
