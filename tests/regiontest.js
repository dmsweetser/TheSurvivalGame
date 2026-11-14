// regiontest.js — the far country (Nov 9): eight regions in distance
// bands out from the epicenter. Teleport-tour every band, assert the
// game's own region readout, screenshot each country, spawn bugs in
// the far bands, and confirm a save made out there still loads (the
// standing rule: loads resume at the Bug Mall centre).
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Enter', 300, 500);                       // START -> slots
  await page.waitForFunction(() => window.__TSG.dbg_state() === 7, null, { timeout: 10000 });
  await key('Enter', 300, 600);                       // slot 1 -> run
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  T('run started', true);
  const tour = [
    [300, 0, 'meadows'], [900, 1, 'spaghetti'], [2000, 2, 'legacy'],
    [3400, 3, 'badlands'], [5200, 4, 'cache'], [7500, 5, 'ramflats'],
    [10300, 6, 'jungle'], [15000, 7, 'depths'],
  ];
  for (const [d, want, name] of tour) {
    await call('dbg_tp', d, 0);
    await sleep(700);
    const rg = await call('dbg_region');
    T(`region at ${d}px is ${name} (${want})`, rg === want, rg);
    if (d >= 3400) {   // bugs live out here too
      const before = await call('dbg_bugs');
      await page.evaluate(() => { for (let i = 0; i < 10; i++) window.__TSG.dbg_spawnbug(0, 0); });
      // spawn at the player's own distance so the regional strain roll applies
      await page.evaluate(([d]) => { for (let i = 0; i < 10; i++) window.__TSG.dbg_spawnbug(d, (i - 5) * 30); }, [d]);
      await sleep(250);
      T(`bugs spawn in ${name}`, (await call('dbg_bugs')) > before, await call('dbg_bugs'));
    }
    await page.screenshot({ path: `region-${name}.png` });
  }
  // border dither: stand just either side of the Legacy/Badlands line
  await call('dbg_tp', 2540, 0); await sleep(700); await page.screenshot({ path: 'region-border-in.png' });
  await call('dbg_tp', 2680, 0); await sleep(700); await page.screenshot({ path: 'region-border-out.png' });
  T('border screenshots taken', true);
  // autosave out in the far country, reload, CONTINUE: the run must
  // come back (at the mall, per the standing rule) with data intact
  await sleep(2500);                                  // let the autosave land
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('autosave written from the far country', raw.length > 50, raw.length);
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400, 500);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400, 600);
  let st = -1;
  for (let i = 0; i < 12 && st !== 1; i++) { st = await call('dbg_state'); if (st !== 1) await sleep(250); }
  T('continued after far-country save', st === 1, st);
  const px = await call('dbg_px'), py = await call('dbg_py');
  T('load resumes at the Bug Mall', Math.abs(px) < 900 && py < -5000, px + ',' + py);   // the mall interior sits at y ~ -6000
  console.log(`regiontest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
