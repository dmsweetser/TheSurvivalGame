// shots132-audit-b.js — v1.32 cosmetic pass #3, part B: the Arcade,
// every cabinet panel + mode, and the Source Room.
// Usage: node shots132-audit-b.js <tag> <desktop|mobile>
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
  const shot = async name => { await sleep(350); await page.screenshot({ path: `${dir}/audit-${tag}-${flavour}-${name}.png` }); console.log('shot', name, 'state', await call('dbg_state'), 'bwmode', await call('dbg_bwmode'), 'arcgate', await call('dbg_arcgate'), 'cab', await call('dbg_cabinet')); };
  if (mobile) await call('set_mobile', 1);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(500);
  await call('dbg_adopt', 0);   // cabinet entry requires a buddy (Daniel's ruling)
  await sleep(300);
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);   // into the mall
  // Source Room first (while in the mall): door at (-210,-6180)
  await call('dbg_givetrophy', 3);
  await call('dbg_tp', -210, -6180); await sleep(250);
  await key('KeyE', 350);
  await shot('srcroom-inside');
  await call('dbg_tp', 0, -18100); await sleep(250);
  await key('KeyE', 350);
  await shot('srcroom-browser');
  await key('Enter', 400);
  await shot('srcroom-editor');
  await key('Escape', 300); await sleep(200);
  await key('Escape', 300); await sleep(300);
  // back to the mall, then the Arcade door (350,-5976)
  await call('dbg_tp', 0, -5810); await sleep(250);
  await key('KeyE', 300);   // exit mat -> meadow (in case we were still in the room/mall)
  await sleep(300);
  if ((await call('dbg_mall')) === 0) { await call('dbg_tp', 0, 80); await sleep(200); await key('KeyE', 300); await sleep(300); }
  await call('dbg_tp', 350, -5976); await sleep(250);
  await key('KeyE', 300);
  await shot('arcade');
  const cabs = [
    ['entrances', -256, -12108], ['run', 0, -12108], ['craft', 256, -12108],
    ['tank', -256, -11898], ['castle', 0, -11898], ['cart', 256, -11898],
    ['war', 0, -11758], ['mordor', -256, -11758],
  ];
  for (const [nm, x, y] of cabs) {
    await call('dbg_tp', x, y); await sleep(250);
    await key('KeyE', 350);
    await shot('cab-' + nm + '-panel');
    await key('Space', 450);   // launch (WAR!/Cart/Run may land on a select screen first)
    await sleep(900);
    await shot('mode-' + nm + '-a');
    await key('Space', 400);   // advance a select screen if one is up
    await sleep(900);
    await shot('mode-' + nm + '-b');
    // leave the mode: Escape (some modes need it twice: pause -> quit)
    await key('Escape', 350); await sleep(300);
    await key('Escape', 350); await sleep(400);
    if ((await call('dbg_bwmode')) > 0) { await key('ArrowDown', 130); await key('Enter', 350); await sleep(400); }
    if ((await call('dbg_bwmode')) > 0) { await key('Escape', 350); await sleep(300); await key('ArrowDown', 130); await key('ArrowDown', 130); await key('Enter', 350); await sleep(400); }
    console.log('after mode', nm, 'bwmode', await call('dbg_bwmode'), 'inarcade', await call('dbg_inarcade'));
    if ((await call('dbg_inarcade')) !== 1) { console.log('  NOT back in arcade — subsequent cabinet shots may be off'); }
  }
  await browser.close();
  console.log('part B done', tag, flavour);
})().catch(e => { console.error('ERR', e); process.exit(1); });
