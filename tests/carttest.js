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
  await call('dbg_tp', 350, -5976); await sleep(200);
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
  // the drivers ARE the buddies (Nov 14 overhaul): kart 0 is P1's own
  // puppy (species 0, natural coat 0); every AI seat wears a buddy too
  { const d0 = await call('dbg_crtdrv', 0);
    T('kart 0 is driven by your own buddy (puppy, natural coat)', ((d0 / 100) | 0) === 0 && (((d0 / 10) | 0) % 10) === 0, 'drv0=' + d0); }
  { let allBuddies = true; const sp = new Set();
    for (let k = 0; k < 6; k++){ const d = await call('dbg_crtdrv', k); const s = (d / 100) | 0; if (s < 0 || s > 5) allBuddies = false; sp.add(s); }
    T('every kart has a buddy driver, species varied', allBuddies && sp.size >= 4, 'species=' + [...sp].join(',')); }
  T('racing after the countdown', (await poll('dbg_crtstate', v => v === 1, 8000)) === 1, await call('dbg_crtstate'));

  // drive for real: FIRE is the gas now (Nov 14 — the stick only
  // steers), so hold Space down the bottom straight
  const prog0 = await call('dbg_crtprog');
  await hold('Space', 2500);
  const prog1 = await call('dbg_crtprog');
  T('the kart drives and gains ground', prog1 > prog0 + 120, prog0 + ' -> ' + prog1);
  const pos = await call('dbg_crtpos');
  T('a sane live position', pos >= 1 && pos <= 6, 'pos=' + pos);
  await cv.screenshot({ path: __dirname + '/cart-race.png' });

  // the GRAND PRIX (Nov 14): four races, one per HOME-cup region,
  // points banked after each, standings between, a rank at the end.
  // Each race: skip to the last straight of lap 3 and drive it home.
  for (let r = 0; r < 4; r++) {
    T('race ' + (r + 1) + ' runs on region track ' + r, (await call('dbg_crttrack')) === r, await call('dbg_crttrack'));
    await call('dbg_crtskip');
    await hold('Space', 1600);
    T('race ' + (r + 1) + ' finish banner', (await poll('dbg_crtstate', v => v === 2, 6000)) === 2, await call('dbg_crtstate'));
    if (r < 3) {
      T('standings after race ' + (r + 1), (await poll('dbg_crtstate', v => v === 4, 9000)) === 4, await call('dbg_crtstate'));
      T('points banked after race ' + (r + 1), (await call('dbg_crtpts')) >= 3, await call('dbg_crtpts'));
      if (r === 0) await cv.screenshot({ path: __dirname + '/cart-standings.png' });
      await key('Space', 300);   // FIRE: roll on to the next race
      T('race ' + (r + 2) + ' countdown', (await poll('dbg_crtstate', v => v === 0, 6000)) === 0, await call('dbg_crtstate'));
      T('race ' + (r + 2) + ' racing', (await poll('dbg_crtstate', v => v === 1, 8000)) === 1, await call('dbg_crtstate'));
    } else {
      T('final standings after race 4', (await poll('dbg_crtstate', v => v === 5, 9000)) === 5, await call('dbg_crtstate'));
      const rank = await call('dbg_crtgprank');
      T('a final rank was earned', rank >= 1 && rank <= 6, 'rank=' + rank);
      T('the cup best was saved (packed home rank)', (await call('dbg_crtcupbest')) % 8 === rank, await call('dbg_crtcupbest'));
      await cv.screenshot({ path: __dirname + '/cart-final.png' });
    }
  }
  T('the cup auto-exits after the final standings', (await poll('dbg_bwmode', v => v === -1, 12000)) === -1, await call('dbg_bwmode'));
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
