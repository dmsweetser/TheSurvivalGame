// shots132-audit-d.js — v1.32 cosmetic pass #3, part D: BW overworld,
// WAR! skirmish setup, game over, split-screen, mobile settings.
// Usage: node shots132-audit-d.js <tag> <desktop|mobile> <bw|skirmish|over|split|settings>
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
  await page.addInitScript(() => {
    function mkpad(index) {
      const buttons = [];
      for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
      return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    }
    window.__pads = [mkpad(0), mkpad(1)];
    navigator.getGamepads = () => window.__pads;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
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
  const shot = async name => { await sleep(350); await page.screenshot({ path: `${dir}/audit-${tag}-${flavour}-${name}.png` }); console.log('shot', name, 'state', await call('dbg_state'), 'bwmode', await call('dbg_bwmode')); };
  if (mobile) await call('set_mobile', 1);

  if (target === 'split') {
    // co-op on at the title (the splittest pattern), P2 drops in, split apart
    { let prev = await call('dbg_menusel'), bottom = prev, wrapped = false;
      for (let i = 0; i < 10; i++) { await key('ArrowDown', 160); const s = await call('dbg_menusel');
        if (s <= prev) { bottom = prev; wrapped = true; break; } prev = s; }
      if (wrapped) { for (let i = 0; i < bottom - 1; i++) await key('ArrowDown', 160); }
      else { await key('ArrowUp', 160); } }
    await key('Space', 300);
    for (let i = 0; i < 10 && (await call('dbg_menusel')) !== 0; i++) await key('ArrowUp', 160);
    await key('Space', 500);
    if ((await call('dbg_state')) !== 1) { await key('Enter', 500); }
    await sleep(600);
    await page.evaluate(() => window.__btn(1, 0, true));
    await sleep(160);
    await page.evaluate(() => window.__btn(1, 0, false));
    console.log('p2:', await poll('dbg_p2', v => v > 0, 6000));
    await call('dbg_tp', 0, -1500);
    console.log('splitf:', await poll('dbg_splitf', v => v >= 97, 9000));
    await shot('split-stacked');
    await call('dbg_tp', 1500, 0);
    await sleep(1500);
    await shot('split-side');
    await browser.close();
    console.log('part D done split');
    return;
  }

  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(500);
  await call('dbg_adopt', 0);
  await sleep(300);

  if (target === 'over') {
    await call('dbg_tp', 0, 900); await sleep(300);
    await call('dbg_hurt', 9999);
    await sleep(1200);
    await shot('gameover');
  } else if (target === 'settings') {
    await call('dbg_tp', 0, 900); await sleep(300);
    await key('Escape', 350);   // pause
    await key('ArrowDown', 140);   // CONTROLS row
    await key('Space', 450);
    await shot('settings');
  } else if (target === 'bw') {
    await call('dbg_bwent2');
    await sleep(800);
    await page.keyboard.down('ArrowDown'); await sleep(700); await page.keyboard.up('ArrowDown');
    await shot('bw-overworld');
  } else if (target === 'skirmish') {
    await call('dbg_tp', 0, 80); await sleep(200);
    await key('KeyE', 300);
    await call('dbg_tp', 350, -5976); await sleep(250);
    await key('KeyE', 300);
    await sleep(400);
    await call('dbg_tp', 0, -11758); await sleep(250);
    let gate = 0;
    for (let i = 0; i < 4 && gate !== 1; i++) { await key('KeyE', 300); gate = await poll('dbg_arcgate', v => v === 1, 2500); }
    await key('Space', 400);
    await poll('dbg_bwmode', v => v === 8, 5000);
    await sleep(600);
    await shot('war-map');
    await key('KeyS', 350);   // skirmish setup
    await sleep(500);
    await shot('war-skirmish-setup');
  }
  await browser.close();
  console.log('part D done', target, flavour);
})().catch(e => { console.error('ERR', e); process.exit(1); });
