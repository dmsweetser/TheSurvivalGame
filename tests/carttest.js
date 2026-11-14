// carttest.js — SUPER BUDDY CART (Nov 14, 2026): the Arcade's sixth
// cabinet, first on the new island row. A Mario Kart-flavoured
// racer: countdown, 3 laps vs five AI buddies, boost pads, and a
// saved best finish + time. The finish itself is driven via
// dbg_crtskip (parked a short straight from home on lap 3) so the
// suite stays fast; the driving assertions use the real keys.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const hold = async (code, ms) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); };
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // mall, then the arcade
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/cart-arcade.png' });

  // the CART cabinet: island row, east box (256, ARC_CY+102)
  await call('dbg_tp', 256, -11898); await sleep(200);
  T('standing at the cart cabinet', (await call('dbg_cabinet')) === 5, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('cart cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('SUPER BUDDY CART launched (bw_mode 7)', (await poll('dbg_bwmode', v => v === 7, 4000)) === 7, await call('dbg_bwmode'));
  T('the waiting room comes first', (await call('dbg_crtstate')) === 3, await call('dbg_crtstate'));
  await cv.screenshot({ path: __dirname + '/cart-lobby.png' });
  await key('Space', 300);   // FIRE in the waiting room: race solo
  T('countdown running', (await poll('dbg_crtstate', v => v === 0, 4000)) === 0, await call('dbg_crtstate'));
  T('on the grid behind the line', (await call('dbg_crtprog')) < 0, await call('dbg_crtprog'));
  T('racing after the countdown', (await poll('dbg_crtstate', v => v === 1, 8000)) === 1, await call('dbg_crtstate'));

  // drive for real: gas only, down the bottom straight
  const prog0 = await call('dbg_crtprog');
  await hold('ArrowUp', 2500);
  const prog1 = await call('dbg_crtprog');
  T('the kart drives and gains ground', prog1 > prog0 + 120, prog0 + ' -> ' + prog1);
  const pos = await call('dbg_crtpos');
  T('a sane live position', pos >= 1 && pos <= 6, 'pos=' + pos);
  await cv.screenshot({ path: __dirname + '/cart-race.png' });

  // jump to the last straight of lap 3 and drive it home
  await call('dbg_crtskip');
  await hold('ArrowUp', 1600);
  T('finish banner state reached', (await poll('dbg_crtstate', v => v === 2, 6000)) === 2, await call('dbg_crtstate'));
  T('race auto-exits after the banner', (await poll('dbg_bwmode', v => v === -1, 9000)) === -1, await call('dbg_bwmode'));
  T('back in the arcade', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));
  const ry = await call('dbg_py');
  T('...right back at the cart cabinet', Math.abs(ry - (-11890)) < 130, 'py=' + ry);
  T('best finish saved (P1 via the skip)', (await call('dbg_crtbest')) === 1, await call('dbg_crtbest'));
  T('best time saved', (await call('dbg_crtbesttime')) > 0, await call('dbg_crtbesttime'));

  // relaunch: the record greets you, and Escape leaves cleanly
  await call('dbg_tp', 256, -11898); await sleep(200);
  await key('KeyE', 250);
  await key('Space', 300);
  T('relaunched', (await poll('dbg_bwmode', v => v === 7, 4000)) === 7, await call('dbg_bwmode'));
  T('best finish still there', (await call('dbg_crtbest')) === 1, await call('dbg_crtbest'));
  await call('dbg_bwexit'); await sleep(300);
  T('clean exit back to the arcade', (await call('dbg_inarcade')) === 1 && (await call('dbg_bwmode')) === -1);

  console.log('carttest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
