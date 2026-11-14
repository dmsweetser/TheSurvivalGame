// maptest.js — minimap + quest compass in Banzai World (story mode)
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
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const tgt = async () => [await call('dbg_bwqtarget', 0), await call('dbg_bwqtarget', 1), await call('dbg_bwqtarget', 2)];
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  // story entry: Q0 points at Elder Patchina
  await call('dbg_bwstory');
  await sleep(300);
  let t = await tgt();
  T('story entry: target is Patchina', t[0] === -2620 && t[1] === -1440 && t[2] === 1, t);
  // talk to her: Q0 done, Q1 active -> target switches to the farm
  await call('dbg_bwtele_npc', 0);
  await sleep(300);
  await key('KeyE', 200); await sleep(300);
  t = await tgt();
  T('after elder: target is the server farm (main quest outranks hermit)', t[0] === 3400 && t[1] === -2400 && t[2] === 1, t);
  // screenshot: buddy at the village, farm far away -> minimap + compass visible
  await page.screenshot({ path: __dirname + '/story-minimap.png' });
  // free play: no quest target at all
  await key('Escape', 200); await sleep(300);
  await call('dbg_bwenter');
  await sleep(300);
  t = await tgt();
  T('free play: no quest target', t[2] === 0, t);
  console.log(`maptest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
