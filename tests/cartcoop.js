// cartcoop.js — SUPER BUDDY CART head-to-head (Nov 14): the waiting-room
// flow. Each player must pick the kart cabinet on their OWN side; the
// two waiting rooms then link, both players ready up, and one shared
// race runs with both inputs live and both records saved.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    function mkpad(index) {
      const buttons = [];
      for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
      return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
    }
    window.__pads = [mkpad(0), mkpad(1)];
    navigator.getGamepads = () => window.__pads;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  async function poll(fn, want, timeout = 10000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  const key = async (k, ms = 120) => { await page.keyboard.down(k); await sleep(90); await page.keyboard.up(k); await sleep(ms); };
  const btn = async (pi, bi, ms = 130) => {
    await page.evaluate(([p, b]) => window.__btn(p, b, true), [pi, bi]);
    await sleep(ms);
    await page.evaluate(([p, b]) => window.__btn(p, b, false), [pi, bi]);
    await sleep(120);
  };
  const padSet = async (bi, on) => page.evaluate(([p, b, o]) => window.__btn(p, b, o), [1, bi, on]);
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });

  // co-op on: the co-op row sits at menu_base_count() — one above the
  // last row (NAME) — but its index depends on the menu's pad/keyboard
  // flavour latch (SETTINGS row hidden or not), which races the shell's
  // pad reporting at boot. Navigate by the live menu count instead of
  // a fixed number of downs: coop index = dbg_menucount - 1 either way.
  await sleep(800);   // let the pad/keyboard flavour latch settle first
  const mc = await call('dbg_menucount');
  const coopRow = mc - 1;
  for (let i = 0; i < coopRow; i++) await key('ArrowDown', 160);
  ok((await call('dbg_menusel')) === coopRow, 'menu on the co-op row', await call('dbg_menusel'));
  // verify-and-retry: a press can fall between frames under load
  for (let t = 0; t < 4 && (await call('dbg_coop')) !== 1; t++) await key('Space', 300);
  ok((await call('dbg_coop')) === 1, 'co-op toggled on', await call('dbg_coop'));
  for (let i = 0; i < coopRow; i++) await key('ArrowUp', 160);
  for (let t = 0; t < 4 && (await call('dbg_state')) !== 1; t++) { await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'co-op run started');
  await btn(1, 0, 160);
  ok((await poll('dbg_p2', v => v > 0, 6000)) > 0, 'P2 dropped in');
  const p2slot = await call('dbg_p2');
  await call('dbg_adopt', 0);
  await call('dbg_adopt2', 0);
  await sleep(400);

  // both players into the mall (the pull-in needs P2 in the mall)
  await call('dbg_tp', 0, 80);
  await key('KeyE', 200); await sleep(400);
  await call('dbg_tp2', 0, 80);
  await btn(1, 1, 180); await sleep(500);

  // P1 to the Arcade (east-wall door strip), then the kart cabinet
  await call('dbg_tp', 230, -5976);
  await key('KeyE', 200);
  ok((await poll('dbg_inarcade', v => v === 1, 5000)) === 1, 'P1 entered the Arcade');
  await call('dbg_tp', 256, -11898);
  await sleep(250);
  await key('KeyE', 200);
  ok((await poll('dbg_arcgate', v => v === 1, 5000)) === 1, 'cart cabinet panel opened');
  await key('Space', 200);
  ok((await poll('dbg_bwmode', v => v === 7, 5000)) === 7, 'P1 launched SUPER BUDDY CART');
  ok((await call('dbg_crtstate')) === 3, 'P1 lands in the waiting room', await call('dbg_crtstate'));
  ok((await call('dbg_crtduo')) === -1, 'nobody is dragged in: no link yet', await call('dbg_crtduo'));
  ok((await call('dbg_bwstate2')) === 0, 'P2 is NOT in the race yet');

  // P2 must choose the cart on their own side: Arcade door, cabinet, PLAY
  await call('dbg_tp2', 230, -5976);
  await btn(1, 1, 180); await sleep(400);
  await call('dbg_tp2', 256, -11898);
  await sleep(250);
  await btn(1, 1, 180); await sleep(400);
  await btn(1, 0, 180);   // pad A picks PLAY on P2's own panel
  ok((await poll('dbg_bwstate2', v => v === 1, 5000)) === 1, 'P2 picked the cart too');
  ok((await poll('dbg_crtduo', v => v === p2slot, 5000)) === p2slot, 'the waiting rooms linked up', await call('dbg_crtduo'));
  await sleep(300);
  await page.screenshot({ path: __dirname + '/cart-lobby-duo.png' });

  // both ready up: P1 on Space, P2 on pad A — then the countdown runs
  await key('Space', 200);
  await btn(1, 0, 180);
  ok((await poll('dbg_crtstate', v => v === 0 || v === 1, 8000)) !== 3, 'both ready: the race is starting');
  ok((await poll('dbg_crtstate', v => v === 1, 8000)) === 1, 'countdown finished, racing');

  // both drive: FIRE is go for both (Nov 14) — P1 holds Space,
  // P2 holds pad A (button 0); the sticks only steer
  await page.keyboard.down('Space');
  await padSet(0, true);
  await sleep(2600);
  await page.keyboard.up('Space');
  await padSet(0, false);
  const pg1 = await call('dbg_crtprog'), pg2 = await call('dbg_crtprog2');
  ok(pg1 > 60, 'P1 kart moves on keyboard gas', pg1);
  ok(pg2 > 60, 'P2 kart moves on pad gas', pg2);
  await sleep(300);
  await page.screenshot({ path: __dirname + '/cart-duo.png' });

  // P1 skips home and finishes; the race must stay open for P2
  await call('dbg_crtskip');
  await page.keyboard.down('Space'); await sleep(2200); await page.keyboard.up('Space');
  ok((await call('dbg_crtpos')) === 1, 'P1 finished first', await call('dbg_crtpos'));
  ok((await call('dbg_crtstate')) === 1, 'race stays open until P2 is home', await call('dbg_crtstate'));
  ok((await call('dbg_bwstate2')) === 1, 'P2 still racing');

  // P2 skips home and finishes second; race 1 of the GP closes
  await call('dbg_crtskip2');
  await padSet(0, true); await sleep(2400); await padSet(0, false);
  ok((await poll('dbg_crtstate', v => v === 2, 8000)) === 2, 'race 1 closed once both humans finished');
  ok((await poll('dbg_crtstate', v => v === 4, 9000)) === 4, 'standings follow race 1 — nobody goes home yet');
  ok((await call('dbg_bwstate2')) === 1, 'P2 is still in the cup');

  // races 2-4 of the HOME cup: host fires through the standings,
  // both karts skip home each race
  for (let r = 1; r < 4; r++) {
    await key('Space', 200);
    ok((await poll('dbg_crtstate', v => v === 1, 9000)) === 1, 'race ' + (r + 1) + ' racing');
    await call('dbg_crtskip');
    await page.keyboard.down('Space'); await sleep(1800); await page.keyboard.up('Space');
    await call('dbg_crtskip2');
    await padSet(0, true); await sleep(2200); await padSet(0, false);
    ok((await poll('dbg_crtstate', v => v === 2, 9000)) === 2, 'race ' + (r + 1) + ' closed');
    if (r < 3) ok((await poll('dbg_crtstate', v => v === 4, 9000)) === 4, 'standings follow race ' + (r + 1));
  }
  ok((await poll('dbg_crtstate', v => v === 5, 9000)) === 5, 'final standings after race 4');
  ok((await call('dbg_crtcupbest')) % 8 >= 1, 'P1 cup best saved', await call('dbg_crtcupbest'));
  ok((await poll('dbg_bwmode', v => v === -1, 12000)) === -1, 'P1 returned to the Arcade');
  ok((await poll('dbg_bwstate2', v => v === 0, 9000)) === 0, 'P2 returned to the Arcade too');
  ok((await call('dbg_crtbest')) === 1, 'P1 best finish saved as P1', await call('dbg_crtbest'));
  ok((await call('dbg_crtbest2')) === 2, 'P2 best finish saved as P2', await call('dbg_crtbest2'));

  console.log('\ncartcoop: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
