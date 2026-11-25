// shots132-hud.js — v1.32 Item 1: main-game HUD screenshots with all
// furniture live, for the pause-chip placement decision. Usage:
//   node shots132-hud.js <tag>   (tag = before | after)
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const tag = process.argv[2] || 'before';
  const dir = __dirname + '/shots132';
  fs.mkdirSync(dir, { recursive: true });
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  await call('set_mobile', 1);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await sleep(500);
  // dress the HUD: hat weight, beans, a long TIME line, threat
  await call('dbg_tp', 0, 900); await sleep(300);
  await call('dbg_feed'); await call('dbg_feed'); await call('dbg_feed');
  await call('dbg_givebean'); await call('dbg_givebean');
  await call('dbg_setkills', 1234);
  await call('dbg_setthreat', 30);
  await sleep(400);
  await call('dbg_touchctl', 1);   // STICK
  await sleep(300);
  await page.screenshot({ path: `${dir}/hud-${tag}-stick.png` });
  await call('dbg_touchctl', 0);   // DRAG
  await sleep(300);
  await page.screenshot({ path: `${dir}/hud-${tag}-drag.png` });
  // desktop flavour (no chip — keyboard): context for the same HUD
  await call('set_mobile', 0);
  await page.setViewportSize({ width: 1280, height: 720 });
  await sleep(400);
  await page.screenshot({ path: `${dir}/hud-${tag}-desktop.png` });
  console.log('shots written with tag', tag);
  await browser.close();
})().catch(e => { console.error('ERR', e); process.exit(1); });
