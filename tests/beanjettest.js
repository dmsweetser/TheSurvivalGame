// beanjettest.js — the bean's second trick (Nov 10): used INSIDE the
// epicenter the Backfire Bean is a JETPACK — free flight over
// everything until you FIRE or take ONE HIT, and then it drops you
// where you are. Used OUTSIDE, it still flies you home like always.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 130) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(100); } return v; };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  const sx = await call('dbg_px'), sy = await call('dbg_py');
  T('spawned at the epicenter', Math.abs(sx) < 200 && Math.abs(sy - 70) < 200, sx + ',' + sy);

  // 1) inside the epicenter: the bean JETPACKS
  await call('dbg_givebean');
  await key('Digit6', 200);
  T('bean launched the jetpack', (await poll('dbg_jet', v => v === 1, 3000)) === 1, await call('dbg_jet'));
  T('the bean was spent', (await call('dbg_beans', -1)) === 0, await call('dbg_beans', -1));
  await hold('ArrowRight', 2000);
  const jx = await call('dbg_px');
  T('jetted far east (over 400 px in 2 s)', jx - sx > 400, 'dx=' + (jx - sx));
  T('still jetting (no fire, no hit)', (await call('dbg_jet')) === 1, await call('dbg_jet'));

  // 2) firing ends the jet, and drops you THERE (not home)
  await key('Space', 120);
  T('firing ended the jet', (await poll('dbg_jet', v => v !== 1, 3000)) !== 1, await call('dbg_jet'));
  await sleep(600);
  const lx = await call('dbg_px');
  T('dropped where you fired, not flown home', lx > 300, 'x=' + lx);

  // 3) outside the epicenter: the same bean still flies you home
  await call('dbg_tp', 3000, 70); await sleep(250);
  await call('dbg_givebean');
  await key('Digit6', 200);
  await sleep(2500);
  const hx = await call('dbg_px'), hy = await call('dbg_py');
  T('outside bean flew home to the plaza', Math.abs(hx) < 60 && Math.abs(hy - 70) < 60, hx + ',' + hy);
  T('no jetpack from an outside launch', (await call('dbg_jet')) === 0, await call('dbg_jet'));

  // 4) one hit ends the jet
  await call('dbg_givebean');
  await key('Digit6', 200);
  T('jetpack up again', (await poll('dbg_jet', v => v === 1, 3000)) === 1, await call('dbg_jet'));
  const px = await call('dbg_px'), py = await call('dbg_py');
  await call('dbg_spawnbug', px, py);
  T('one bug bite knocked the jet down', (await poll('dbg_jet', v => v !== 1, 6000)) !== 1, await call('dbg_jet'));

  console.log('beanjettest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
