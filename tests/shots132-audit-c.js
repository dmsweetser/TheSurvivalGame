// shots132-audit-c.js — v1.32 cosmetic pass #3, part C: the surfaces
// part B's heuristics missed (Run, Tank, Cart, Mordor, Photo Booth).
// One boot per target; the cabinet gate is polled with E retries (the
// buddy has to trot to the cabinet after a teleport).
// Usage: node shots132-audit-c.js <tag> <desktop|mobile> <run|tank|cart|mordor|booth>
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const tag = process.argv[2] || 'x';
  const flavour = process.argv[3] || 'desktop';
  const target = process.argv[4];
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
  const poll = async (fn, want, timeout = 5000, ...args) => {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(150); }
    return call(fn, ...args);
  };
  const shot = async name => { await sleep(350); await page.screenshot({ path: `${dir}/audit-${tag}-${flavour}-${name}.png` }); console.log('shot', name, 'state', await call('dbg_state'), 'bwmode', await call('dbg_bwmode'), 'arcgate', await call('dbg_arcgate'), 'station', await call('dbg_station'), 'portstage', await call('dbg_portstage')); };
  if (mobile) await call('set_mobile', 1);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(500);
  await call('dbg_adopt', 0);
  await sleep(300);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);   // mall
  if (target === 'booth') {
    await call('dbg_tp', -232, -5980); await sleep(300);
    await page.keyboard.down('ArrowUp'); await sleep(250); await page.keyboard.up('ArrowUp');   // face the fixture
    for (let i = 0; i < 3 && (await call('dbg_portstage')) !== 5; i++) { await key('KeyE', 350); await sleep(400); }
    await shot('station-booth');
    await browser.close();
    console.log('part C done', target);
    return;
  }
  await call('dbg_tp', 350, -5976); await sleep(250);
  await key('KeyE', 300);   // arcade
  await sleep(400);
  const cabs = { run: [0, -12108, 3], tank: [-256, -11898, 5], cart: [256, -11898, 7], mordor: [-256, -11758, 9] };
  const [x, y, mode] = cabs[target];
  await call('dbg_tp', x, y); await sleep(250);
  let gate = 0;
  for (let i = 0; i < 4 && gate !== 1; i++) { await key('KeyE', 300); gate = await poll('dbg_arcgate', v => v === 1, 2500); }
  console.log('gate for', target, '=', gate);
  await shot('cab-' + target + '-panel');
  await key('Space', 400);
  const bw = await poll('dbg_bwmode', v => v === mode, 5000);
  console.log('bwmode for', target, '=', bw);
  await sleep(800);
  await shot('mode-' + target + '-a');
  if (target === 'cart') {
    await key('Space', 400);   // waiting room -> countdown -> race
    await sleep(3500);
    await shot('mode-cart-b');
  }
  if (target === 'run') {
    await page.keyboard.down('ArrowRight'); await sleep(900); await page.keyboard.up('ArrowRight');
    await shot('mode-run-b');
  }
  await browser.close();
  console.log('part C done', target);
})().catch(e => { console.error('ERR', e); process.exit(1); });
