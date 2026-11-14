// xverprobe.js — does the CURRENT loader keep the buddy when the slot
// blob was written by an OLDER version? Manufactures old-format saves
// by stripping the tokens each era didn't have yet, then continues.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await sleep(7000);
  const slot = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  const glob = await page.evaluate(() => localStorage.getItem('tsg_save') || '');
  console.log('slot blob:', slot.replace(/\n/g, '\\n').slice(0, 900));
  const eras = {
    'control (current format)': s => s,
    'minus sb (Entrances-era)': s => s.replace(/ sb=\d+/, ''),
    'minus er+sb (Nov 4 era)': s => s.replace(/ er=\d+/, '').replace(/ sb=\d+/, ''),
    'minus sq2+si+er+sb': s => s.replace(/ sq2=\d+/, '').replace(/ si=\d+/, '').replace(/ er=\d+/, '').replace(/ sb=\d+/, ''),
    'minus bnv (legacy names)': s => s.replace(/ bnv=\d+/, ''),
    'minus all late tokens': s => s.replace(/ (bnv|sm|sq2?|er|sb|si)=\d+/g, ''),
  };
  for (const [name, fn] of Object.entries(eras)) {
    const mutated = fn(slot);
    await page.evaluate(([g, s]) => { localStorage.clear(); localStorage.setItem('tsg_save', g); localStorage.setItem('tsg_save1', s); }, [glob, mutated]);
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await sleep(500);
    await key('Space', 500);
    const st = await call('dbg_state');
    if (st !== 7) { console.log(name + ': picker did not open (state ' + st + ')'); continue; }
    await key('Space', 600);
    await sleep(400);
    const bud = await call('dbg_buddy');
    console.log(name + ': buddy after continue = ' + bud + (bud !== 0 ? ' OK' : ' *** LOST ***'));
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
