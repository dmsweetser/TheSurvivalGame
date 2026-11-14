const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  const CY = -6000, DX = 185, DY = CY + 92;
  await call('dbg_tp', 0, 80); await sleep(200); await key('KeyE', 300); await sleep(300);
  console.log('in mall:', await call('dbg_mall'));
  const dist = async () => { const x = await call('dbg_px'), y = await call('dbg_py'); return Math.hypot(x - DX, y - DY).toFixed(1) + ' (x' + Math.round(x) + ' y' + Math.round(y) + ')'; };
  const tries = [
    ['WEST at door latitude', 60, DY, 'KeyD'],
    ['WEST at mat row', 60, CY + 118, 'KeyD'],
    ['SOUTH in door column', DX, CY + 175, 'KeyW'],
    ['NORTH of the gate', DX, CY - 60, 'KeyS'],
    ['EAST of the gate', 300, DY, 'KeyA'],
  ];
  for (const [name, sx, sy, k] of tries) {
    await call('dbg_tp', sx, sy); await sleep(200);
    await page.keyboard.down(k); await sleep(1500); await page.keyboard.up(k); await sleep(150);
    const d = await dist();
    await key('KeyE', 250); await sleep(350);
    const opened = (await call('dbg_gate')) === 1;
    console.log(name.padEnd(24), 'stop:', d, '| panel:', opened);
    if (opened) {   // close it the player's other way: navigate to LEAVE and pick it
      await key('ArrowDown', 200); await sleep(150); await key('ArrowDown', 200); await sleep(150);
      await key('Space', 250); await sleep(300);
      console.log(' '.padEnd(24), 'panel closed via LEAVE:', (await call('dbg_gate')) === 0);
    }
    if ((await call('dbg_bwstate')) === 1) { await key('Escape', 250); await sleep(350); }
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
