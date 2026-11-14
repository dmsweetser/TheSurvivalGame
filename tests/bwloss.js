// bwloss.js — does a Banzai World visit (incl. sleeping there, which
// saves mid-visit) cost you your buddy across a save + continue?
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let fails = [];
const T = (name, ok, info) => { console.log((ok ? 'PASS' : 'FAIL') + ' | ' + name + (ok ? '' : ' | ' + info)); if (!ok) fails.push(name); };
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
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0);
  // into Banzai World, sleep at a tent (saves mid-visit), then leave
  await call('dbg_bwenter');
  // BW is per-player now (in_bw), G.state stays GS_PLAY — the mode
  // readout is the honest entry check
  T('in Banzai World', (await call('dbg_bwmode')) === 0, await call('dbg_bwmode'));
  await call('dbg_bwtele_tent');
  await sleep(300);
  await key('KeyE', 300);            // sleep
  await sleep(2200);                 // fade + save completes
  T('still in BW after sleeping', (await call('dbg_bwmode')) === 0, await call('dbg_bwmode'));
  await call('dbg_bwexit');
  T('back from BW', (await call('dbg_state')) === 1, await call('dbg_state'));
  T('buddy present after BW trip', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  await sleep(6500);                 // autosave in PLAY
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  T('save carries a buddy token (bd=)', / bd=\d/.test(raw));
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(600);
  await key('Space', 500);           // CONTINUE -> picker
  await key('Space', 600);           // load slot 1
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(400);
  T('BUDDY SURVIVED the BW trip + save + continue', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  console.log(fails.length ? 'BWLOSS FAILURES: ' + fails.join(', ') : 'BWLOSS ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
