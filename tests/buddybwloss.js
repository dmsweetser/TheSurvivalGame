// buddybwloss.js — Daniel's lost buddy (Nov 3): adopt, dive straight into
// Banzai World, sleep at a tent (which fires a save), then reload WITHOUT
// leaving BW. The slot must carry the buddy (bd= token) and the buddy
// must survive the reload. On the old build the tent save skipped the
// run section entirely and the buddy vanished.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 160) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const slot = () => page.evaluate(() => localStorage.getItem('tsg_save1') || '');
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1);

  // adopt and dive into Banzai World immediately — no field autosave first
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));
  await call('dbg_bwenter');
  T('inside Banzai World', (await call('dbg_bwstate')) === 1);
  const before = await slot();
  T('no buddy in the slot yet (nothing has saved since adoption)', !/ bd=\d/.test(before), before.slice(0, 80));

  // sleep at a tent: the sleep completion fires do_save from inside BW
  await call('dbg_bwtele_tent');
  await sleep(250);
  await key('KeyE', 150);
  await sleep(2600);   // sleep runs 1.4s; the save fires as it completes
  const after = await slot();
  T('the tent save wrote the buddy into the slot', / bd=\d/.test(after), after.slice(0, 120));
  T('still in Banzai World after the sleep', (await call('dbg_bwstate')) === 1);

  // reload WITHOUT leaving BW — the flow that ate Daniel's buddy
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);   // title -> slots (load mode)
  await sleep(300);
  await key('Space', 500);   // pick the slot
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); await sleep(300); }
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await sleep(600);
  T('BUDDY SURVIVED the BW tent-save + reload', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  console.log('\nbuddybwloss: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
